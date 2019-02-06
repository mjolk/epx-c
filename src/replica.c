/**
 * File   : replica.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 20:21
 */

#include "replica.h"
#include <stdio.h>

int send_io(struct replica *r, struct message *m){
    int rc = chsend(r->chan_io[1], &m, MSG_SIZE, 10);
    if(rc < 0){
        return -1;
    }
    return rc;
}

int send_eo(struct replica *r, struct message *m){
    int rc = chsend(r->chan_eo[1], &m, MSG_SIZE, 10);
    if(rc < 0){
        return -1;
    }
    return rc;
}

int send_exec(struct replica *r, struct message *m){
    int rc = chsend(r->chan_exec[1], &m, MSG_SIZE, 10);
    if(rc < 0){
        return -1;
    }
    return rc;
}

int new_replica(struct replica *r){
    r->running = 0;
    if(chmake(r->chan_tick) != 0) goto error;
    if(chmake(r->chan_propose) !=0) goto error;
    if(chmake(r->chan_ii) !=0) goto error;
    if(chmake(r->chan_io) !=0) goto error;
    if(chmake(r->chan_eo) !=0) goto error;
    if(chmake(r->chan_exec) !=0) goto error;
    r->dh = kh_init(deferred);
    SIMPLEQ_INIT(&r->timers);
    for(int i = 0;i < N;i++){
        LLRB_INIT(&r->index[i]);
    }
    return 0;
error:
    perror("problem creating replica\n");
    return -1;
}

void destroy_replica(struct replica *r){
    r->running = 0;
    hclose(r->chan_tick[0]);
    hclose(r->chan_tick[1]);
    hclose(r->chan_propose[0]);
    hclose(r->chan_propose[1]);
    hclose(r->chan_ii[0]);
    hclose(r->chan_ii[1]);
    hclose(r->chan_io[0]);
    hclose(r->chan_io[1]);
    hclose(r->chan_eo[0]);
    hclose(r->chan_eo[1]);
    hclose(r->chan_exec[0]);
    hclose(r->chan_exec[1]);
    kh_destroy(deferred, r->dh);
    for(int i = 0;i < N;i++){
        LLRB_DESTROY(instance_index, &r->index[i], destroy_instance);
    }
}

int trigger(struct timer *t){
    return (t->elapsed >= t->time_out);
}

void tick(struct replica *r) {
    struct timer *c, *prev, *nxt;
    nxt = SIMPLEQ_FIRST(&r->timers);
    if(!nxt) return;
    while(nxt){
        if(!nxt->paused)nxt->elapsed++;
        c = nxt;
        nxt = SIMPLEQ_NEXT(nxt, next);
        if(trigger(c) && c->on) {
            c->on(r, c);
        }
        //recheck status since timer cfg might have changed
        if(trigger(c)) {
            if(prev){
                SIMPLEQ_REMOVE_AFTER(&r->timers, prev, next);
            } else {
                SIMPLEQ_REMOVE_HEAD(&r->timers, next);
                prev = 0;
            }
            free(c);
        } else {
            prev = c;
        }
    }
}

void register_timer(struct replica *r, struct instance *i, int time_out) {
    i->ticker.time_out = time_out;
    i->ticker.elapsed = 0;
    i->ticker.instance = i;
    SIMPLEQ_INSERT_TAIL(&r->timers, &i->ticker, next);
}

int register_instance(struct replica *r, struct instance *i) {
    register_timer(r, i, 2);
    struct instance *rej = LLRB_INSERT(instance_index,
            &r->index[i->key.replica_id], i);
    if(rej){
        //instance already exists, cannot create
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
        if(c == 0){
            noop_deps(&r->index[rc], rc, p->deps);
        }
        mseq = max_seq(mseq, seq_deps_for_command(&r->index[rc], c, ignore, p));
    }
    return mseq;
}

struct instance* pac_conflict(struct replica *r, struct instance *i,
        struct command *c, uint64_t seq, struct dependency *deps){
    if(i->command){
        if(i->status >= ACCEPTED){
            return i;
        } else if(i->seq == seq && equal_deps(i->deps, deps)){
            return 0;
        }
    }
    struct instance *conflict;
    for(int rc = 0;rc < N;rc++){
        conflict = pre_accept_conflict(&r->index[rc], i, c, seq, deps);
        if(conflict){
            return conflict;
        }
    }
    return 0;
}

uint64_t max_local(struct replica *r) {
    struct instance *i = LLRB_MAX(instance_index, &r->index[r->id]);
    if(i == 0) return 0;
    return i->key.instance_id;
}
