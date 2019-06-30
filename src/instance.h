/**
 * File   : instance.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 03:42
 */
#ifndef EPX_INSTANCE
#define EPX_INSTANCE
#include "timeout/timeout.h"
#include "message.h"

#define DEPSIZE sizeof(struct dependency)*MAX_DEPS
#define TIMEOUT_DISABLE_RELATIVE_ACCESS

struct replica;
struct instance_index;

struct seq_deps_probe{
    int updated;
    struct dependency deps[MAX_DEPS];
};

struct instance {
    struct timeout timer;
    struct dependency deps[MAX_DEPS];
    uint8_t deps_updated;
    struct instance_id key;
    uint8_t ballot;
    volatile enum state status;
    struct command* command;
    uint64_t seq;
    LLRB_ENTRY(instance) entry;
    struct leader_tracker *lt;
    struct replica *r;
};

struct recovery_instance {
    struct command *command;
    struct dependency deps[MAX_DEPS];
    enum state status;
    uint64_t seq;
    uint8_t pre_accept_count;
    uint8_t leader_replied;
};

struct leader_tracker {
    uint8_t pre_accept_oks;
    uint8_t accept_oks;
    uint8_t prepare_oks;
    uint8_t try_pre_accept_oks;
    uint8_t nacks;
    uint8_t equal;
    uint8_t max_ballot;
    enum state recovery_status;
    struct recovery_instance ri;
    uint8_t quorum[N];
};

//int intcmp(struct instance*, struct instance*);
SIMPLEQ_HEAD(tickers, timer);
LLRB_HEAD(instance_index, instance);
LLRB_PROTOTYPE(instance_index, instance, entry, intcmp);

struct instance *new_instance();
struct instance *instance_from_message(struct message*);
struct message *message_from_instance(struct instance*);
void update_recovery_instance(struct recovery_instance*, struct message*);
void instance_reset(struct instance*);
int is_state(enum state, enum state);
int is_initial_ballot(uint8_t ballot);
int update_deps(struct dependency*, struct dependency*);
int equal_deps(struct dependency*, struct dependency*);
int add_dep(struct dependency*, struct instance*);
uint8_t larger_ballot(uint8_t, size_t);
size_t replica_from_ballot(uint8_t);
uint8_t unique_ballot(uint8_t, size_t);
int has_uncommitted_deps(struct instance *i);
int is_committed(struct instance*);
int has_key(struct instance_id*, struct dependency*);
uint64_t lt_dep_replica(size_t, struct dependency*);
void destroy_instance(struct instance*);
void set_dep_committed(struct instance_id*, struct dependency*);
#endif
