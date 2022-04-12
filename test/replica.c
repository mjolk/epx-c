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
    printf("callback %d\t", id);
    assert(id == 1);
    triggered[id] = 1;
    free(arg);
}

void test_simple_timeout(struct replica *r) {
    struct instance i = {
        .key = {
            .replica_id = 0,
            .instance_id = 1
        }
    };
    timeout_init(&i.timer, 0);
    int *ic  = (int*)malloc(sizeof(int));
    assert(ic != 0);
    *ic = 1;
    timeout_setcb(&i.timer, timeout_callback, ic);
    timeouts_add(r->timers, &i.timer, 2);
    tick(r);
    tick(r);

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
    test_simple_timeout(&r);
    //destroy_replica(&r);
    return 0;
}
