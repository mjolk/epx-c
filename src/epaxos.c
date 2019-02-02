/**
 * File   : epaxos.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 03:39
 */

#include "epaxos.h"
#include <stdio.h>

uint64_t dh_key(uint64_t instance_id, size_t replica_id){
    return (instance_id << 16) | replica_id;
}

void noop (struct replica *s, struct timer *t){
    return;
}

void cancel_timeout(struct instance *i){
    i->ticker.on = noop;
    timer_cancel(&i->ticker);
}

void fast_path_timeout(struct replica*, struct timer*);
void slow_path_timeout(struct replica*, struct timer*);
void init_timeout(struct instance *i){
    timer_set(&i->ticker, 2);
    i->ticker.on = slow_path_timeout;
}

void init_fp_timeout(struct instance *i){
    timer_set(&i->ticker, 2);
    i->ticker.on = fast_path_timeout;
}
void progress(struct replica *r, struct instance*, struct message*);
void recover(struct replica *r, struct instance *i, struct message *m) {
    init_timeout(i);
    if(i->lt == 0){
        i->lt = malloc(sizeof(struct leader_tracker));
        if(i->lt == 0){
            return;
        }
        i->lt->ri.status = NONE;
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
    memcpy(i->lt->ri.deps, i->deps, DEPSIZE);
    i->ballot = larger_ballot(i->ballot, r->id);
    m->ballot = i->ballot;
    m->id = i->key;
    m->command = i->command;
    m->seq = i->seq;
    send_prepare(r, m);
}

void phase1(struct replica *r, struct instance *i, struct message *m){
    instance_reset(i);
    init_fp_timeout(i);
    //add updated field somewhere, use lt.. should remove this probe struct
    struct seq_deps_probe p = {
        .updated = 0,
    };
    memcpy(p.deps, m->deps, DEPSIZE);
    i->seq = sd_for_command(r, m->command, &i->key, &p) + 1;
    i->status = PRE_ACCEPTED;
    memcpy(i->deps, p.deps, DEPSIZE);
    m->seq = i->seq;
    m->command = i->command;
    memcpy(m->deps, i->deps, DEPSIZE);
    m->ballot = i->ballot;
    m->id = i->key;
    send_pre_accept(r, m);
}

void pre_accept(struct replica *r, struct instance *i, struct message *m){
    if(is_state(i->status, (COMMITTED | ACCEPTED))) return; //TODO sync to stable storage
    if(!is_state(i->status, (PRE_ACCEPTED | NONE))) return;
    struct seq_deps_probe p = {
        .updated = 0
    };
    memcpy(p.deps, i->deps, DEPSIZE);
    uint64_t nseq = max_seq(m->seq, (sd_for_command(r, i->command, &i->key,
                    &p) + 1));
    if(m->ballot < i->ballot){
        m->ballot = i->ballot;
        m->command = i->command;
        m->seq = i->seq;
        m->nack = 1;
        memcpy(m->deps, i->deps, DEPSIZE);
        send_pre_accept_reply(r, m);
        return;
    }
    if(!p.updated){
        i->status = PRE_ACCEPTED_EQ;
    }
    i->command = m->command;
    i->ballot = m->ballot;
    memcpy(i->deps, p.deps, DEPSIZE);
    i->seq = nseq;
    if(i->command == 0){
        //TODO barrier
    }
    //TODO sync to stable storage
    if(i->seq != m->seq || p.updated ||
            has_uncommitted_deps(i) || !is_initial_ballot(m->ballot)){
        m->seq = nseq;
        memcpy(m->deps, i->deps, DEPSIZE);
        send_pre_accept_reply(r, m);
        return;
    }
    send_pre_accept_ok(r, m);
}

void pre_accept_ok(struct replica *r, struct instance *i, struct message *m){
    if(!is_state(i->status, PRE_ACCEPTED) ||
            !is_initial_ballot(i->ballot)) return;
    ++i->lt->pre_accept_oks;
    if(i->lt->pre_accept_oks >= (N/2) && i->lt->equal &&
            !has_uncommitted_deps(i)){
        cancel_timeout(i);
        i->status = COMMITTED;
        send_exec(r, m);
        send_commit(r, m);
        if(i->command->writing){
            send_eo(r, m);
        }
    } else if(i->lt->pre_accept_oks >= (N/2)){
        init_timeout(i);
        send_accept(r, m);
    }
}

void pre_accept_reply(struct replica *r, struct instance *i, struct message *m){
    if(!is_state(i->status, PRE_ACCEPTED) || i->ballot != m->ballot) return;
    if(m->nack){
        i->lt->nacks++;
        if(m->ballot > i->lt->max_ballot){
            i->lt->max_ballot = m->ballot;
        }
        if(i->lt->nacks >= N/2){
            recover(r, i, m);
        }
        return;
    }
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
        cancel_timeout(i);
        i->status = COMMITTED;
        send_exec(r, m);
        send_commit(r, m);
        if(i->command->writing){
            send_eo(r, m);
        }
        return;
    } else if((i->lt->pre_accept_oks +1) >= (N/2)){
        init_timeout(i);
        send_accept(r, m);
    }
}

