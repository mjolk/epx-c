/**
 * File   : src/epaxos.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 03:39
 */

#include "epaxos.h"
#include "replica.h"

void fast_path_timeout(struct timer*);
void slow_path_timeout(struct timer*);
void cancel_timeout(struct timer *t){
    timer_cancel(t);
}

void phase1(struct replica *r, struct instance *i, struct message *m){
    instance_reset(i);
    struct seq_deps_probe p = {
        .updated = 0,
        .deps = {}
    };
    i->seq = sd_for_command(r, i->command, &i->key, &p) + 1;
    i->status = PRE_ACCEPTED;
    i->on = &fast_path_timeout;
    memcpy(i->deps, p.deps, sizeof(p.deps)/sizeof(p.deps[0]));
    int rg = register_instance(r, i);
    if(rg < 0) return;
    m->seq = i->seq;
    m->type = PRE_ACCEPT;
    m->command = i->command;
    memcpy(m->deps, i->deps, sizeof(i->deps)/sizeof(i->deps[0]));
    m->ballot = i->ballot;
    m->id = i->key;
    send(r->sink, m);
}

void pre_accept(struct replica *r, struct instance *i, struct message *m){
    if(is_state(i->status, (COMMITTED | ACCEPTED))) return; //TODO sync to stable storage
    if(!is_state(i->status, (PRE_ACCEPTED | NONE))) return;
    size_t ids = sizeof(struct dependency*);
    struct seq_deps_probe p = {
        .updated = 0
    };
    memcpy(p.deps, i->deps, sizeof(i->deps)/ids);
    uint64_t nseq = max_seq(m->seq, (sd_for_command(r, i->command, &i->key,
                    &p) + 1));
    if(m->ballot < i->ballot){
        m->type = NACK;
        m->ballot = i->ballot;
        m->command = i->command;
        m->seq = i->seq;
        memcpy(m->deps, i->deps, sizeof(i->deps)/ids);
        send(r->sink, m);
        return;
    }
    if(!p.updated){
        i->status = PRE_ACCEPTED_EQ;
    }
    i->command = m->command;
    i->ballot = m->ballot;
    memcpy(i->deps, p.deps, sizeof(p.deps)/ids);
    i->seq = nseq;
    //TODO sync to stable storage
    if(i->seq != m->seq || p.updated ||
            has_uncommitted_deps(i) || !is_initial_ballot(m->ballot)){
        m->type = PRE_ACCEPT_REPLY;
        m->seq = nseq;
        memcpy(m->deps, i->deps, sizeof(i->deps)/ids);
        send(r->sink, m);
        return;
    }
    m->type = PRE_ACCEPT_OK;
    send(r->sink, m);
}

void pre_accept_ok(struct replica *r, struct instance *i, struct message *m){
    if(!is_state(i->status, PRE_ACCEPTED) ||
            !is_initial_ballot(i->ballot)) return;
    ++i->lt->pre_accept_oks;
    if(i->lt->pre_accept_oks >= (N/2) && i->lt->equal &&
            !has_uncommitted_deps(i)){
        if(i->command->writing){
            //TODO reply to client
        }
        m->type = COMMIT;
    } else if(i->lt->pre_accept_oks >= (N/2)){
        m->type = ACCEPT;
    }
    send(r->sink, m);
}

void pre_accept_reply(struct replica *r, struct instance *i, struct message *m){
    if(!is_state(i->status, PRE_ACCEPTED) || i->ballot != m->ballot) return;
    ++i->lt->pre_accept_oks;
    i->lt->equal = 0;
    if(m->seq > i->seq){
        i->seq = m->seq;
        i->lt->equal += 1;
    }
    for(int ds = 0; ds < N; ds++){
        i->lt->equal += update_deps(i->deps, m->deps[ds]);
    }
    i->lt->equal = (i->lt->equal == 0)?1:0;
    if(((i->lt->pre_accept_oks +1) >= N/2) && i->lt->equal
            && is_initial_ballot(i->ballot) && !has_uncommitted_deps(i)){
        i->on = &cancel_timeout;
        i->status = COMMITTED;
        if(i->command->writing){
            //TODO reply to client
        }
        m->type = COMMIT;
        send(r->sink, m);
        return;
    } else if((i->lt->pre_accept_oks +1) >= (N/2)){
        m->type = ACCEPT;
    }
    send(r->sink, m);
}

