/**
 * File   : src/index.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : vr 04 jan 2019 19:49
 */

#include "instance.h"

uint64_t seq_deps_for_command(struct instance_index*, struct command*,
        struct instance_id*, struct seq_deps_probe *p);
