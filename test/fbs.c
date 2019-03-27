/**
 * File   : fbs.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 17 jan 2019 03:38
 */

#include "../src/fbs/epx_verifier.h"
#include "../src/message.h"
#include <stdio.h>

void* to_buffer(flatcc_builder_t *b, struct message *m){
    message_to_buffer(m, b);
    size_t s;
    void *nb = flatcc_builder_finalize_aligned_buffer(b, &s);
    int ret;
    if((ret = ns(message_verify_as_root(nb, s)))){
        printf("Message buffer invalid: %s\n",
                flatcc_verify_error_string(ret));
        return 0;
    }
    return nb;
}

int main(){
    struct span sp[1] ={{
        .start_key = "1",
        .end_key = "4"
    }};
    struct command cmd = {
        .id = 1,
        .writing = WRITE,
        .value = 0
    };
    memcpy(cmd.spans, sp, sizeof(struct span));
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
            .id.instance_id = 1,
            .id.replica_id = 1,
            .status = COMMITTED,
        },
        .start = 1,
        .stop = 2
    };
    memset(m.deps, 0, sizeof(struct dependency)*MAX_DEPS);

    flatcc_builder_t builder;
    flatcc_builder_init(&builder);

    void *b = to_buffer(&builder, &m);
    if(b == 0) return 1;

    struct message decoded;
    struct span spd[1] = {{
        .start_key = "0",
        .end_key = "0"
    }};
    struct command dcmd = {
        .id = 0,
        .writing = READ,
        .value = 0
    };
    memcpy(dcmd.spans, spd, sizeof(struct span));
    decoded.command = &dcmd;
    if(message_from_buffer(&decoded, b)){
        return 1;
    }
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
    assert(strcmp(decoded.command->spans[0].start_key, "1") == 0);
    assert(strcmp(decoded.command->spans[0].end_key, "4") == 0);
    assert(decoded.command->id == 1);
    assert(decoded.command->writing == WRITE);

    flatcc_builder_reset(&builder);
    for(int i = 0;i < N;i++){
        m.deps[i].id.replica_id = 2;
        m.deps[i].id.instance_id = 1;
        m.deps[i].committed = 1;
    }
    void *db = to_buffer(&builder, &m);
    if(db == 0) return 1;
    if(message_from_buffer(&decoded, db)){
        return 1;
    }
    for(int i = 0;i < N;i++){
        assert(decoded.deps[i].id.replica_id == 2);
        assert(decoded.deps[i].id.instance_id == 1);
        assert(decoded.deps[i].committed == 1);
    }
    flatcc_builder_clear(&builder);
    free(db);
    return 0;
}