//TODO add checkpointing
void accept(struct replica *r, struct instance *i, struct message *m){
    if(is_state(i->status, (COMMITTED | EXECUTED))) return;
    if(m->ballot < i->ballot){
        m->type = NACK;
        send(r->sink, m);
        return;
    }
    i->status = ACCEPTED;
    i->ballot = m->ballot;
    i->seq = m->seq;
    memcpy(i->deps, m->deps, sizeof(m->deps)/sizeof(m->deps[0]));
    send(r->sink, m);
}

void accept_reply(struct replica *r, struct instance *i, struct message *m){
    if(!is_state(i->status, ACCEPTED) || i->ballot != m->ballot) return;
    ++i->lt->accept_oks;
    if((i->lt->accept_oks+1) > N/2){
        i->on = &cancel_timeout;
        i->status = COMMITTED;
        if(i->command->writing){
            //TODO reply to client
        }
        m->type = COMMIT;
        send(r->sink, m);
    }
}

void recover(struct replica *r, struct instance *i) {
    //TODO check if failed instance has clients waiting
    if(i->lt == 0){
        i->lt = malloc(sizeof(struct leader_tracker));
        if(i->lt == 0){
            return;
        }
    }
    i->lt->recovery_status = PREPARING;
    struct recovery_instance *ri = malloc(sizeof(struct recovery_instance));
    if(ri == 0) return;
    if(i->status == ACCEPTED){
        i->lt->max_ballot = i->ballot;
    } else if(i->status == PRE_ACCEPTED){
        ri->pre_accept_count = 1;
        ri->leader_replied = (r->id == i->key.replica_id);
    }
    ri->status = i->status;
    ri->command = i->command;
    ri->seq = i->seq;
    memcpy(ri->deps, i->deps, sizeof(i->deps)/sizeof(i->deps[0]));
    i->lt->ri = ri;
    i->ballot = larger_ballot(i->ballot, r->id);
    struct message *recm = malloc(sizeof(struct message));
    if(recm == 0) return;
    recm->type = PREPARE;
    recm->ballot = i->ballot;
    recm->id = i->key;
    recm->command = i->command;
    recm->seq = i->seq;
    send(r->sink, recm);
}

void prepare(struct replica *r, struct instance *i, struct message *m){
    m->from = r->id;
    if(m->ballot < i->ballot){
        m->type = NACK;
        send(r->sink, m);
        return;

    } else {
        i->ballot = m->ballot;
    }
    m->type = PREPARE_REPLY;
    m->command = i->command;
    memcpy(m->deps, i->deps, sizeof(i->deps)/sizeof(i->deps[0]));
    m->seq = i->seq;
    m->ballot = i->ballot;
    m->instance_status = i->status;
    send(r->sink, m);
}

void prepare_reply(struct replica *r, struct instance *i, struct message *m){
    if(i->lt->ri == 0 || i->lt->recovery_status != PREPARING) return;
    ++i->lt->prepare_oks;
    if(is_state(m->instance_status, (COMMITTED | EXECUTED))){
        struct command *cmd = m->command;
        i->status = COMMITTED;
        i->seq = m->seq;
        memcpy(i->deps, m->deps, sizeof(m->deps)/sizeof(m->deps[0]));
        m->command = i->command;
        i->command = cmd;
        m->type = COMMIT;
        //TODO acknowledge clients
        send(r->sink, m);
        return;
    }
    if(is_state(m->instance_status, ACCEPTED)){
        if(i->lt->max_ballot < m->ballot || i->lt->ri == 0){
            i->lt->max_ballot = m->ballot;
            if(i->lt->ri == 0){
                i->lt->ri = malloc(sizeof(struct recovery_instance));
                if(i->lt->ri == 0) return;
            }
            update_recovery_instance(i->lt->ri, m);
            i->lt->ri->leader_replied = 0;
            i->lt->ri->pre_accept_count = 0;
        }
    }
    if(is_state(m->instance_status, (PRE_ACCEPTED | PRE_ACCEPTED_EQ)) &&
            ((i->lt->ri == 0) || i->lt->ri->status < ACCEPTED)){
        if(i->lt->ri == 0){
            i->lt->ri = malloc(sizeof(struct recovery_instance));
            if(i->lt->ri == 0) return;
            update_recovery_instance(i->lt->ri, m);
            i->lt->ri->leader_replied = 0;
            i->lt->ri->pre_accept_count = 1;
        } else if((m->seq == i->seq) && equal_deps(m->deps, i->deps)){
            ++i->lt->ri->pre_accept_count;
        } else if(is_state(m->instance_status, PRE_ACCEPTED_EQ)){
            update_recovery_instance(i->lt->ri, m);
            i->lt->ri->leader_replied = 0;
            i->lt->ri->pre_accept_count = 1;
        }
        if(m->from == m->id.replica_id){
            i->lt->ri->leader_replied = 1;
            return;
        }
    }
    if(i->lt->prepare_oks < N/2){
        return;
    }
    struct recovery_instance *ri = i->lt->ri;
    if(ri == 0){
        return;
    }
    if(is_state(ri->status, ACCEPTED) || ((ri->leader_replied == 0) &&
                (ri->pre_accept_count >= N/2 &&
                 is_state(ri->status, PRE_ACCEPTED_EQ)))){
        i->command = ri->command;
        i->seq = ri->seq;
        memcpy(i->deps, ri->deps, sizeof(ri->deps)/sizeof(ri->deps[0]));
        i->status = ACCEPTED;
        i->lt->recovery_status = NONE;
        m->ballot = i->ballot;
        memcpy(m->deps, i->deps, sizeof(i->deps)/sizeof(i->deps[0]));
        m->seq = i->seq;
        m->type = ACCEPT;
        send(r->sink, m);
    } else if((ri->leader_replied == 0) && ri->pre_accept_count >= (N/2+1)/2){
    }
    return;
}

