/**
 * File   : src/replica.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 20:21
 */

#include "replica.h"
#include <stdio.h>

struct replica* replica_new(){
    struct replica *r = malloc(sizeof(struct replica));
    if(r == 0) return 0;
    r->running = 0;
    if(chmake(r->chan_tick) != 0) goto error;
    if(chmake(r->chan_propose) !=0) goto error;
    if(chmake(r->chan_ii) !=0) goto error;
    if(chmake(r->chan_io) !=0) goto error;
    if(chmake(r->chan_eo) !=0) goto error;
    return r;
error:
    perror("problem creating channels\n");
    return 0;
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
        if(trigger(c) && c->instance->on) {
            c->instance->on(r, c);
        }
        //recheck status since timer cfg might have changed
        if(trigger(c)) {
            if(prev){
                SIMPLEQ_REMOVE_AFTER(&r->timers, prev, next);
            } else {
                SIMPLEQ_REMOVE_HEAD(&r->timers, next);
                prev = 0;
            }
            SIMPLEQ_NEXT(c, next) = 0;
            c->instance = 0;
            free(c);
        } else {
            prev = c;
        }
    }
}

int register_timer(struct replica *r, struct instance *i, int time_out) {
    struct timer *timer = malloc(sizeof(struct timer));
    if(timer == 0) return -1;
    timer->time_out = time_out;
    timer->elapsed = 0;
    timer->instance = i;
    SIMPLEQ_INSERT_TAIL(&r->timers, timer, next);
    return 0;
}

int register_instance(struct replica *r, struct instance *i) {
    int t = register_timer(r, i, 2);
    if(t != 0) return -1;
    LLRB_INSERT(instance_index, &r->index[i->key.replica_id], i);
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
    uint64_t mseq;
    for(int rc = 0;rc < N;rc++) {
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
    return LLRB_MAX(instance_index, &r->index[r->id])->key.instance_id;
}
