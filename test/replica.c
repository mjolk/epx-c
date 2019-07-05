/**
 * File   : replica.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : za 02 feb 2019 15:06
 */

#include "../src/replica.h"

int triggered[5] = {0};

void timeout_callback(void *arg){
    int id = *(int*) arg;
    triggered[id] = 1;
}

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
    timeout_init(&i.timer, 0);
    timeout_setcb(&i.timer, timeout_callback, 0);
    timeouts_add(r.timers, &i.timer, 20);
    //run_replica(&r);
    //destroy_replica(&r);
    return 0;
}
