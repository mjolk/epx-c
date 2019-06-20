/**
 * File   : epaxos.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 03:39
 */

#include "epaxos.h"
#include <stdio.h>

struct instance* find_conflict(struct replica *r, struct instance *i, 
    struct command *c, uint64_t seq, struct dependency *deps){
    if(i->command){
        if(lt_eq_state(i, ACCEPTED)){
            return i;
        } else if(i->seq == seq && equal_deps(i->deps, deps)){
            return 0;
        }
    }
    return pac_conflict(r, i, c, seq, deps);
}

struct leader_tracker *new_tracker(){
    struct leader_tracker *lt = (struct leader_tracker*)calloc(1, 
        sizeof(struct leader_tracker));
    if(!lt){ errno = ENOMEM; return 0;}
    lt->ri.status = NONE;
    lt->recovery_status = NONE;
    return lt;
}

uint64_t dh_key(struct instance_id *id){
    return (id->instance_id << 16) | id->replica_id;
}

void cancel_timeout(struct instance *i){
    i->ticker.on = 0;
    timer_cancel(&i->ticker);
}

void fast_path_timeout(struct replica*, struct timer*);
void slow_path_timeout(struct replica*, struct timer*);
void checkpoint_timeout(struct replica*, struct timer*);
void init_timeout(struct instance *i){
    timer_set(&i->ticker, 2);
    i->ticker.on = slow_path_timeout;
}

void init_fp_timeout(struct instance *i){
    timer_set(&i->ticker, 2);
    i->ticker.on = fast_path_timeout;
}

static inline void do_commit(struct replica *r, struct instance *i,
        struct message *m){
    cancel_timeout(i);
    set_state(i, COMMITTED);
    m->type = COMMIT;
    send_exec(r, i);
    m->type = COMMIT;
    send_io(r, m);
    if(i->command->writing){
        send_eo(r, m);
    }
}

void progress(struct replica *r, struct instance*, struct message*);

void checkpoint(struct replica *r, struct message *m){
    struct instance *i = new_instance();
    if(!i) return;
    i->lt = new_tracker();
    if(!i->lt) return;
    i->key.instance_id = max_local(r) + 1;
    i->key.replica_id = r->id;
    if(register_instance(r, i ) < 0) return;
    barrier(r, i->deps);
    if(!m){
        m = (struct message*)malloc(sizeof(struct message));
    }
    m->id = i->key;
    memcpy(m->deps, i->deps, DEPSIZE);
    m->ballot = i->ballot;
    set_state(i, PRE_ACCEPTED);
    m->type = PRE_ACCEPT;
    init_fp_timeout(i);
    send_io(r, m);
}

void checkpoint_timeout(struct replica *r, struct timer *t){
    checkpoint(r, 0);
    timer_set(t, r->checkpoint<<N);
}

void recover(struct replica *r, struct instance *i, struct message *m) {
    if(!i->lt){
        i->lt = new_tracker();
        if(!i->lt) return;
    }
    i->lt->recovery_status = PREPARING;
    if(is_state(i, ACCEPTED)){
        i->lt->max_ballot = i->ballot;
    } else if(is_state(i, PRE_ACCEPTED)){
        i->lt->ri.pre_accept_count = 1;
        i->lt->ri.leader_replied = (r->id == i->key.replica_id);
    }
    i->lt->ri.status = get_state(i);
    i->lt->ri.command = i->command;
    i->lt->ri.seq = i->seq;
    memcpy(i->lt->ri.deps, i->deps, DEPSIZE);
    i->ballot = larger_ballot(i->ballot, r->id);
    m->ballot = i->ballot;
    m->id = i->key;
    m->command = i->command;
    m->seq = i->seq;
    m->type = PREPARE;
    init_timeout(i);
    send_io(r, m);
}

void phase1(struct replica *r, struct instance *i, struct message *m){
    instance_reset(i);
    //TODO add updated field somewhere, use lt.. should remove this probe struct
    struct seq_deps_probe p = {
        .updated = 0,
    };
    //memcpy(p.deps, m->deps, DEPSIZE);
    i->seq = sd_for_command(r, m->command, &i->key, &p) + 1;
    set_state(i, PRE_ACCEPTED);
    i->lt->equal = (p.updated == 0);
    memcpy(i->deps, p.deps, DEPSIZE);
    m->seq = i->seq;
    i->command = m->command;
    memcpy(m->deps, i->deps, DEPSIZE);
    i->ballot = m->ballot;
    m->id = i->key;
    m->type = PRE_ACCEPT;
    init_fp_timeout(i);
    send_io(r, m);
}

