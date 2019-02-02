/**
 * File   : span.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : vr 04 jan 2019 19:49
 */

#include "instance.h"

uint64_t seq_deps_for_command(struct instance_index*, struct command*,
        struct instance_id*, struct seq_deps_probe *p);
void noop_deps(struct instance_index*, struct dependency*);
struct instance* pre_accept_conflict(struct instance_index*, struct instance *i,
        struct command *c, uint64_t seq, struct dependency *deps);
