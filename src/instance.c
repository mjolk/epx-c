/**
 * File   : src/instance.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 03:36
 */

#include "instance.h"

struct message *message_from_instance(struct instance *i){
    struct message *nm = malloc(sizeof(struct message));
    if(nm == 0) return 0;
    nm->ballot = i->ballot;
    nm->command = i->command;
    nm->id = i->key;
    nm->seq = i->seq;
    memcpy(nm->deps, i->deps,
            size(i->deps));
    return nm;
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

int equal_deps(struct dependency *deps1, struct dependency *deps2){
    for(int i = 0; i < N; i++){
        if(deps1->id.instance_id != deps2->id.instance_id){
            return 0;
        }
        if(deps1->committed != deps2->committed){
            return 0;
        }
        deps1++;
        deps2++;
    }
    return 1;
}

struct instance *instance_from_message(struct message *m){
    struct instance *i = malloc(sizeof(struct instance));
    if(i == 0) return 0;
    i->command = m->command;
    i->key = m->id;
    i->seq = m->seq;
    i->status = NONE;
    return i;
}

void update_recovery_instance(struct recovery_instance *ri, struct message *m,
        int leader_replied, int pa_count){
    ri->command = m->command;
    memcpy(ri->deps, m->deps,
            sizeof(m->deps)/sizeof(m->deps[0]));
    ri->seq = m->seq;
    ri->status = m->instance_status;
    ri->leader_replied = leader_replied;
    ri->pre_accept_count = pa_count;
}

int intcmp(struct instance *e1, struct instance *e2)
{
    return (e1->key.instance_id < e2->key.instance_id ? -1 : e1->key.instance_id > e2->key.instance_id);
}


LLRB_GENERATE(instance_index, instance, entry, intcmp);

int update_deps(struct dependency *deps, struct dependency* d) {
    int updated = 0;
    if(deps[d->id.replica_id].id.instance_id == 0){
        deps[d->id.replica_id].id = d->id;
        deps[d->id.replica_id].committed = d->committed;
        updated++;
    } else if(deps[d->id.replica_id].id.instance_id <= d->id.instance_id){
        deps[d->id.replica_id].id = d->id;
        deps[d->id.replica_id].committed = d->committed;
        updated++;
    }
    return updated;
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
    return (i->status == COMMITTED);
}

int has_uncommitted_deps(struct instance *i){
    for(int dc = 0;dc < N;dc++){
        if(i->deps[dc].committed == 0){
            return 1;
        }
    }
    return 0;
}

void noop_deps(struct instance_index *index, struct dependency *dep) {
    struct instance *ti, *nxt;
    nxt = LLRB_MAX(instance_index, index);
    while(nxt){
        ti = nxt;
        nxt = LLRB_PREV(instance_index, index, nxt);
        if(ti->status >= COMMITTED &&
                ti->key.instance_id >= dep->id.instance_id){
           dep->committed = 1;
          return;
        }
    }
}

struct instance* pre_accept_conflict(struct instance_index *index,
        struct instance *i, struct command *c,
        uint64_t seq, struct dependency *deps){
    struct instance *ti;
    struct instance *nxt = LLRB_PREV(instance_index, index, i);
    while(nxt){
        ti = nxt;
        nxt = LLRB_PREV(instance_index, index, nxt);
        if(is_state(ti->status, EXECUTED)){
            break;
        }
        if(ti->key.instance_id == deps[ti->key.replica_id].id.instance_id){
            continue;
        }
        if(ti->command == 0){
            continue;
        }
        if(ti->deps[i->key.replica_id].id.instance_id >= i->key.instance_id){
            continue;
        }
        if(interferes(ti->command, c)){
            if((ti->key.instance_id > deps[ti->key.replica_id].id.instance_id) ||
                ((ti->key.instance_id < deps[ti->key.replica_id].id.instance_id) &&
                 (ti->seq >= seq) && ((ti->key.replica_id != i->key.replica_id) ||
                     ti->status > PRE_ACCEPTED_EQ))){
                return ti;
            }
         }

    }
    return 0;
}

void instance_reset(struct instance *i){
    for(int d = 0;d < N;d++){
        i->deps[d].committed = 0;
        i->deps[d].id.instance_id = 0;
    }
    if(i->lt){
        //TODO reset all counters
        i->lt->ri.status = NONE;
    }
}