//TODO add checkpointing
void accept(struct replica *r, struct instance *i, struct message *m){
    if(is_state(i->status, (COMMITTED | EXECUTED))) return;
    if(m->ballot < i->ballot){
        m->nack = 1;
        m->ballot = i->ballot;
        send_accept_reply(r, m);
        return;
    }
    i->status = ACCEPTED;
    i->ballot = m->ballot;
    i->seq = m->seq;
    memcpy(i->deps, m->deps, DEPSIZE);
    if(m->command == 0){
        //TODO barrier
    }
    send_accept_reply(r, m);
}

void accept_reply(struct replica *r, struct instance *i, struct message *m){
    if(!is_state(i->status, ACCEPTED) || i->ballot != m->ballot) return;
    if(m->nack){
        i->lt->nacks++;
        if(m->ballot > i->lt->max_ballot){
            i->lt->max_ballot = m->ballot;
        }
        if(i->lt->nacks >= N/2){
            recover(r, i, m);
        }
        return;
    }
    ++i->lt->accept_oks;
    if((i->lt->accept_oks+1) > N/2){
        cancel_timeout(i);
        i->status = COMMITTED;
        send_exec(r, m);
        send_commit(r, m);
        if(i->command->writing){
            send_eo(r, m);
        }
    }
}

void prepare(struct replica *r, struct instance *i, struct message *m){
    m->from = r->id;
    if(m->ballot < i->ballot){
        m->nack = 1;
        send_prepare_reply(r, m);
        return;

    } else {
        i->ballot = m->ballot;
    }
    m->command = i->command;
    memcpy(m->deps, i->deps, DEPSIZE);
    m->seq = i->seq;
    m->ballot = i->ballot;
    m->instance_status = i->status;
    send_prepare_reply(r, m);
}

