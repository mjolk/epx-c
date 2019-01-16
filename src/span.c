/**
 * File   : src/span.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : vr 04 jan 2019 19:51
 */

#include "span.h"
#include "llrb-interval/llrb-interval.h"

int spcmp(struct span *sp1, struct span *sp2){
    return strncmp(sp1->start_key, sp2->start_key, KEY_SIZE);
}

SLL_HEAD(span_group, span) ml;
LLRB_HEAD(span_tree, span) rt;
LLRB_GENERATE(span_tree, span, entry, spcmp);
LLRB_RANGE_GROUP_GEN(span_tree, span, entry, span_group, next);

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
