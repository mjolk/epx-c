/**
 * File   : instance.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 03:42
 */

#include "llrb-interval/slist.h"
#include "message.h"

#define DEPSIZE sizeof(struct dependency)*MAX_DEPS

struct replica;
struct instance_index;

struct seq_deps_probe{
    int updated;
    struct dependency deps[MAX_DEPS];
};

struct timer {
    SIMPLEQ_ENTRY(timer) next;
    int elapsed;
    int time_out;
    int paused;
    struct instance* instance;
    void (*on)(struct replica*, struct timer*);
};

struct instance {
    struct timer ticker;
    struct dependency deps[MAX_DEPS];
    uint8_t deps_updated;
    struct instance_id key;
    uint8_t ballot;
    volatile enum state status;
    struct command* command;
    uint64_t seq;
    LLRB_ENTRY(instance) entry;
    struct leader_tracker *lt;
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
int is_state(struct instance*, enum state);
int is_sstate(enum state, enum state);
int lt_state(struct instance*, enum state);
int lt_eq_state(struct instance*, enum state);
int st_state(struct instance*, enum state);
void set_state(struct instance*, enum state);
enum state get_state(struct instance*);
int is_initial_ballot(uint8_t ballot);
int update_deps(struct dependency*, struct dependency*);
int equal_deps(struct dependency*, struct dependency*);
int add_dep(struct dependency*, struct instance*);
void timer_cancel(struct timer*);
void timer_set(struct timer*, int);
void timer_reset(struct timer*, int);
uint8_t larger_ballot(uint8_t, size_t);
size_t replica_from_ballot(uint8_t);
uint8_t unique_ballot(uint8_t, size_t);
int has_uncommitted_deps(struct instance *i);
int is_committed(struct instance*);
int has_key(struct instance_id*, struct dependency*);
uint64_t lt_dep_replica(size_t, struct dependency*);
void destroy_instance(struct instance*);
void set_dep_committed(struct instance_id*, struct dependency*);
