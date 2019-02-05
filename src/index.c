/**
 * File   : index.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : vr 04 jan 2019 19:51
 */

#include <stdio.h>
#include <inttypes.h>
#include "index.h"
#include "llrb-interval/llrb-interval.h"

int spcmp(struct span *sp1, struct span *sp2){
    return strcmp(sp1->start_key, sp2->start_key);
}

LLRB_HEAD(span_tree, span) rt;
LLRB_PROTOTYPE(span_tree, span, entry, spcmp)
SLL_HEAD(span_group, span) ml;
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
            if((strcmp(to_merge->start_key, c->start_key) > 0)){
                strcpy(to_merge->start_key, c->start_key);
            }
            if(strcmp(to_merge->end_key, c->end_key) < 0) {
                strcpy(to_merge->end_key, c->end_key);
            }
            SLL_REMOVE_AFTER(prev, next);
            LLRB_DELETE(span_tree, &rt, c);
        }else{
            prev = c;
        }
    }
}

uint64_t seq_deps_for_command(
        struct instance_index *index,
        struct command *cmd,
        struct instance_id *ignore,
        struct seq_deps_probe *probe
        ){
    uint64_t mseq = 0;
    SLL_INIT(&ml);
    LLRB_INIT(&rt);
    struct instance *ci, *ti;
    ci = LLRB_MAX(instance_index, index);
    while(ci){
        ti = ci;
        ci = LLRB_PREV(instance_index, index, ci);
        if((ignore) && eq_instance_id(&ti->key, ignore)){
            continue;
        }
        if(is_state(ti->status, EXECUTED)){
            break;
        }
        if(interferes(ti->command, cmd)){
            mseq = max_seq(mseq, ti->seq);
            if(ti->command->writing) {
                if(LLRB_RANGE_GROUP_ADD(span_tree, &rt,
                            &ti->command->span, &ml, merge)){
                    probe->updated += add_dep(probe->deps, ti);
                    struct span nsp = ti->command->span;
                    LLRB_INSERT(span_tree, &rt, &nsp);
                    if(!LLRB_RANGE_GROUP_ADD(span_tree, &rt, &cmd->span, &ml,
                                merge)){
                        break;
                    }
                }
            }else if(!LLRB_RANGE_OVERLAPS(span_tree, &rt, &ti->command->span)){
                probe->updated += add_dep(probe->deps, ti);
            }
        }
    }
    return mseq;
}

void noop_deps(struct instance_index *index, size_t replica_id,
        struct dependency *deps) {
    struct instance *ti, *nxt;
    int idx = lt_dep(replica_id, deps);
    nxt = LLRB_MAX(instance_index, index);
    while(nxt){
        ti = nxt;
        nxt = LLRB_PREV(instance_index, index, nxt);
        if(ti->status >= COMMITTED &&
                ti->key.instance_id >= deps[idx].id.instance_id){
            deps[idx].committed = 1;
            return;
        }
    }
}

struct instance* pre_accept_conflict(struct instance_index *index,
        struct instance *i, struct command *c,
        uint64_t seq, struct dependency *deps){
    struct instance *ti;
    struct instance *nxt = LLRB_FIND(instance_index, index, i);
    while(nxt){
        ti = nxt;
        nxt = LLRB_PREV(instance_index, index, nxt);
        if(is_state(ti->status, EXECUTED)){
            break;
        }
        if(has_key(&ti->key, deps)){
            continue;
        }
        if(ti->command == 0){
            continue;
        }
        if(ti->deps[lt_dep(i->key.replica_id, ti->deps)].id.instance_id >=
                i->key.instance_id){
            continue;
        }
        if(interferes(ti->command, c)){
            int didx = lt_dep(ti->key.replica_id, deps);
            if((ti->key.instance_id > deps[didx].id.instance_id) ||
                    ((ti->key.instance_id < deps[didx].id.instance_id) &&
                     (ti->seq >= seq) && ((ti->key.replica_id != i->key.replica_id) ||
                         ti->status > PRE_ACCEPTED_EQ))){
                return ti;
            }
        }

    }
    return 0;
}
