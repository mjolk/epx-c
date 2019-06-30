/**
 * File   : replica.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 20:21
 */

#include "replica.h"
#include <stdio.h>

coroutine void tcl(struct replica *r){
    uint8_t t = 1;
    while(r->running){
        chsend(r->chan_tick[0], &t, sizeof(uint8_t), r->frequency);
        msleep(now() + r->frequency);
    }
}

coroutine void recv_step(struct replica *r){
    struct message *m;
    while(r->running){
        if(!chan_recv_mpsc(&r->in->chan_step, &m)){
            yield();
            continue;
        }
        chsend(r->chan_ii[0], &m, MSG_SIZE, -1);
    }
}

coroutine void recv_propose(struct replica *r){
    struct message *m;
    while(r->running){
        if(!chan_recv_mpsc(&r->in->chan_propose, &m)){
            yield();
            continue;
        }
        chsend(r->chan_propose[0], &m, MSG_SIZE, -1);
    }
}

int send_io(struct replica *r, struct message *m){
    return chan_send_spsc(&r->out->chan_io, m);
}

int send_eo(struct replica *r, struct message *m){
    return chan_send_mpsc(&r->out->chan_eo, m);
}

int send_exec(struct replica *r, struct instance *i){
    return chan_send_spsc(&r->out->chan_exec, i);
}

void clear(struct replica *r){
    for(int i = 0;i < N;i++){
       clear_index(&r->index[i]);
    }
}

int new_replica(struct replica *r){
    r->running = 0;
    int err;
    r->timers = timeouts_open(TIMEOUT_mHZ, &err);
    timeout_init(&r->timer, 0);
    r->timer.interval = r->frequency<<N;
    timeout_setcb(&r->timer, clear, r);
    if(err) goto error;
    if(chmake(r->chan_tick) != 0) goto error;
    if(chmake(r->chan_propose) !=0) goto error;
    if(chmake(r->chan_ii) !=0) goto error;
    r->dh = kh_init(deferred);
    for(int i = 0;i < N;i++){
        LLRB_INIT(&r->index[i]);
    }
    r->ap = bundle();
    return 0;
error:
    perror("problem creating replica\n");
    return -1;
}

int run_replica(struct replica *r){
    int rc = 0;
    r->running = 1;
    if(bundle_go(r->ap, tcl(r)) < 0) goto on_error;
    if(bundle_go(r->ap, recv_propose(r)) < 0) goto on_error;
    if(bundle_go(r->ap, recv_step(r)) < 0) goto on_error;
    timeouts_add(r->timers, &r->timer, r->frequency<<N);
    return rc;
on_error:
    r->running = 0;
    return -1;
}

void destroy_replica(void* rep){
    struct replica *r = (struct replica*)rep;
    timeouts_close(r->timers);
    r->running = 0;
    hclose(r->ap);
    hclose(r->chan_tick[0]);
    hclose(r->chan_tick[1]);
    hclose(r->chan_propose[0]);
    hclose(r->chan_propose[1]);
    hclose(r->chan_ii[0]);
    hclose(r->chan_ii[1]);
    kh_destroy(deferred, r->dh);
    for(int i = 0;i < N;i++){
        LLRB_DESTROY(instance_index, &r->index[i], destroy_instance);
    }
}

void tick(struct replica *r) {
   timeouts_step(r->timers, 1);
   struct timeout *t;
   while (NULL != (t = timeouts_get(r->timers))) {
       t->callback.fn(t->callback.arg);
   }
}

int register_instance(struct replica *r, struct instance *i) {
    i->r = r;
    struct instance *rej = LLRB_INSERT(instance_index,
            &r->index[i->key.replica_id], i);
    if(rej){
        //instance already exists, cannot create
        //first step is lookup so we should in theory never get here.
        destroy_instance(i);
        return -1;
    }
    return 0;
}

struct instance* find_instance(struct replica *r,
        struct instance_id *instance_id) {
    struct instance i = {
        .key = *instance_id
    };
    return LLRB_FIND(instance_index, &r->index[instance_id->replica_id], &i);
}

uint64_t sd_for_command(struct replica *r, struct command *c,
        struct instance_id *ignore, struct seq_deps_probe *p) {
    uint64_t mseq = 0;
    for(size_t rc = 0;rc < N;rc++) {
        mseq = max_seq(mseq, seq_deps_for_command(&r->index[rc], c, ignore, p));
    }
    return mseq;
}

void noop(struct replica *r, struct instance_id *id, struct dependency *deps){
    noop_dep(&r->index[id->replica_id], id, deps);
}

void barrier(struct replica *r, struct dependency *deps){
    memset(deps, 0, DEPSIZE);
    for(int i = 0;i < N;i++){
        barrier_dep(&r->index[i], deps);
    }
}

struct instance* pac_conflict(struct replica *r, struct instance *i,
        struct command *c, uint64_t seq, struct dependency *deps){
    struct instance *conflict = NULL;
    for(int rc = 0;rc < N;rc++){
        conflict = pre_accept_conflict(&r->index[rc], i, c, seq, deps);
        if(conflict) return conflict;
    }
    return 0;
}

uint64_t max_local(struct replica *r) {
    struct instance *i = LLRB_MAX(instance_index, &r->index[r->id]);
    if(!i) return 0;
    return i->key.instance_id;
}
