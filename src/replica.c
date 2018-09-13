/**
 * File   : src/replica.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 20:21
 */

#include "replica.h"

void replica_tick(struct replica *r) {
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

int replica_register_timer(struct replica *r, int time_out, struct instance *i) {
    struct timer *timer;
    if((timer = malloc(sizeof(struct timer))) == NULL)
       return -1;
    timer->time_out = time_out;
    timer->instance = i;
    SLL_INSERT_HEAD(&r->timers, timer, next);
    return 0;
}

void timer_cancel(struct timer *t){
    t->elapsed = t->time_out;
}

void timer_set(struct timer *t ,int time_out) {
    t->time_out = time_out;
    t->elapsed = 0;
}

struct instance* replica_find_instance(struct replica *r, struct instance *i) {
    return LLRB_FIND(instance_index, &r->index[i->id->replica_id], i);
}

