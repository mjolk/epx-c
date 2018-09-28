/**
 * File   : src/instance.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 03:42
 */

#include "llrb-interval/slist.h"
#include "message.h"

struct seq_deps_probe{
    int updated;
    struct dependency* deps[N];
};

struct nptr {
    struct instance* sle_next;
};

struct timer {
    SLL_ENTRY(timer) next;
    int elapsed;
    int time_out;
    int paused;
    struct instance* instance;
};

struct instance {
    struct dependency* deps[N];
    struct instance_id key;
    uint8_t ballot;
    enum state status;
    struct command* command;
    uint64_t seq;
    struct replica* r;
    uint64_t start_key;
    uint64_t end_key;
    uint64_t max;
    struct nptr next;
    LLRB_ENTRY(instance) entry;
    void (*on)(struct timer*);
    struct leader_tracker *lt;
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
    struct recovery_instance *ri;
    uint8_t quorum[N];
};

struct recovery_instance {
    struct command *command;
    struct dependency* deps[N];
    enum state status;
    uint64_t seq;
    uint8_t pre_accept_count;
    uint8_t leader_replied;
};

SLL_HEAD(tickers, timer);
LLRB_HEAD(instance_index, instance);
LLRB_PROTOTYPE(instance_index, instance, entry, intcmp);

uint64_t seq_deps_for_command(struct instance_index*, struct command*,
        struct instance_id*, struct seq_deps_probe *p);
struct instance *instance_from_message(struct message*);
void update_recovery_instance(struct recovery_instance*, struct message*);
void instance_reset(struct instance*);
int is_state(enum state, enum state);
int is_initial_ballot(uint8_t ballot);
int update_deps(struct dependency**, struct dependency*);
int equal_deps(struct dependency**, struct dependency**);
void timer_cancel(struct timer*);
void timer_set(struct timer*, int);
uint8_t larger_ballot(uint8_t, size_t);
size_t leader_from_ballot(uint8_t);
int has_uncommitted_deps(struct instance *i);
