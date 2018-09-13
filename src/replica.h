/**
 * File   : src/replica.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 20:21
 */

#include <stdint.h>
#include <stdlib.h>
#include "llrb-interval/slist.h"
#include "llrb-interval/llrb.h"
#include "llrb-interval/llrb-interval.h"
#include <err.h>

struct instance_index;
struct range_tree;

SLL_HEAD(tickers, timer);
struct replica {
    struct instance_index index[3];
    struct range_tree range_group;
    size_t replicas[3];
    size_t replica_id;
    struct tickers timers;
};

struct timer {
    SLL_ENTRY(timer) next;
    int elapsed;
    int time_out;
    int paused;
    struct instance* instance;
};

struct instance* replica_find_instance(struct replica*, struct instance*);
