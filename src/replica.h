/**
 * File   : src/replica.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 20:21
 */

#include "instance.h"
#include <err.h>
#include <libdill.h>
#include "buffer.h"

struct replica {
    struct cbuf* crank;
    struct cbuf* sink;
    struct cbuf* exec;
    int running;
    uint8_t epoch;
    size_t id;
    struct instance_index index[N];
    size_t replicas[N];
    size_t replica_id;
    struct tickers timers;
};

struct instance* find_instance(struct replica*, struct instance_id*);
uint64_t sd_for_command(struct replica*, struct command*,
        struct instance_id*, struct seq_deps_probe*);
uint64_t max_local(struct replica*);
int register_timer(struct replica*, struct instance*, int time_out);
int register_instance(struct replica*, struct instance*);
struct replica* replica_init(void);
