/**
 * File   : src/epaxos.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 03:39
 */

#include "epaxos.h"
#include "replica.h"

#define size(V) (sizeof(V)/sizeof((V)[0]))
#define MSG_SIZE sizeof(struct message)

void fast_path_timeout(struct replica*, struct timer*);
void slow_path_timeout(struct replica*, struct timer*);
void cancel_timeout(struct replica *r, struct timer *t){
    timer_cancel(t);
}
void progress(struct replica *r, struct instance*, struct message*);

void phase1(struct replica *r, struct instance *i, struct message *m){
    instance_reset(i);
    struct seq_deps_probe p = {
        .updated = 0,
        .deps = {}
    };
    i->seq = sd_for_command(r, m->command, &i->key, &p) + 1;
    i->status = PRE_ACCEPTED;
    i->on = fast_path_timeout;
    memcpy(i->deps, p.deps, size(p.deps));
    int rg = register_instance(r, i);
    if(rg < 0) return;
    m->seq = i->seq;
    m->type = PRE_ACCEPT;
    m->command = i->command;
    memcpy(m->deps, i->deps, size(i->deps));
    m->ballot = i->ballot;
    m->id = i->key;
    chsend(r->chan_io[1], m, MSG_SIZE, 10);
}

void pre_accept(struct replica *r, struct instance *i, struct message *m){
    if(is_state(i->status, (COMMITTED | ACCEPTED))) return; //TODO sync to stable storage
    if(!is_state(i->status, (PRE_ACCEPTED | NONE))) return;
    struct seq_deps_probe p = {
        .updated = 0
    };
    memcpy(p.deps, i->deps, size(i->deps));
    uint64_t nseq = max_seq(m->seq, (sd_for_command(r, i->command, &i->key,
                    &p) + 1));
    if(m->ballot < i->ballot){
        m->type = NACK;
        m->ballot = i->ballot;
        m->command = i->command;
        m->seq = i->seq;
        memcpy(m->deps, i->deps, size(i->deps));
        chsend(r->chan_io[1], m, MSG_SIZE, 10);
        return;
    }
    if(!p.updated){
        i->status = PRE_ACCEPTED_EQ;
    }
    i->command = m->command;
    i->ballot = m->ballot;
    memcpy(i->deps, p.deps, size(p.deps));
    i->seq = nseq;
    //TODO sync to stable storage
    if(i->seq != m->seq || p.updated ||
            has_uncommitted_deps(i) || !is_initial_ballot(m->ballot)){
        m->type = PRE_ACCEPT_REPLY;
        m->seq = nseq;
        memcpy(m->deps, i->deps, size(i->deps));
        chsend(r->chan_io[1], m, MSG_SIZE, 10);
        return;
    }
    m->type = PRE_ACCEPT_OK;
    chsend(r->chan_io[1], m, MSG_SIZE, 10);
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
    chsend(r->chan_io[1], m, MSG_SIZE, 10);
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
        i->lt->equal += update_deps(i->deps, &m->deps[ds]);
    }
    i->lt->equal = (i->lt->equal == 0)?1:0;
    if(((i->lt->pre_accept_oks +1) >= N/2) && i->lt->equal
            && is_initial_ballot(i->ballot) && !has_uncommitted_deps(i)){
        i->on = cancel_timeout;
        i->status = COMMITTED;
        if(i->command->writing){
            //TODO reply to client
        }
        m->type = COMMIT;
        chsend(r->chan_io[1], m, MSG_SIZE, 10);
        return;
    } else if((i->lt->pre_accept_oks +1) >= (N/2)){
        m->type = ACCEPT;
    }
    chsend(r->chan_io[1], m, MSG_SIZE, 10);
}