void pre_accept(struct replica *r, struct instance *i, struct message *m){
    if(is_state(i, (enum state)(COMMITTED | ACCEPTED))) return; //TODO sync to stable storage
    if(!is_state(i, (enum state)(PRE_ACCEPTED | NONE))) return;
    if(m->ballot < i->ballot){
        m->ballot = i->ballot;
        m->command = i->command;
        m->seq = i->seq;
        m->nack = 1;
        memcpy(m->deps, i->deps, DEPSIZE);
        m->type = PRE_ACCEPT_REPLY;
        send_io(r, m);
        return;
    }
    struct seq_deps_probe p = {
        .updated = 0
    };
    memcpy(p.deps, i->deps, DEPSIZE);
    uint64_t nseq = max_seq(m->seq, (sd_for_command(r, m->command, &i->key,
                    &p) + 1));
    set_state(i, PRE_ACCEPTED);
    if(p.updated == 0){
        set_state(i, PRE_ACCEPTED_EQ);
    }
    i->command = m->command;
    i->ballot = m->ballot;
    memcpy(i->deps, p.deps, DEPSIZE);
    i->seq = nseq;
    if(i->command == 0){
        clear(r);
    }
    //TODO sync to stable storage
    if(i->seq != m->seq || (p.updated > 0) ||
            has_uncommitted_deps(i) || !is_initial_ballot(m->ballot)){
        m->seq = nseq;
        memcpy(m->deps, i->deps, DEPSIZE);
        m->type = PRE_ACCEPT_REPLY;
        send_io(r, m);
        return;
    }
    m->type = PRE_ACCEPT_OK;
    send_io(r, m);
}

void pre_accept_ok(struct replica *r, struct instance *i, struct message *m){
    if(!is_state(i, PRE_ACCEPTED) ||
            !is_initial_ballot(i->ballot)) return;
    ++i->lt->pre_accept_oks;
    if(i->lt->pre_accept_oks >= (N/2) && i->lt->equal &&
            !has_uncommitted_deps(i)){
        do_commit(r, i, m);
    } else if(i->lt->pre_accept_oks >= (N/2)){
        set_state(i, ACCEPTED);
        m->type = ACCEPT;
        send_io(r, m);
    }
}

void pre_accept_reply(struct replica *r, struct instance *i, struct message *m){
    if(!is_state(i, PRE_ACCEPTED) || i->ballot != m->ballot) return;
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
    if(m->seq > i->seq){
        i->seq = m->seq;
        i->lt->equal = 1;
    }
    for(int ds = 0; ds < MAX_DEPS; ds++){
        i->lt->equal -= update_deps(i->deps, &m->deps[ds]);
    }
    i->lt->equal = (i->lt->equal == 1);
    if((i->lt->pre_accept_oks >= N/2) && i->lt->equal
            && is_initial_ballot(i->ballot) && !has_uncommitted_deps(i)){
        do_commit(r, i, m);
        return;
    } else if(i->lt->pre_accept_oks >= (N/2)){
        set_state(i, ACCEPTED);
        m->type = ACCEPT;
        send_io(r, m);
    }
}

//TODO add checkpointing
static void accept(struct replica *r, struct instance *i, struct message *m){
    if(is_state(i, (enum state)(COMMITTED | EXECUTED))) return;
    if(m->ballot < i->ballot){
        m->nack = 1;
        m->ballot = i->ballot;
        m->type = ACCEPT_REPLY;
        send_io(r, m);
        return;
    }
    set_state(i, ACCEPTED);
    i->ballot = m->ballot;
    i->seq = m->seq;
    memcpy(i->deps, m->deps, DEPSIZE);
    if(m->command == 0){
        clear(r);
    }
    m->type = ACCEPT_REPLY;
    send_io(r, m);
}

void accept_reply(struct replica *r, struct instance *i, struct message *m){
    if(!is_state(i, ACCEPTED) || i->ballot != m->ballot) return;
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
    if(i->lt->accept_oks+1 > N/2){
        do_commit(r, i, m);
    }
}

void prepare(struct replica *r, struct instance *i, struct message *m){
    if(m->ballot < i->ballot){
        m->nack = 1;
        m->type = PREPARE_REPLY;
        send_io(r, m);
        return;
    } else {
        i->ballot = m->ballot;
    }
    m->command = i->command;
    memcpy(m->deps, i->deps, DEPSIZE);
    m->seq = i->seq;
    m->ballot = i->ballot;
    m->instance_status = get_state(i);
    m->type = PREPARE_REPLY;
    send_io(r, m);
}