void prepare_reply(struct replica *r, struct instance *i, struct message *m){
    if(i->lt == 0 || i->lt->recovery_status != PREPARING) return;
    if(m->nack){
        i->lt->nacks++;
        return;
    }
    ++i->lt->prepare_oks;
    if(is_state(m->instance_status, (COMMITTED | EXECUTED))){
        cancel_timeout(i);
        i->status = COMMITTED;
        i->seq = m->seq;
        memcpy(i->deps, m->deps, DEPSIZE);
        i->command = m->command;
        m->ballot = i->ballot;
        send_commit(r, m);
        send_exec(r, m);
        if(i->command->writing){
            send_eo(r, m);
        }
        return;
    }
    if(is_state(m->instance_status, ACCEPTED)){
        if(i->lt->max_ballot < m->ballot || is_state(i->lt->ri.status, NONE)){
            i->lt->max_ballot = m->ballot;
            update_recovery_instance(&i->lt->ri, m, 0, 0);
        }
    }
    if(is_state(m->instance_status, (PRE_ACCEPTED | PRE_ACCEPTED_EQ)) &&
            (i->lt->ri.status < ACCEPTED)){
        if(is_state(i->lt->ri.status, NONE)){
            update_recovery_instance(&i->lt->ri, m, 0, 1);
        } else if((m->seq == i->seq) && equal_deps(m->deps, i->deps)){
            ++i->lt->ri.pre_accept_count;
        } else if(is_state(m->instance_status, PRE_ACCEPTED_EQ)){
            update_recovery_instance(&i->lt->ri, m, 0, 1);
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
        m->deps[m->id.replica_id].id.instance_id = m->id.instance_id -1;
        i->lt->recovery_status = NONE;
        i->status = ACCEPTED;
        memcpy(i->deps, m->deps, DEPSIZE);
        m->command = 0;
        init_timeout(i);
        send_accept(r, m);
        return;
    }
    if(is_state(ri.status, ACCEPTED) || ((ri.leader_replied == 0) &&
                (ri.pre_accept_count >= N/2 &&
                 is_state(ri.status, PRE_ACCEPTED_EQ)))){
        i->command = ri.command;
        i->seq = ri.seq;
        memcpy(i->deps, ri.deps, DEPSIZE);
        i->status = ACCEPTED;
        i->lt->recovery_status = NONE;
        m->ballot = i->ballot;
        memcpy(m->deps, i->deps, DEPSIZE);
        m->seq = i->seq;
        init_timeout(i);
        send_accept(r, m);
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
            memcpy(i->deps, ri.deps, DEPSIZE);
            i->status = PRE_ACCEPTED;
            i->lt->pre_accept_oks = 1;
        }
        i->lt->recovery_status = TRY_PRE_ACCEPTING;
        m->command = i->command;
        memcpy(m->deps, i->deps, DEPSIZE);
        m->seq = i->seq;
        m->ballot = i->ballot;
        init_timeout(i);
        send_try_pre_accept(r, m);
    } else {
        i->lt->recovery_status = NONE;
        m->type = PHASE1;
        m->command = ri.command;
        m->ballot = i->ballot;
        progress(r, i, m);
    }
    return;
}

void try_pre_accept(struct replica *r, struct instance *i, struct message *m){
    if(i->ballot > m->ballot){
        m->conflict.instance_id = m->id.instance_id;
        m->conflict.replica_id = m->id.replica_id;
        m->conflict.status = i->status;
        m->ballot = i->ballot;
        m->nack = 1;
        send_try_pre_accept_reply(r, m);
        return;
    }
    struct instance *conflict = pac_conflict(r, i, m->command, m->seq,
            m->deps);
    if(conflict){
        m->conflict.instance_id = conflict->key.instance_id;
        m->conflict.replica_id = conflict->key.replica_id;
        m->conflict.status = conflict->status;
        m->ballot = i->ballot;
        m->nack = 1;
        send_try_pre_accept_reply(r, m);
    } else {
        i->command = m->command;
        memcpy(i->deps, m->deps, DEPSIZE);
        i->ballot = m->ballot;
        i->seq = m->seq;
        i->status = PRE_ACCEPTED;
        send_try_pre_accept_reply(r, m);
    }
}

