/**
 * File   : fbs.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 17 jan 2019 03:38
 */

#include "../src/fbs/epx_verifier.h"
#include "../src/message.h"
#include <stdio.h>

int main(){
    struct span sp = {
        .start_key = "1",
        .end_key = "4"
    };
    struct command cmd = {
        .span = sp,
        .id = 1,
        .writing = WRITE,
        .value = {0}
    };
    struct message m = {
        .type = PRE_ACCEPT,
        .to = 2,
        .ballot = 2,
        .nack = 1,
        .id = {
            .replica_id = 1,
            .instance_id = 1
        },
        .command = &cmd,
        .seq = 123456,
        .deps = {},
        .from = 3,
        .instance_status = PRE_ACCEPTED_EQ,
        .conflict = {
            .instance_id = 1,
            .replica_id = 1,
            .status = COMMITTED,
        },
        .start = 1,
        .stop = 2
    };
    memset(m.deps, 0, sizeof(struct dependency)*N);

    flatcc_builder_t builder;
    flatcc_builder_init(&builder);
    message_to_buffer(&m, &builder);
    size_t s;
    void *b = flatcc_builder_finalize_aligned_buffer(&builder, &s);
    int ret;
    if((ret = ns(message_verify_as_root(b, s)))){
        printf("Message buffer invalid: %s\n",
                flatcc_verify_error_string(ret));
        return 1;
    }

    struct message decoded;
    struct span spd = {
        .start_key = "0",
        .end_key = "0"
    };
    struct command dcmd = {
        .span = spd,
        .id = 0,
        .writing = READ,
        .value = {0}
    };
    decoded.command = &dcmd;
    if(message_from_buffer(&decoded, b)){
        return 1;
    }
    flatcc_builder_clear(&builder);
    free(b);
    assert(decoded.type == PRE_ACCEPT);
    assert(decoded.to == 2);
    assert(decoded.nack == 1);
    assert(decoded.seq == 123456);
    assert(decoded.from == 3);
    assert(decoded.start == 1);
    assert(decoded.stop == 2);
    assert(decoded.instance_status == PRE_ACCEPTED_EQ);
    assert(decoded.id.replica_id == 1);
    assert(decoded.id.instance_id == 1);
    assert(strcmp(decoded.command->span.start_key, "1") == 0);
    assert(strcmp(decoded.command->span.end_key, "4") == 0);
    assert(decoded.command->id == 1);
    assert(decoded.command->writing == WRITE);
    return 0;
}