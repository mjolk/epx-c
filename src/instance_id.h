/**
 * File   : src/instance_id.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 13 sep 2018 23:22
 */

#include <stdint.h>
#include <stdlib.h>
#include "fbs/epx_reader.h"
#include "fbs/epx_builder.h"

#define N 3
#define VALUE_SIZE 1024
#define KEY_SIZE 16

struct instance_id{
    size_t replica_id;
    uint64_t instance_id;
};

struct dependency{
    struct instance_id id;
    uint8_t committed;
};

int is_instance_id(struct instance_id *a, struct instance_id *b){
    return (a->instance_id == b->instance_id) && (a->replica_id == b->replica_id);
}

uint64_t max_seq(uint64_t a, uint64_t b){
    return (a > b)?a:b;
}
