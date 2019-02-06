/**
 * File   : replica.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 20:21
 */

#include "index.h"
#include <err.h>
#include <libdill.h>
#include "hash.h"

#define MSG_SIZE sizeof(struct message*)
KHASH_MAP_INIT_INT64(deferred, size_t)

struct replica {
    int chan_tick[2];
    int chan_propose[2] ;
    int chan_ii[2];
    int chan_io[2];
    int chan_eo[2];
    int chan_exec[2];
    int running;
    uint8_t epoch;
    size_t id;
    struct instance_index index[N];
    struct tickers timers;
    khash_t(deferred) *dh;
};

struct instance* find_instance(struct replica*, struct instance_id*);
uint64_t sd_for_command(struct replica*, struct command*,
        struct instance_id*, struct seq_deps_probe*);
struct instance* pac_conflict(struct replica*, struct instance*,
        struct command *c, uint64_t seq, struct dependency *deps);
uint64_t max_local(struct replica*);
void register_timer(struct replica*, struct instance*, int time_out);
int register_instance(struct replica*, struct instance*);
int new_replica(struct replica*);
void tick(struct replica*);
int send_eo(struct replica*, struct message*);
int send_exec(struct replica*, struct message*);
int send_io(struct replica*, struct message*);
void destroy_replica(struct replica*);
