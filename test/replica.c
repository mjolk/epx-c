/**
 * File   : replica.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : za 02 feb 2019 15:06
 */

#include "../src/replica.h"

void extend_timer(struct replica *r, struct timer *c){
    c->elapsed = 0;
    c->time_out = 2;
}

void check_cnt_elapsed(struct replica *r, int chc, int elapsed){
    struct timer *tt;
    int cnt = 0;
    SIMPLEQ_FOREACH(tt, &r->timers, next){
        assert(tt->elapsed == elapsed);
        cnt++;
    }
    assert(cnt == chc);
}

coroutine void time_elapsed(struct replica *r, int chtick){
    int ticks = 4;
    uint8_t t;
    while(ticks){
        t = 0;
        chrecv(r->chan_tick[1], &t, sizeof(uint8_t), now() + 500);
        if(t) ticks--;
    }
    chsend(chtick, &t, sizeof(uint8_t), 20);
}

int main(){
    struct io_sync s;
    struct replica_sync rs;
    struct replica r;
    r.out = &s;
    r.in = &rs;
    r.frequency = 100;
    r.id = 1;
    assert(new_replica(&r) == 0);
    struct instance i = {
        .key = {
            .replica_id = 0,
            .instance_id = 1
        }
    };
    instance_timer(&r, &i, 4);
    for(int t = 0;t < 4;t++){
        tick(&r);
    }
    assert(i.ticker.elapsed == 4);
    assert(!SIMPLEQ_FIRST(&r.timers));
    i.ticker.on = extend_timer;
    instance_timer(&r, &i, 4);
    for(int t = 0;t < 4;t++){
        tick(&r);
    }
    assert(i.ticker.elapsed == 0);
    assert(i.ticker.time_out == 2);
    assert(SIMPLEQ_FIRST(&r.timers));
    i.ticker.on = 0;
    for(int t = 0;t < 2;t++){
        tick(&r);
    }
    assert(i.ticker.elapsed == 2);
    assert(!SIMPLEQ_FIRST(&r.timers));
    struct instance i2 = {
        .key = {
            .replica_id = 0,
            .instance_id = 2
        }
    };
    struct instance i3 = {
        .key = {
            .replica_id = 0,
            .instance_id = 3
        }
    };
    instance_timer(&r, &i, 3);
    instance_timer(&r, &i2, 2);
    instance_timer(&r, &i3, 1);
    tick(&r);
    assert(i3.ticker.elapsed == 1);
    check_cnt_elapsed(&r, 2, 1);
    tick(&r);
    assert(i2.ticker.elapsed == 2);
    check_cnt_elapsed(&r, 1, 2);
    tick(&r);
    assert(i.ticker.elapsed == 3);
    check_cnt_elapsed(&r, 0, 3);

    instance_timer(&r, &i, 1);
    instance_timer(&r, &i2, 2);
    instance_timer(&r, &i3, 3);
    tick(&r);
    assert(i.ticker.elapsed == 1);
    check_cnt_elapsed(&r, 2, 1);
    tick(&r);
    assert(i2.ticker.elapsed == 2);
    check_cnt_elapsed(&r, 1, 2);
    tick(&r);
    assert(i3.ticker.elapsed == 3);
    check_cnt_elapsed(&r, 0, 3);

    struct instance i4 = {
        .key = {
            .replica_id = 0,
            .instance_id = 4
        }
    };

    instance_timer(&r, &i4, 4);
    instance_timer(&r, &i, 1);
    instance_timer(&r, &i2, 3);
    instance_timer(&r, &i3, 2);
    tick(&r);
    assert(i.ticker.elapsed == 1);
    check_cnt_elapsed(&r, 3, 1);
    tick(&r);
    assert(i3.ticker.elapsed == 2);
    check_cnt_elapsed(&r, 2, 2);
    tick(&r);
    assert(i2.ticker.elapsed == 3);
    check_cnt_elapsed(&r, 1, 3);
    tick(&r);
    assert(i4.ticker.elapsed == 4);
    check_cnt_elapsed(&r, 0, 0);
    int ap = bundle();
    int w[2];
    chmake(w);
    uint8_t done;
    run_replica(&r);
    bundle_go(ap, time_elapsed(&r, w[0]));
    int rc = chrecv(w[1], &done, sizeof(uint8_t), now() + 8000);
    assert(done == 1 && rc == 0);
    destroy_replica(&r);
    hclose(w[0]);
    hclose(w[1]);
    hclose(ap);
    return 0;
}