void nack(struct replica *r, struct instance *i, struct message *m){
    return;
}

//TODO checkpointing (commit short/long)..
void commit(struct replica *r, struct instance *i, struct message *m){
    i->status = COMMITTED;
    i->seq = m->seq;
    memcpy(i->deps, m->deps, sizeof(m->deps)/sizeof(m->deps[0]));
}

void progress(struct replica *r, struct instance *i, struct message *m){
    switch(m->type){
        case NACK:
            nack(r, i, m);
            break;
        case PHASE1:
            phase1(r, i, m);
            break;
        case PRE_ACCEPT:
            pre_accept(r, i, m);
            break;
        case PRE_ACCEPT_REPLY:
            pre_accept_reply(r, i, m);
            break;
        case ACCEPT:
            accept(r, i, m);
            break;
        case COMMIT:
            commit(r, i, m);
            break;
        case ACCEPT_REPLY:
            accept_reply(r, i , m);
            break;
        case PRE_ACCEPT_OK:
            pre_accept_ok(r, i, m);
            break;
        case PREPARE:
            prepare(r, i, m);
            break;
        case PREPARE_REPLY:
            prepare_reply(r, i, m);
            break;
        case TRY_PRE_ACCEPT:
            break;
        case TRY_PRE_ACCEPT_REPLY:
            break;
    }
}


void slow_path_timeout(struct timer *t) {
    struct message *nm = malloc(sizeof(struct message));
    if(nm == 0) return;
    nm->ballot = t->instance->ballot;
    nm->command = t->instance->command;
    nm->id = t->instance->key;
    nm->seq = t->instance->seq;
    memcpy(nm->deps, t->instance->deps,
            sizeof(t->instance->deps)/sizeof(t->instance->deps[0]));
    nm->type = PREPARE;
    send(t->instance->r->sink, nm);
}

void fast_path_timeout(struct timer *t) {
    struct message *nm = malloc(sizeof(struct message));
    if(nm == 0) return;
    nm->ballot = t->instance->ballot;
    nm->command = t->instance->command;
    nm->id = t->instance->key;
    nm->seq = t->instance->seq;
    memcpy(nm->deps, t->instance->deps,
            sizeof(t->instance->deps)/sizeof(t->instance->deps[0]));
    nm->type = ACCEPT;
    t->instance->on = &slow_path_timeout;
    timer_set(t, 2);
    send(t->instance->r->sink, nm);
}

int step(struct replica *r, struct message *m) {
    struct instance *i = find_instance(r, &m->id);
    if(i == 0){
        i = instance_from_message(m);
        int rg = register_instance(r, i);
        if(rg != 0) return -1;
    }
    progress(r, i, m);
    return 0;
}

int propose(struct replica *r, struct message *m){
    struct instance *i = instance_from_message(m);
    if(i == 0) return -1;
    struct leader_tracker *lt = malloc(sizeof(struct leader_tracker));
    if(lt == 0) return -1;
    i->lt = lt;
    i->key.instance_id = max_local(r) + 1;
    i->key.replica_id = r->id;
    progress(r, i, m);
    return 0;
}

void run(struct replica *r){
    while(r->running){
        struct message *m = recv(r->crank);
        if(m == 0) continue;
        switch(m->type){
            case PHASE1:
                propose(r, m);
                break;
            default:
                step(r, m);
        }
    }
}

