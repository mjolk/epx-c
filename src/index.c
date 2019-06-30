/**
 * File   : index.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : vr 04 jan 2019 19:51
 */

#include <stdio.h>
#include <inttypes.h>
#include "index.h"
#include "llrb-interval/slist.h"
#include "llrb-interval/llrb-interval.h"

int spcmp(struct span *sp1, struct span *sp2){
    return strcmp(sp1->start_key, sp2->start_key);
}

LLRB_HEAD(span_tree, span);
LLRB_PROTOTYPE(span_tree, span, entry, spcmp)
SLL_HEAD(span_group, span);
LLRB_GENERATE(span_tree, span, entry, spcmp);
LLRB_RANGE_GROUP_GEN(span_tree, span, entry, span_group, next);

void merge(struct span_tree *rt, struct span *to_merge, struct span_group *sll){
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
            LLRB_DELETE(span_tree, rt, c);
        }else{
            prev = c;
        }
    }
}

int read_dependency(struct span_tree *rt, struct command *cmd){
    int no_overlap = 1;
    for(size_t i = 0;i < cmd->tx_size;i++){
        if(LLRB_RANGE_OVERLAPS(span_tree, rt, &cmd->spans[i])){
            no_overlap = 0;
            break;
        }
    }
    return no_overlap;
}

uint64_t seq_deps_for_command(
    struct instance_index *index,
    struct command *cmd,
    struct instance_id *ignore,
    struct seq_deps_probe *probe
){
    uint64_t mseq = 0;
    struct span_group ml;
    struct span_tree rt;
    LLRB_INIT(&rt);
    struct span cspan[TX_SIZE];
    struct instance *ci, *ti;
    size_t i, j;
    int covered, add;
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
        memset(cspan, 0, sizeof(struct span)*TX_SIZE);
        if(interferes(ti->command, cmd, cspan)){
            mseq = max_seq(mseq, ti->seq);
            if(ti->command->writing) {
                covered = 0;
                add = 0;
                for(i = 0;i < ti->command->tx_size;i++){
                    if(empty_range(&cspan[i])) break;
                    if(LLRB_RANGE_GROUP_ADD(span_tree, &rt,
                                &cspan[i], &ml, merge)){
                        struct span nsp = cspan[i];
                        LLRB_INSERT(span_tree, &rt, &nsp);
                        add = 1;
                        covered = 1;
                        for(j = 0;j < cmd->tx_size;j++){
                            if(LLRB_RANGE_GROUP_ADD(span_tree, &rt,
                                        &cmd->spans[j], &ml, merge)){
                                covered = 0;
                            }
                        }
                        if(covered) break;
                    }
                }
                if(add) probe->updated += add_dep(probe->deps, ti);
                if(covered) return mseq;
            }else if(read_dependency(&rt, cmd)){
                probe->updated += add_dep(probe->deps, ti);
            }
        }
    }
    return mseq;
}

void barrier_dep(struct instance_index *index, struct dependency *deps) {
    add_dep(
        deps,
        LLRB_PREV(instance_index, index, LLRB_MAX(instance_index, index))
    );
}

void noop_dep(struct instance_index *index, struct instance_id *id,
        struct dependency *deps) {
    memset(deps, 0, DEPSIZE);
    struct instance l = {
        .key = *id
    };
    struct instance *dep = LLRB_PREV(instance_index, index,
            LLRB_FIND(instance_index, index, &l));
    if(!dep) return;
    add_dep(deps, dep);
}

void clear_index(struct instance_index *index){
    struct instance *c, *nxt, *executed;
    executed = 0;
    nxt = LLRB_MIN(instance_index, index);
    while(nxt){
        c = nxt;
        nxt = LLRB_NEXT(instance_index, index, nxt);
        if(c->status < EXECUTED) return;
        if(is_state(c->status, EXECUTED)){
            if(executed){
                destroy_instance(LLRB_DELETE(instance_index, index, executed));
            }
            executed = c;
        }
    }
}

int is_dep_conflict(struct instance *ti, uint64_t seq, struct dependency *deps,
        size_t replica_id){
    uint64_t lt_instance = lt_dep_replica(ti->key.replica_id, deps);
    if((ti->key.instance_id > lt_instance) ||
            ((ti->key.instance_id < lt_instance) &&
             (ti->seq >= seq) &&
             ((ti->key.replica_id != replica_id) ||
              ti->status > PRE_ACCEPTED_EQ))){
        return 1;
    }
    return 0;
}

struct instance* pre_accept_conflict(
    struct instance_index *index,
    struct instance *i,
    struct command *c,
    uint64_t seq,
    struct dependency *deps
){
    struct span_group ml;
    struct span_tree rt;
    struct instance *ti;
    struct span cspan[TX_SIZE];
    size_t j, k;
    int add, covered;
    LLRB_INIT(&rt);
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
        if(lt_dep_replica(i->key.replica_id, ti->deps) >=
                i->key.instance_id){
            continue;
        }
        memset(cspan, 0, sizeof(struct span)*TX_SIZE);
        if(interferes(ti->command, c, cspan)){
            if(ti->command->writing) {
                covered = 0;
                add = 0;
                for(j = 0;j < ti->command->tx_size;j++){
                    if(empty_range(&cspan[j])) break;
                    if(LLRB_RANGE_GROUP_ADD(span_tree, &rt,
                                &cspan[j], &ml, merge)){
                        struct span nsp = cspan[j];
                        LLRB_INSERT(span_tree, &rt, &nsp);
                        add = 1;
                        covered = 1;
                        for(k = 0;k < c->tx_size;k++){
                            if(LLRB_RANGE_GROUP_ADD(span_tree, &rt,
                                        &c->spans[k], &ml, merge)){
                                covered = 0;
                            }
                        }
                        if(covered) break;
                    }
                }
                if(add && is_dep_conflict(ti, seq, deps, i->key.replica_id)){
                    return ti;
                }
                if(covered) return 0;
            }else if(read_dependency(&rt, c)){
                if(is_dep_conflict(ti, seq, deps, i->key.replica_id)){
                    return ti;
                }
            }
        }

    }
    return 0;
}
