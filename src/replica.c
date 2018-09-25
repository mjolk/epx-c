/**
 * File   : src/replica.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 20:21
 */

#include "replica.h"

struct replica* replica_init(){
    struct replica *r = malloc(sizeof(struct replica));
    if(r == 0) return 0;
    r->running = 0;
    r->crank = buf_init();
    r->sink = buf_init();
    r->exec = buf_init();
    return r;
}

void tick(struct replica *r) {
    struct timer *c, *prev, *nxt;
    nxt = SLL_FIRST(&r->timers);
    if(!nxt) return;
    while(nxt){
        if(!nxt->paused)nxt->elapsed++;
        c = nxt;
        nxt = SLL_NEXT(nxt, next);
        int exec = (c->elapsed >= c->time_out);
        if(exec > 0 && c->instance->on) {
            c->instance->on(c);
        }
        //recheck status since timer cfg might have changed
        if(exec > 0) {
            if(prev){
                SLL_REMOVE_AFTER(prev, next);
            } else {
                SLL_REMOVE_HEAD(&r->timers, next);
                prev = 0;
            }
            SLL_NEXT(c, next) = 0;
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
    SLL_INSERT_HEAD(&r->timers, timer, next);
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
        .start_key = instance_id->instance_id
    };
    return LLRB_FIND(instance_index, &r->index[instance_id->replica_id], &i);
}

uint64_t sd_for_command(struct replica *r, struct command *c,
        struct instance_id *ignore, struct seq_deps_probe *p) {
    uint64_t mseq;
    for(int rc = 0;rc < N;rc++) {
        mseq = max_seq(mseq, seq_deps_for_command(&r->index[rc], c, ignore, p));
    }
    return 0;
}

uint64_t max_local(struct replica *r) {
    return LLRB_MAX(instance_index, &r->index[r->id])->start_key;
}

