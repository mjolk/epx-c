/**
 * File   : src/instance.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 03:36
 */

#include "instance.h"

int is_state(struct instance *i, uint8_t state){
    if((i->status & state) == state) return 1;
    return 0;
}

int is_initial_ballot(uint8_t ballot) {
    return (ballot >> 4) == 0;
}

struct instance *instance_from_message(struct message *m) {
    struct instance *i = malloc(sizeof(struct instance));
    if(i == 0) return 0;
    i->command = m->command;
    i->key = m->id;
    i->start_key = m->command->span.start_key;
    i->end_key = m->command->span.end_key;
    i->seq = m->seq;
    i->max = 0;
    i->status = NONE;
    return i;
}

int intcmp(struct instance *e1, struct instance *e2)
{
    return (e1->start_key < e2->start_key ? -1 : e1->start_key > e2->start_key);
}

int spcmp(struct span *e1, struct span *e2)
{
    return (e1->start_key < e2->start_key ? -1 : e1->start_key > e2->start_key);
}

LLRB_HEAD(span_tree, span) rt;
LLRB_PROTOTYPE(span_tree, span, entry, intcmp);
SLL_HEAD(span_group, span) ml;
LLRB_GENERATE(instance_index, instance, entry, intcmp)
LLRB_GENERATE(span_tree, span, entry, spcmp)
LLRB_RANGE_GROUP_GEN(span_tree, span, entry, span_group, next);


int update_deps(struct dependency **deps, struct instance* i) {
    int updated = 0;
    for(int dc = 0; dc < N;dc++){
        if(deps[dc]->id.replica_id == i->key.replica_id &&
                deps[dc]->id.instance_id <= i->key.instance_id){
            deps[dc]->id = i->key;
            deps[dc]->committed = (i->status >= COMMITTED);
            updated = 1;
        }
    }
    return updated;
}

void timer_cancel(struct timer *t){
    t->elapsed = t->time_out;
}

void timer_set(struct timer *t ,int time_out) {
    t->time_out = time_out;
    t->elapsed = 0;
}

void merge(struct span *to_merge, struct span_group *sll) {
    struct span *c, *prev, *next;
    next = SLL_FIRST(sll);
    SLL_INSERT_HEAD(sll, to_merge, next);
    if(!next) return;
    prev = SLL_FIRST(sll);
    while(next) {
        c = next;
        next = SLL_NEXT(next, next);
        if(overlaps(to_merge, c)){
            if(to_merge->start_key > c->start_key){
                to_merge->start_key = c->start_key;
            }
            if(to_merge->end_key < c->end_key) {
                to_merge->end_key = c->end_key;
            }
            SLL_REMOVE_AFTER(prev, next);
            LLRB_DELETE(span_tree, &rt, c);
        } else {
            prev = c;
        }
    }
}

void delete_span_group() {
    struct span *del;
    LLRB_FOREACH(del, span_tree, &rt) {
        LLRB_DELETE(span_tree, &rt, del);
        free(del);
    }
}

int is_committed(struct instance *i){
    return (i->status == EXECUTED || i->status == COMMITTED);
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
        if(interferes(ti->command, c)){
            mseq = max_seq(mseq, ti->seq);
            if(ti->command->writing) {
                if(LLRB_RANGE_GROUP_ADD(span_tree, &rt,
                            &ti->command->span, &ml, &merge)){
                    probe->updated = update_deps(probe->deps, ti);
                    if(!SLL_NEXT(SLL_FIRST(&ml), next) &&
                            encloses(SLL_FIRST(&ml), &c->span)) {
                        break;
                    }
                }
            } else if(!LLRB_RANGE_OVERLAPS(span_tree, &rt, &ti->command->span)) {
                probe->updated = update_deps(probe->deps, ti);
            }
        }
    }
    delete_span_group();
    return mseq;
}

void instance_reset(struct instance *i) {
    for(int d = 0;d < N;d++){
        i->deps[d] = 0;
    }
    i->accept_oks = 0;
    i->prepare_oks = 0;
    i->nacks = 0;
    i->pre_accept_oks = 0;
    i->equal = 0;
}
