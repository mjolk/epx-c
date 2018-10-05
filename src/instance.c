/**
 * File   : src/instance.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 03:36
 */

#include "instance.h"
#include "llrb-interval/llrb-interval.h"

int is_state(enum state state_a, enum state state_b){
    if((state_a & state_b) == state_b) return 1;
    return 0;
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
        if(deps1[i].id.instance_id != deps2[i].id.instance_id){
            return 0;
        }
        if(deps1[i].committed != deps2[i].committed){
            return 0;
        }
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

void update_recovery_instance(struct recovery_instance *ri, struct message *m){
    ri->command = m->command;
    memcpy(ri->deps, m->deps,
            sizeof(m->deps)/sizeof(m->deps[0]));
    ri->seq = m->seq;
    ri->status = m->instance_status;
}

int intcmp(struct instance *e1, struct instance *e2)
{
    return (e1->key.instance_id < e2->key.instance_id ? -1 : e1->key.instance_id > e2->key.instance_id);
}

int spcmp(struct span *e1, struct span *e2)
{
    return strncmp(e1->start_key, e2->start_key, 16);
}

LLRB_HEAD(span_tree, span) rt;
LLRB_PROTOTYPE(span_tree, span, entry, intcmp);
SLL_HEAD(span_group, span) ml;
LLRB_GENERATE(span_tree, span, entry, spcmp)
LLRB_RANGE_GROUP_GEN(span_tree, span, entry, span_group, next);


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
    t->time_out = time_out;
    t->elapsed = 0;
}

void merge(struct span *to_merge, struct span_group *sll){
    struct span *c, *prev, *next;
    next = SLL_FIRST(sll);
    SLL_INSERT_HEAD(sll, to_merge, next);
    if(!next) return;
    prev = SLL_FIRST(sll);
    while(next) {
        c = next;
        next = SLL_NEXT(next, next);
        if(overlaps(to_merge, c)){
            if((strncmp(to_merge->start_key, c->start_key, KEY_SIZE) > 0)){
                strncpy(to_merge->start_key, c->start_key, KEY_SIZE);
            }
            if(strncmp(to_merge->end_key, c->end_key, KEY_SIZE) < 0) {
                strncpy(to_merge->end_key, c->end_key, KEY_SIZE);
            }
            SLL_REMOVE_AFTER(prev, next);
            LLRB_DELETE(span_tree, &rt, c);
            free(c);
        } else {
            prev = c;
        }
    }
}

//TODO check this shit, prolly segfaults
void delete_span_group(){
    struct span *del;
    LLRB_FOREACH(del, span_tree, &rt) {
        LLRB_DELETE(span_tree, &rt, del);
        free(del);
    }
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

uint64_t seq_deps_for_command(
        struct instance_index *index,
        struct command *c,
        struct instance_id *ignore,
        struct seq_deps_probe *probe
        ){
    uint64_t mseq = 0;
    SLL_INIT(&ml);
    LLRB_INIT(&rt);
    struct instance *ci;
    struct instance *ti;
    ci = LLRB_MAX(instance_index, index);
    while(ci){
        ti = ci;
        ci = LLRB_PREV(instance_index, index, ci);
        if((ignore) && is_instance_id(&ti->key, ignore)){
            continue;
        }
        if(is_state(ti->status, EXECUTED)){
            break;
        }
        if(interferes(ti->command, c)){
            mseq = max_seq(mseq, ti->seq);
            if(ti->command->writing) {
                if(LLRB_RANGE_GROUP_ADD(span_tree, &rt,
                            &ti->command->span, &ml, &merge)){
                    probe->updated = add_dep(probe->deps, ti);
                    if(!SLL_NEXT(SLL_FIRST(&ml), next) &&
                            encloses(SLL_FIRST(&ml), &c->span)){
                        break;
                    }
                }
            } else if(!LLRB_RANGE_OVERLAPS(span_tree, &rt, &ti->command->span)){
                probe->updated = add_dep(probe->deps, ti);
            }
        }
    }
    delete_span_group();
    return mseq;
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
        i->lt->ri.status = NONE;
    }
}
