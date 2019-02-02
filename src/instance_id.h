/**
 * File   : instance_id.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 13 sep 2018 23:22
 */

#include "fbs/epx_reader.h"
#include "fbs/epx_builder.h"

#define N 3
#define VALUE_SIZE 1024
#define KEY_SIZE 3
#define MAX_DEPS 8

struct instance_id{
    size_t replica_id;
    uint64_t instance_id;
};

struct dependency{
    struct instance_id id;
    uint8_t committed;
};

int eq_instance_id(struct instance_id*, struct instance_id*);

uint64_t max_seq(uint64_t a, uint64_t b);