void prepare_reply(struct replica *r, struct instance *i, struct message *m){
    if(!(i->lt) || i->lt->recovery_status != PREPARING) return;
    if(m->nack){
        i->lt->nacks++;
        return;
    }
    ++i->lt->prepare_oks;
    if(is_sstate(m->instance_status, (enum state)(COMMITTED | EXECUTED))){
        i->seq = m->seq;
        i->command = m->command;
        m->ballot = i->ballot;
        memcpy(i->deps, m->deps, DEPSIZE);
        do_commit(r, i, m);
        return;
    }
    if(is_sstate(m->instance_status, ACCEPTED)){
        if(i->lt->max_ballot < m->ballot || is_sstate(i->lt->ri.status, NONE)){
            i->lt->max_ballot = m->ballot;
            update_recovery_instance(&i->lt->ri, m);
            i->lt->ri.pre_accept_count++;
        }
    }
    if(is_sstate(m->instance_status, (enum state)(PRE_ACCEPTED | PRE_ACCEPTED_EQ)) &&
            (i->lt->ri.status < ACCEPTED)){
        if(is_sstate(i->lt->ri.status, NONE) ||
                is_sstate(m->instance_status, PRE_ACCEPTED_EQ)){
            update_recovery_instance(&i->lt->ri, m);
            i->lt->ri.pre_accept_count++;
        } else if((m->seq == i->seq) && equal_deps(m->deps, i->deps)){
            i->lt->ri.pre_accept_count++;
        }
        if(m->from == m->id.replica_id){
            i->lt->ri.leader_replied = 1;
        }
    }
    if(i->lt->prepare_oks < N/2) return;
    struct recovery_instance ri = i->lt->ri;
    if(is_sstate(ri.status, NONE)){
        noop(r, &m->id, m->deps);
        i->lt->recovery_status = NONE;
        set_state(i, ACCEPTED);
        memcpy(i->deps, m->deps, DEPSIZE);
        m->command = 0;
        m->ballot = i->ballot;
        m->type = ACCEPT;
        send_io(r, m);
        return;
    }
    if(is_sstate(ri.status, ACCEPTED) || ((ri.leader_replied == 0) &&
                (ri.pre_accept_count >= N/2 &&
                 is_sstate(ri.status, PRE_ACCEPTED_EQ)))){
        i->command = ri.command;
        i->seq = ri.seq;
        memcpy(i->deps, ri.deps, DEPSIZE);
        set_state(i, ACCEPTED);
        i->lt->recovery_status = NONE;
        m->ballot = i->ballot;//i->lt->max_ballot;
        memcpy(m->deps, i->deps, DEPSIZE);
        m->seq = i->seq;
        m->type = ACCEPT;
        send_io(r, m);
    } else if((ri.leader_replied == 0) && ri.pre_accept_count >= (N/2+1)/2){
        i->lt->pre_accept_oks = 0;
        i->lt->nacks = 0;
        for(int rc = 0;rc<N;rc++){
            i->lt->quorum[rc] = 1;
        }
        struct instance *conflict = pac_conflict(r, i, ri.command, ri.seq,
                ri.deps);
        if(conflict){
            if(is_committed(conflict)){
                m->type = PHASE1;
                m->command = ri.command;
                m->ballot = i->ballot;
                progress(r, i, m);
                return;
            } else {
                i->lt->nacks = 0;
                i->lt->quorum[r->id] = 0;
            }
        } else {
            i->command = ri.command;
            i->seq = ri.seq;
            memcpy(i->deps, ri.deps, DEPSIZE);
            set_state(i, PRE_ACCEPTED);
            i->lt->pre_accept_oks = 1;
        }
        i->lt->recovery_status = TRY_PRE_ACCEPTING;
        m->command = i->command;
        memcpy(m->deps, i->deps, DEPSIZE);
        m->seq = i->seq;
        m->ballot = i->ballot;
        m->type = TRY_PRE_ACCEPT;
        send_io(r, m);
    } else {
        i->lt->recovery_status = NONE;
        m->type = PHASE1;
        m->command = ri.command;
        m->ballot = i->ballot;
        progress(r, i, m);
    }
}

