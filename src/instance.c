/**
 * File   : instance.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 03:36
 */

#include <stdio.h>
#include <inttypes.h>
#include "instance.h"
#include <errno.h>

struct message *message_from_instance(struct instance *i){
    struct message *nm = malloc(sizeof(struct message));
    if(nm == 0){ errno = ENOMEM; return 0;};
    nm->ballot = i->ballot;
    nm->command = i->command;
    nm->id = i->key;
    nm->seq = i->seq;
    memcpy(nm->deps, i->deps, DEPSIZE);
    return nm;
}

struct instance *instance_from_message(struct message *m){
    struct instance *i = malloc(sizeof(struct instance));
    if(i == 0){ errno = ENOMEM; return 0;};
    for(int ld = 0;ld < MAX_DEPS;ld++){
        i->deps[ld].id.instance_id = 0;
        i->deps[ld].id.replica_id = 0;
        i->deps[ld].committed = 0;
    }
    i->ballot = m->ballot;
    i->command = m->command;
    i->key = m->id;
    i->seq = m->seq;
    i->status = NONE;
    i->lt = 0;
    return i;
}

void destroy_instance(struct instance *i){
    if(i->command){
        free(i->command);
    }
    if(i->lt){
        free(i->lt);
    }
    free(i);
}

int is_state(enum state state_a, enum state state_b){
    return !((state_a & state_b) == 0);
}

uint8_t unique_ballot(uint8_t ballot, size_t replica_id){
    return (ballot << 4) | replica_id;
}

uint8_t larger_ballot(uint8_t ballot, size_t replica_id){
    return unique_ballot((ballot >> 4) +1, replica_id);
}

int is_initial_ballot(uint8_t ballot){
    return (ballot >> 4) == 0;
}

size_t replica_from_ballot(uint8_t ballot){
    return ballot & 15;
}

int lt_dep(size_t replica_id, struct dependency *deps){
    int idx = 0;
    uint64_t inst_id = 0;
    for(int i = 0;i < MAX_DEPS;i++){
        if(replica_id == deps->id.replica_id &&
                deps->id.instance_id > inst_id){
            inst_id = deps->id.instance_id;
            idx = i;
        }
    }
    return idx;
}

int has_key(struct instance_id *id, struct dependency *deps){
    for(int i =0;i < MAX_DEPS;i++){
        if(eq_instance_id(id, &deps[i].id)){
            return 1;
        }
    }
    return 0;
}

void sort_deps(struct dependency *deps){
    int j = 0;
    for(int i = 1;i < MAX_DEPS;i++){
        struct dependency d = deps[i];
        j = i - 1;
        while(j >= 0 && deps[j].id.instance_id < d.id.instance_id ){
            deps[j +1] = deps[j];
            j--;
        }
        deps[j+1] = d;
    }
}

int equal_deps(struct dependency *deps1, struct dependency *deps2){
    sort_deps(deps1);
    sort_deps(deps2);
    for(int i = 0; i < MAX_DEPS; i++){
        if(!eq_instance_id(&deps1->id, &deps2->id) ||
                deps1->committed != deps2->committed){
            return 0;
        }
        deps1++;
        deps2++;
    }
    return 1;
}

void update_recovery_instance(struct recovery_instance *ri, struct message *m,
        int leader_replied, int pa_count){
    ri->command = m->command;
    memcpy(ri->deps, m->deps, DEPSIZE);
    ri->seq = m->seq;
    ri->status = m->instance_status;
    ri->leader_replied = leader_replied;
    ri->pre_accept_count = pa_count;
}

int intcmp(struct instance *e1, struct instance *e2)
{
    return (e1->key.instance_id < e2->key.instance_id ? -1 :
            e1->key.instance_id > e2->key.instance_id);
}

LLRB_GENERATE(instance_index, instance, entry, intcmp);

int update_deps(struct dependency *deps, struct dependency* d) {
    if(d->id.instance_id == 0) return 0;
    int f_index = -1;
    for(int i = 0;i < MAX_DEPS;i++){
        if(deps[i].id.instance_id == 0 && f_index < 0){
            f_index = i;
            continue;
        }
        if(eq_instance_id(&deps[i].id, &d->id)){
            return 0;
        }
    }
    if(f_index < 0) return -1;
    deps[f_index].id = d->id;
    deps[f_index].committed = d->committed;
    return 1;
}

int add_dep(struct dependency *deps, struct instance *i){
    struct dependency dep = {.id = i->key,
        .committed = (i->status >= COMMITTED)};
    return update_deps(deps, &dep);
}

void timer_cancel(struct timer *t){
    t->elapsed = t->time_out;
}

void timer_set(struct timer *t ,int time_out){
    t->time_out += t->elapsed;
}

void timer_reset(struct timer *t, int time_out){
    t->time_out = time_out;
    t->elapsed = 0;
}

int is_committed(struct instance *i){
    return (i->status >= COMMITTED);
}

int has_uncommitted_deps(struct instance *i){
    for(int dc = 0;dc < MAX_DEPS;dc++){
        if(i->deps[dc].id.instance_id == 0) continue;
        if(i->deps[dc].committed == 0){
            return 1;
        }
    }
    return 0;
}

void instance_reset(struct instance *i){
    for(int d = 0;d < MAX_DEPS;d++){
        i->deps[d].committed = 0;
        i->deps[d].id.instance_id = 0;
    }
    if(i->lt){
        //TODO reset all counters
        i->lt->ri.status = NONE;
    }
}