void try_pre_accept_reply(struct replica *r, struct instance *i,
        struct message *m){
    if(i->lt == 0 || i->lt->recovery_status != TRY_PRE_ACCEPTING ||
            i->lt->ri.status == NONE) return;

    struct recovery_instance ri = i->lt->ri;
    if(m->nack){
        ++i->lt->nacks;
        if(m->ballot > i->ballot){
            m->ballot = larger_ballot(m->ballot, r->id);
            init_timeout(i);
            send_pre_accept(r, m);
            return;
        }
        ++i->lt->try_pre_accept_oks;
        if(m->conflict.instance_id == m->id.instance_id &&
                m->conflict.replica_id == m->id.replica_id){
            i->lt->quorum[m->from] = 0;
            i->lt->quorum[m->conflict.replica_id] = 0;
            init_timeout(i);
            send_prepare(r, m);
            return;
        }
        int nq = 0;
        for(int rc = 0; rc < N; rc++){
            if(i->lt->quorum[rc] == 0){
                nq++;
            }
        }
        if(m->conflict.status >= COMMITTED || nq > N/2){
            i->lt->recovery_status = NONE;
            m->type = PHASE1;
            progress(r, i, m);
        }
        if(nq == N/2){
            khint_t k = kh_get(deferred, r->dh, dh_key(m->id.instance_id,
                        m->id.replica_id));
            if(k != kh_end(r->dh)){
                if(i->lt->quorum[kh_val(r->dh, k)]){
                    i->lt->recovery_status = NONE;
                    m->command = ri.command;
                    m->ballot = i->ballot;
                    m->type = PHASE1;
                    progress(r, i, m);
                }
            }
        }
        if(i->lt->try_pre_accept_oks > N/2){
            int rv;
            khint_t k = kh_put(deferred, r->dh, dh_key(m->conflict.instance_id,
                        m->conflict.replica_id), &rv);
            kh_val(r->dh, k) = m->id.replica_id;
            init_timeout(i);
        }

    } else {
        ++i->lt->pre_accept_oks;
        ++i->lt->try_pre_accept_oks;
        if(i->lt->pre_accept_oks >= N/2){
            i->command = m->command = ri.command;
            memcpy(i->deps, ri.deps, DEPSIZE);
            memcpy(m->deps, ri.deps, DEPSIZE);
            i->seq = m->seq = ri.seq;
            i->status = m->instance_status = ACCEPTED;
            i->lt->accept_oks = 0;
            i->lt->recovery_status = NONE;
            init_timeout(i);
            send_accept(r, m);
        }
    }
}

//TODO checkpointing (commit short/long)..
void commit(struct replica *r, struct instance *i, struct message *m){
    cancel_timeout(i);
    i->status = COMMITTED;
    send_exec(r, m);
    i->seq = m->seq;
    memcpy(i->deps, m->deps, DEPSIZE);
}

void progress(struct replica *r, struct instance *i, struct message *m){
    switch(m->type){
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
            try_pre_accept(r, i, m);
            break;
        case TRY_PRE_ACCEPT_REPLY:
            try_pre_accept_reply(r, i, m);
            break;
    }
}

void slow_path_timeout(struct replica *r, struct timer *t) {
    struct message *nm = message_from_instance(t->instance);
    nm->type = PREPARE;
    recover(r, t->instance, nm);
}

void fast_path_timeout(struct replica *r, struct timer *t) {
    struct message *nm = message_from_instance(t->instance);
    init_timeout(t->instance);
    send_accept(r, nm);
}

int step(struct replica *r, struct message *m) {
    struct instance *i = find_instance(r, &m->id);
    int rg = 0;
    if(i == 0){
        i = instance_from_message(m);
        if((rg = register_instance(r, i)) && (rg < 0)) return -1;
        i->ticker.on = slow_path_timeout;
    }
    progress(r, i, m);
    return rg;
}

int propose(struct replica *r, struct message *m){
    m->type = PHASE1;
    struct instance *i = instance_from_message(m);
    if(i == 0) return -1;
    struct leader_tracker *lt = malloc(sizeof(struct leader_tracker));
    if(lt == 0) return -1;
    i->lt->ri.status = NONE;
    i->lt = lt;
    i->key.instance_id = max_local(r) + 1;
    i->key.replica_id = r->id;
    int rg = 0;
    if((rg = register_instance(r, i )) && (rg < 0)) return -1;
    progress(r, i, m);
    return rg;
}

void run(struct replica *r){
    int rc;
    r->running = 1;
    struct message *nm;
    struct chclause cls[] = {
        {CHRECV, r->chan_tick[1], &nm, MSG_SIZE},/** advance time **/
        {CHRECV, r->chan_ii[1], &nm, MSG_SIZE},/** churn **/
        {CHRECV, r->chan_propose[1], &nm, MSG_SIZE} /** new io **/
    };
    while(r->running && rc >= 0){
        rc = 0;
        rc = choose(cls, 3, -1);
        switch(rc){
            case 0:
                tick(r);
                break;
            case 1:
                rc = step(r, nm);
                break;
            case 2:
                rc = propose(r, nm);
                break;
        }
    }
    //rc = chsend(chan_s, &rc, 1, -1);
    r->running = 0;
    if(rc < 0){
        //totally fucked
        perror("error!");
        exit(-1);
    }
    return;
}
