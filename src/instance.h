/**
 * File   : instance.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 03:42
 */

#include "llrb-interval/llrb-interval.h"
#include "llrb-interval/llrb.h"
#include "llrb-interval/slist.h"
#include <stdint.h>
#include <stdlib.h>

enum state {
    NONE,
    PRE_ACCEPTED,
    ACCEPTED,
    PREPARING,
    COMMITTED,
    EXECUTED
} state;

struct instance_id {
    size_t replica_id;
    uint64_t instance_id;
};

struct nptr {
    struct instance* sle_next;
};

struct timer;

struct instance {
    struct instance_id deps[8];
    struct instance_id id;
    uint8_t ballot;
    enum state status;
    struct command* command;
    uint64_t seq;
    struct replica* r;
    uint64_t start;
    uint64_t end;
    uint64_t max;
    struct nptr next;
    LLRB_ENTRY(instance) entry;
    void (*on)(struct timer*);
    struct message* replies[16];
};

SLL_HEAD(rg, instance);
LLRB_HEAD(instance_index, instance);
LLRB_HEAD(range_tree, instance);
LLRB_PROTOTYPE(instance_index, instance, entry, intcmp);
LLRB_PROTOTYPE(range_tree, instance, entry, intcmp);

void instance_update_deps(struct instance*, struct instance_id id);
