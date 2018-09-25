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
void cancel_timeout(struct timer *t) {
    timer_cancel(t);
}

void phase1(struct replica *r, struct instance *i, struct message *m) {
    instance_reset(i);
    struct seq_deps_probe p = {
        .updated = 0,
        .deps = {}
    };
    i->seq = sd_for_command(r, i->command, i->key, &p) + 1;
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

void pre_accept(struct replica *r, struct instance *i, struct message *m) {
    if(is_state(i, (COMMITTED | ACCEPTED))) return; //TODO sync to stable storage
    if(!is_state(i, (PRE_ACCEPTED | NONE))) return;
    size_t ids = sizeof(struct instance_id*);
    struct seq_deps_probe p = {
        .updated = 0
    };
    memcpy(p.deps, i->deps, sizeof(i->deps)/ids);
    uint64_t nseq = max_seq(m->seq, (sd_for_command(r, i->command, i->key,
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
    i->command = m->command;
    i->ballot = m->ballot;
    memcpy(i->deps, p.deps, sizeof(p.deps)/ids);
    i->seq = nseq;
    //TODO sync to stable storage
    if(i->seq != m->seq || p.updated || !is_initial_ballot(m->ballot)){
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
    if(!is_state(i, PRE_ACCEPTED) || !is_initial_ballot(i->ballot)) return;
    ++i->pre_accept_oks;
    if(i->pre_accept_oks >= (N/2) && is_initial_ballot(i->ballot)){
        if(i->command->writing){
            //TODO reply to client
        }
        m->type = COMMIT;
    } else if(i->pre_accept_oks >= (N/2)){
        m->type = ACCEPT;
    }
    send(r->sink, m);
}

void pre_accept_reply(struct replica *r, struct instance *i, struct message *m){
    if(!is_state(i, PRE_ACCEPTED) || i->ballot != m->ballot) return;
    ++i->pre_accept_oks;
    i->equal = N + 1;
    if(m->seq > i->seq){
        i->seq = m->seq;
        i->equal -= 1;
    }
    for(int ds = 0; ds < N; ds++){
        i->equal -= update_deps(i->deps, m->deps[ds]);
    }
    if(((i->pre_accept_oks +1) >= N/2) && i->equal
            && is_initial_ballot(i->ballot)){
        i->on = &cancel_timeout;
        i->status = COMMITTED;
        if(i->command->writing){
            //TODO reply to client
        }
        m->type = COMMIT;
        send(r->sink, m);
        return;
    } else if((i->pre_accept_oks +1) >= (N/2)){
        m->type = ACCEPT;
    }
    send(r->sink, m);
}

void accept(struct replica *r, struct instance *i, struct message *m){
    if(is_state(i, (COMMITTED | EXECUTED))) return;
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
    if(!is_state(i, ACCEPTED) || i->ballot != m->ballot) return;
    ++i->accept_oks;
    if((i->accept_oks+1) > N/2){
        i->on = &cancel_timeout;
        i->status = COMMITTED;
        if(i->command->writing){
            //TODO reply to client
        }
        m->type = COMMIT;
        send(r->sink, m);
    }
}

void recover(struct instance *i) {

}

void prepare(struct replica *r, struct instance *i, struct message *m){
    return;
}

void prepare_reply(struct replica *r, struct instance *i, struct message *m){
    return;
}

void nack(struct replica *r, struct instance *i, struct message *m){
    return;
}

void commit(struct replica *r, struct instance *i, struct message *m){
    i->status = COMMITTED;
    i->seq = m->seq;
    memcpy(i->deps, m->deps, sizeof(m->deps)/sizeof(m->deps[0]));
}

void prepare_ok(struct replica *r, struct instance *i, struct message *m){
    return;
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
        case PREPARE_OK:
            prepare_ok(r, i, m);
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
    struct instance *i = find_instance(r, m->id);
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
    i->key->instance_id = max_local(r) + 1;
    i->key->replica_id = r->id;
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