void try_pre_accept(struct replica *r, struct instance *i, struct message *m){
    if(i->ballot > m->ballot){
        m->conflict.id = m->id;
        m->conflict.status = get_state(i);
        m->ballot = i->ballot;
        m->nack = 1;
        m->type = TRY_PRE_ACCEPT_REPLY;
        send_io(r, m);
        return;
    }
    struct instance *conflict = find_conflict(r, i, m->command, m->seq,
            m->deps);
    if(conflict){
        m->conflict.id = conflict->key;
        m->conflict.status = conflict->status;
        m->ballot = i->ballot;
        m->nack = 1;
        m->type = TRY_PRE_ACCEPT_REPLY;
        send_io(r, m);
    } else {
        i->command = m->command;
        memcpy(i->deps, m->deps, DEPSIZE);
        i->ballot = m->ballot;
        i->seq = m->seq;
        set_state(i, PRE_ACCEPTED);
        m->type = TRY_PRE_ACCEPT_REPLY;
        send_io(r, m);
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
            i->ballot = larger_ballot(m->ballot, r->id);
            m->ballot = i->ballot;
            m->type = TRY_PRE_ACCEPT;
            send_io(r, m);
            return;
        }
        ++i->lt->try_pre_accept_oks;
        if(eq_instance_id(&m->conflict.id, &m->id)){
            i->lt->quorum[m->from] = 0;
            i->lt->quorum[m->conflict.id.replica_id] = 0;
            m->type = PREPARE;
            send_io(r, m);
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
            khint_t k = kh_get(deferred, r->dh, dh_key(&m->id));
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
            khint_t k = kh_put(deferred, r->dh, dh_key(&m->conflict.id), &rv);
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
            set_state(i, ACCEPTED);
            m->instance_status = ACCEPTED;
            i->lt->accept_oks = 0;
            i->lt->recovery_status = NONE;
            m->type = ACCEPT;
            send_io(r, m);
        }
    }
}

//TODO checkpointing (commit short/long)..
void commit(struct replica *r, struct instance *i, struct message *m){
    cancel_timeout(i);
    set_state(i, COMMITTED);
    send_exec(r, i);
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
        case EXECUTE:
            break;
    }
}

void slow_path_timeout(struct replica *r, struct timer *t) {
    //struct message *nm = message_from_instance(t->instance);
    struct message *nm = (struct message*)malloc(sizeof(struct message));
    nm->type = PREPARE;
    recover(r, t->instance, nm);
}

void fast_path_timeout(struct replica *r, struct timer *t) {
    struct message *m = message_from_instance(t->instance);
    init_timeout(t->instance);
    m->type = ACCEPT;
    send_io(r, m);
}

int step(struct replica *r, struct message *m) {
    if(m->id.instance_id == 0) return -1;
    struct instance *i = find_instance(r, &m->id);
    if(i == 0){
        i = new_instance();
        if(!i) return -1;
        i->key = m->id;
        if(register_instance(r, i) < 0) return -1;
        i->ticker.on = slow_path_timeout;
    }
    progress(r, i, m);
    return 0;
}

int propose(struct replica *r, struct message *m){
    m->type = PHASE1;
    struct instance *i = new_instance();
    if(!i) return -1;
    i->lt = new_tracker();
    if(!i->lt) return -1;
    i->key.instance_id = max_local(r) + 1;
    i->key.replica_id = r->id;
    if(register_instance(r, i ) < 0) return -1;
    progress(r, i, m);
    return 0;
}

void* run(void* rep){
    struct replica *r = (struct replica*)rep;
    pthread_cleanup_push(destroy_replica, r);
    if(new_replica(r) != 0) return 0;
    int rc = 0;
    int err = 0;
    if(r->checkpoint){
        r->ticker.time_out = r->checkpoint<<r->id;
        r->ticker.elapsed = 0;
        r->ticker.on = checkpoint_timeout;
    }
    struct message *m;
    struct chclause cls[] = {
        {CHRECV, r->chan_tick[1], &m, MSG_SIZE},/** advance time **/
        {CHRECV, r->chan_ii[1], &m, MSG_SIZE},/** churn **/
        {CHRECV, r->chan_propose[1], &m, MSG_SIZE} /** new io **/
    };
    if(run_replica(r) != 0) return 0;
    while(r->running && rc >= 0){
        rc = choose(cls, 3, -1);
        switch(rc){
            case 0:
                tick(r);
                break;
            case 1:
                err = step(r, m);
                break;
            case 2:
                err = propose(r, m);
                break;
        }
        if(err < 0){
            //TODO stop replica
            pthread_exit(NULL);
            return 0;
        }
    }
    pthread_cleanup_pop(0);
    return 0;
}