/**
 * File   : src/instance.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 03:42
 */

#include "llrb-interval/llrb-interval.h"
#include "llrb-interval/slist.h"
#include "message.h"

enum state {
    NONE,
    PRE_ACCEPTED,
    ACCEPTED,
    PREPARING,
    COMMITTED,
    EXECUTED
} state;

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
    uint8_t pre_accept_oks;
    uint8_t accept_oks;
    uint8_t nacks;
    uint8_t prepare_oks;
    uint8_t equal;
};

struct recovery_instance {

};

SLL_HEAD(tickers, timer);
LLRB_HEAD(instance_index, instance);
LLRB_PROTOTYPE(instance_index, instance, entry, intcmp);

uint64_t seq_deps_for_command(struct instance_index*, struct command*,
        struct instance_id*, struct seq_deps_probe *p);
struct instance *instance_from_message(struct message*);
void instance_reset(struct instance*);
int is_state(struct instance *i, uint8_t state);
int is_initial_ballot(uint8_t ballot);
int update_deps(struct dependency**, struct instance*);
void timer_cancel(struct timer*);
void timer_set(struct timer*, int);
