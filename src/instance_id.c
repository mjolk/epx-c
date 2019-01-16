/**
 * File   : src/instance_id.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 13 sep 2018 23:22
 */

#include <stdint.h>
#include <stdlib.h>
#include "instance_id.h"

int is_instance_id(struct instance_id *a, struct instance_id *b){
    return (a->instance_id == b->instance_id) && (a->replica_id == b->replica_id);
}

uint64_t max_seq(uint64_t a, uint64_t b){
    return (a > b)?a:b;
}