//TODO add checkpointing
void accept(struct replica *r, struct instance *i, struct message *m){
    if(is_state(i->status, (COMMITTED | EXECUTED))) return;
    if(m->ballot < i->ballot){
        m->type = NACK;
        chsend(r->chan_io[1], m, MSG_SIZE, 10);
        return;
    }
    i->status = ACCEPTED;
    i->ballot = m->ballot;
    i->seq = m->seq;
    memcpy(i->deps, m->deps, size(m->deps));
    chsend(r->chan_io[1], m, MSG_SIZE, 10);
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
        chsend(r->chan_io[1], m, MSG_SIZE, 10);
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
    if(i->status == ACCEPTED){
        i->lt->max_ballot = i->ballot;
    } else if(i->status == PRE_ACCEPTED){
        i->lt->ri.pre_accept_count = 1;
        i->lt->ri.leader_replied = (r->id == i->key.replica_id);
    }
    i->lt->ri.status = i->status;
    i->lt->ri.command = i->command;
    i->lt->ri.seq = i->seq;
    memcpy(i->lt->ri.deps, i->deps, size(i->deps));
    i->ballot = larger_ballot(i->ballot, r->id);
    struct message *recm = malloc(sizeof(struct message));
    if(recm == 0) return;
    recm->type = PREPARE;
    recm->ballot = i->ballot;
    recm->id = i->key;
    recm->command = i->command;
    recm->seq = i->seq;
    chsend(r->chan_io[1], recm, MSG_SIZE, 10);
}

void prepare(struct replica *r, struct instance *i, struct message *m){
    m->from = r->id;
    if(m->ballot < i->ballot){
        m->type = NACK;
        chsend(r->chan_io[1], m, MSG_SIZE, 10);
        return;

    } else {
        i->ballot = m->ballot;
    }
    m->type = PREPARE_REPLY;
    m->command = i->command;
    memcpy(m->deps, i->deps, size(i->deps));
    m->seq = i->seq;
    m->ballot = i->ballot;
    m->instance_status = i->status;
    chsend(r->chan_io[1], m, MSG_SIZE, 10);
}

