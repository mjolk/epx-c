/**
 * File   : replica.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : za 02 feb 2019 15:06
 */

#include "../src/replica.h"

int main(){
    struct io_sync s;
    struct replica_sync rs;
    struct replica r;
    r.out = &s;
    r.in = &rs;
    r.frequency = 10;
    r.id = 1;
    assert(new_replica(&r) == 0);
    struct instance i = {
        .key = {
            .replica_id = 0,
            .instance_id = 1
        }
    };
    run_replica(&r);
    destroy_replica(&r);
    return 0;
}
