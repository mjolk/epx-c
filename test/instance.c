/**
 * File   : instance.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : ma 28 jan 2019 18:03
 */

#include "../src/instance.h"

struct dependency tdeps[N];
struct dependency tdeps2[N];

void new_deps(){
    for(int i = 0;i < N;i++){
        tdeps[i].id.instance_id = 1000+i;
        tdeps[i].id.replica_id = i;
        tdeps[i].committed = (((i+2)%2) > 0)?1:0;
        tdeps2[i].id.instance_id = 1000+i;
        tdeps2[i].id.replica_id = i;
        tdeps2[i].committed = (((i+2)%2) > 0)?1:0;
    }
}

int main(){
    new_deps();
    assert(equal_deps(tdeps, tdeps2) == 1);
    tdeps2[0].id.instance_id = 500;
    assert(equal_deps(tdeps, tdeps2) == 0);
    assert(is_state(PRE_ACCEPTED, (PRE_ACCEPTED | PRE_ACCEPTED_EQ)) == 1);
    assert(is_state(PREPARING, (PRE_ACCEPTED | PRE_ACCEPTED_EQ)) == 0);
    struct instance_id nid = {
        .instance_id = 1006,
        .replica_id = 1
    };
    struct dependency ndep = {
       .id = nid,
       .committed = 0
    };
    assert(update_deps(tdeps, &ndep) > 0);
    assert(tdeps[1].id.instance_id == 1006);
    assert(tdeps[1].committed == 0);
    ndep.id.instance_id = 999;
    assert(update_deps(tdeps, &ndep) == 0);
    ndep.id.instance_id = 1006;
    tdeps[1].id.instance_id = 0;
    assert(update_deps(tdeps, &ndep) > 0);
    assert(tdeps[1].id.instance_id == 1006);
    return 0;
}