void prepare_reply(struct replica *r, struct instance *i, struct message *m){
    if(i->lt == 0 || i->lt->recovery_status != PREPARING) return;
    ++i->lt->prepare_oks;
    if(is_state(m->instance_status, (COMMITTED | EXECUTED))){
        struct command *cmd = m->command;
        i->status = COMMITTED;
        i->seq = m->seq;
        memcpy(i->deps, m->deps, size(m->deps));
        m->command = i->command;
        i->command = cmd;
        m->type = COMMIT;
        //TODO acknowledge clients
        chsend(r->chan_io[1], m, MSG_SIZE, 10);
        return;
    }
    if(is_state(m->instance_status, ACCEPTED)){
        if(i->lt->max_ballot < m->ballot || is_state(i->lt->ri.status, NONE)){
            i->lt->max_ballot = m->ballot;
            update_recovery_instance(&i->lt->ri, m);
            i->lt->ri.leader_replied = 0;
            i->lt->ri.pre_accept_count = 0;
        }
    }
    if(is_state(m->instance_status, (PRE_ACCEPTED | PRE_ACCEPTED_EQ)) &&
            (is_state(i->lt->ri.status, NONE) || i->lt->ri.status < ACCEPTED)){
        if(is_state(i->lt->ri.status, NONE)){
            update_recovery_instance(&i->lt->ri, m);
            i->lt->ri.leader_replied = 0;
            i->lt->ri.pre_accept_count = 1;
        } else if((m->seq == i->seq) && equal_deps(m->deps, i->deps)){
            ++i->lt->ri.pre_accept_count;
        } else if(is_state(m->instance_status, PRE_ACCEPTED_EQ)){
            update_recovery_instance(&i->lt->ri, m);
            i->lt->ri.leader_replied = 0;
            i->lt->ri.pre_accept_count = 1;
        }
        if(m->from == m->id.replica_id){
            i->lt->ri.leader_replied = 1;
            return;
        }
    }
    if(i->lt->prepare_oks < N/2){
        return;
    }
    struct recovery_instance ri = i->lt->ri;
    if(is_state(ri.status, NONE)){
        return;
    }
    if(is_state(ri.status, ACCEPTED) || ((ri.leader_replied == 0) &&
                (ri.pre_accept_count >= N/2 &&
                 is_state(ri.status, PRE_ACCEPTED_EQ)))){
        i->command = ri.command;
        i->seq = ri.seq;
        memcpy(i->deps, ri.deps, size(ri.deps));
        i->status = ACCEPTED;
        i->lt->recovery_status = NONE;
        m->ballot = i->ballot;
        memcpy(m->deps, i->deps, size(i->deps));
        m->seq = i->seq;
        m->type = ACCEPT;
        chsend(r->chan_io[1], m, MSG_SIZE, 10);
    } else if((ri.leader_replied == 0) && ri.pre_accept_count >= (N/2+1)/2){
        i->lt->pre_accept_oks = 0;
        i->lt->nacks = 0;
        for(int rc = 0;rc<N;rc++){
            i->lt->quorum[rc] = 1;
        }
        struct instance *conflict = pac_conflict(r, i, ri.command, ri.seq,
                ri.deps);
        if(conflict){
            if(conflict->status >= COMMITTED){
                m->type = PHASE1;
                m->command = ri.command;
                m->ballot = i->ballot;
                progress(r, i, m);
                return;
            } else {
                i->lt->nacks = 0;
                i->lt->quorum[r->id] = 0;
            }
        } else{
            i->command = ri.command;
            i->seq = ri.seq;
            memcpy(i->deps, ri.deps, size(ri.deps));
            i->status = PRE_ACCEPTED;
            i->lt->pre_accept_oks = 1;
        }
        i->lt->recovery_status = TRY_PRE_ACCEPTING;

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


void slow_path_timeout(struct replica *r, struct timer *t) {
    struct message *nm = malloc(sizeof(struct message));
    if(nm == 0) return;
    nm->ballot = t->instance->ballot;
    nm->command = t->instance->command;
    nm->id = t->instance->key;
    nm->seq = t->instance->seq;
    memcpy(nm->deps, t->instance->deps,
            sizeof(t->instance->deps)/sizeof(t->instance->deps[0]));
    nm->type = PREPARE;
    chsend(r->chan_io[1], nm, MSG_SIZE, 10);
}

void fast_path_timeout(struct replica *r, struct timer *t) {
    struct message *nm = malloc(sizeof(struct message));
    if(nm == 0) return;
    nm->ballot = t->instance->ballot;
    nm->command = t->instance->command;
    nm->id = t->instance->key;
    nm->seq = t->instance->seq;
    memcpy(nm->deps, t->instance->deps,
            sizeof(t->instance->deps)/sizeof(t->instance->deps[0]));
    nm->type = ACCEPT;
    t->instance->on = slow_path_timeout;
    timer_set(t, 2);
    chsend(r->chan_io[1], nm, MSG_SIZE, 10);
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

void run(struct replica *r, int chan_s){
    r->running = 1;
    struct message *nm;
    int rc;
    while(r->running && rc >= 0){
        struct message nm;
        struct chclause cls[] = {
            {CHRECV, r->chan_tick[1], &nm, MSG_SIZE},/** advance time **/
            {CHRECV, r->chan_ii[1], &nm, MSG_SIZE},/** churn **/
            {CHRECV, r->chan_propose[1], &nm, MSG_SIZE} /** new io **/
        };
        rc = 0;
        rc = choose(cls, 3, -1);
        switch(rc){
            case 0:
                tick(r);
                break;
            case 1:
                rc = step(r, &nm);
                break;
            case 2:
                rc = propose(r, &nm);
                break;
        }
    }
    //rc = chsend(chan_s, &rc, 1, -1);
    rc = chdone(chan_s);
    if(rc < 0){
        //totally fucked
        exit(-1);
    }
    return;
}
