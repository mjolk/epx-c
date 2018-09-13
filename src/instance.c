/**
 * File   : instance.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 03:36
 */

#include "instance.h"

int intcmp(struct instance *e1, struct instance *e2)
{
    return (e1->start < e2->start ? -1 : e1->start > e2->start);
}

LLRB_GENERATE(instance_index, instance, entry, intcmp)
LLRB_GENERATE(range_tree, instance, entry, intcmp)
LLRB_RANGE_GROUP_GEN(range_tree, instance, entry, rg, next);

void instance_update_deps(struct instance *i, struct instance_id id) {
    int add = 1;
    int next = 0;
    for(int dc = 0; dc < 8;dc++){
        if(i->deps[dc].replica_id == 0 || i->deps[dc].instance_id == 0) {
            next = dc;
            continue;
        }
        if(i->deps[dc].replica_id == id.replica_id && 
                i->deps[dc].instance_id == id.instance_id){
            //memcpy?
            i->deps[dc] = id;
            add = 0; 
        }
        
    }
    if(add && next){
        //memcpy?
        i->deps[next] = id;
    }
    //TODO heap memory or configure size and stay on stack...
}
