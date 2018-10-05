/**
 * File   : src/replica.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 20:21
 */

#include "instance.h"
#include <err.h>
#include <libdill.h>

struct replica {
    /** node -> time/tick input **/
    int chan_tick[2];
    /** node -> replica proposal input **/
    int chan_propose[2] ;
    /** node -> internal input **/
    int chan_ii[2];
    /** internal output **/
    int chan_io[2];
    /** exec output **/
    int chan_eo[2];
    /** is replica running **/
    int running;
    /** current replica quorum **/
    uint8_t epoch;
    /** replica identifier -> global unique **/
    size_t id;
    /** index current live running instances **/
    struct instance_index index[N];
    /** all known peers **/
    size_t replicas[N];
    /** all instance timers **/
    struct tickers timers;
};

struct instance* find_instance(struct replica*, struct instance_id*);
uint64_t sd_for_command(struct replica*, struct command*,
        struct instance_id*, struct seq_deps_probe*);
struct instance* pac_conflict(struct replica*, struct instance*,
        struct command *c, uint64_t seq, struct dependency *deps);
uint64_t max_local(struct replica*);
int register_timer(struct replica*, struct instance*, int time_out);
int register_instance(struct replica*, struct instance*);
struct replica *replica_new();
void tick(struct replica*);
