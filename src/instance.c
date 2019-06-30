/**
 * File   : instance.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 03:36
 */

#include <stdio.h>
#include <inttypes.h>
#include "instance.h"
#include <ck_pr.h>
#include <errno.h>

struct message *message_from_instance(struct instance *i){
    struct message *nm = (struct message*)malloc(sizeof(struct message));
    if(!nm){ errno = ENOMEM; return 0;};
    nm->ballot = i->ballot;
    nm->command = i->command;
    nm->id = i->key;
    nm->seq = i->seq;
    memcpy(nm->deps, i->deps, DEPSIZE);
    return nm;
}

struct instance *instance_from_message(struct message *m){
    struct instance *i = (struct instance*)malloc(sizeof(struct instance));
    if(!i){ errno = ENOMEM; return 0;}
    i->ballot = m->ballot;
    i->seq = m->seq;
    i->status = NONE;
    i->command = m->command;
    i->deps_updated = 0;
    i->lt = 0;
    return i;
}

struct instance *new_instance(){
    struct instance *i = (struct instance*)malloc(sizeof(struct instance));
    if(!i){ errno = ENOMEM; return 0;}
    i->ballot = 0;
    i->seq = 0;
    i->status = NONE;
    i->deps_updated = 0;
    i->lt = 0;
    timeout_init(&i->timer, 0);
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

int is_state(enum state a, enum state b){
    return (a & b) > 0;
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

//ordered a > b so first result is largest instance id for replica
uint64_t lt_dep_replica(size_t replica_id, struct dependency *deps){
    for(int i = 0;i < MAX_DEPS;i++){
        if(replica_id == deps->id.replica_id){
            return deps->id.instance_id;
        }
        deps++;
    }
    return 0;
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
    //we know they are sorted a > b
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

void update_recovery_instance(struct recovery_instance *ri, struct message *m){
    ri->command = m->command;
    memcpy(ri->deps, m->deps, DEPSIZE);
    ri->seq = m->seq;
    ri->status = m->instance_status;
}

//instance tree comparator
int intcmp(struct instance *e1, struct instance *e2){
    return (e1->key.instance_id < e2->key.instance_id ? -1 :
            e1->key.instance_id > e2->key.instance_id);
}

//generate instance tree type
LLRB_GENERATE(instance_index, instance, entry, intcmp);

//update committed status for existing, add to free slot or replace oldest
//if smaller than smallest present ->dropped and no update signalled
int update_deps(struct dependency *deps, struct dependency *d) {
    if(d->id.instance_id == 0) return 0;
    int i;
    for(i = 0;i < MAX_DEPS;i++){
        if(eq_instance_id(&deps[i].id, &d->id)){
            if(deps[i].committed < d->committed){
                deps[i].committed = d->committed;
                return 1;
            }
            return 0;
        }
    }

    //adding
    //will we need to overwrite?
    int overflow = (deps[MAX_DEPS-1].id.instance_id != 0);
    int j = 0;
    for(i = 0;i < MAX_DEPS;i++){
        if(deps[i].id.instance_id < d->id.instance_id){
            j = MAX_DEPS-1;
            while(j > i){
                j--;
                if(deps[j+1].id.instance_id == 0) continue;
                deps[j+1] = deps[j];
            }
            deps[i] = *d;
            size_t ss = sizeof(deps);
            return 1;
        }
    }
    return 0;
}

int add_dep(struct dependency *deps, struct instance *i){
    struct dependency dep = {.id = i->key,
        .committed = is_committed(i)};
    return update_deps(deps, &dep);
}

int is_committed(struct instance *i){
    return (i->status >= COMMITTED);
}

int has_uncommitted_deps(struct instance *i){
    for(int dc = 0;dc < MAX_DEPS;dc++){
        if(i->deps[dc].id.instance_id == 0) return 0; //sorted a > b
        if(i->deps[dc].committed == 0){
            return 1;
        }
    }
    return 0;
}

void instance_reset(struct instance *i){
    memset(i->deps, 0, DEPSIZE);
    if(i->lt){
        //TODO reset all counters
        i->lt->ri.status = NONE;
    }
}
