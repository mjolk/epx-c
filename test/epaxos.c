/**
 * File   : epaxos.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : za 02 feb 2019 20:32
 */

#include "../src/epaxos.c"

struct span sp1[4] = {
    {
        .start_key = "05",
        .end_key = "10"
    },
    {
        .start_key = "20",
        .end_key = "40"
    },
    {
        .start_key = "08",
        .end_key = "18"
    },
    {
        .start_key = "26",
        .end_key = "51"
    }
};

struct span sp2[4] = {
    {
        .start_key = "01",
        .end_key = "05"
    },
    {
        .start_key = "54",
        .end_key = "80"
    },
    {
        .start_key = "38",
        .end_key = "45"
    },
    {
        .start_key = "19",
        .end_key = "19"
    }
};

struct command *create_command(char *start_key, char *end_key, enum io_t rw){
    struct command *cmd = (struct command*)malloc(sizeof(struct command));
    strcpy(cmd->spans[0].start_key, start_key);
    strcpy(cmd->spans[0].end_key, end_key);
    memset(cmd->spans[0].max, 0, KEY_SIZE);
    cmd->writing = rw;
    cmd->tx_size = 1;
    return cmd;
}

struct message *create_message(enum message_type t, char *start_key,
        char *end_key, enum io_t rw){
    struct command *cmd = create_command(start_key, end_key, rw);
    if(!cmd) return 0;
    struct message *m  = (struct message*)malloc(sizeof(struct message));
    m->command = cmd;
    m->type = t;
    m->start = 0;
    m->nack = 0;
    m->instance_status = NONE;
    m->ballot = 0;
    memset(m->deps, 0, MAX_DEPS*sizeof(struct dependency));
    return m;
}

void copy_command(struct message *m){
    struct command *cmd = (struct command*)malloc(sizeof(struct command));
    *cmd = *m->command;
    m->command = cmd;
}

void check(struct replica *r, enum message_type t, size_t s, size_t rid){
    struct message *m;
    int rc = chan_recv_spsc(&r->out->chan_io, &m);
    assert(rc > 0);
    assert(m->id.instance_id == 1);
    assert(m->id.replica_id == rid);
    assert(m->type == t);
    assert(m->seq == s);
}

void no_conflict(){
    struct io_sync s;
    struct replica_sync rs;
    chan_init(&s.chan_io);
    chan_init(&s.chan_eo);
    struct replica r1;
    struct replica r2;
    new_replica(&r1);
    new_replica(&r2);
    r1.id = 0;
    r2.id = 1;
    r1.out = &s;
    r2.out = &s;
    r1.in = &rs;
    r2.in = &rs;
    struct message *m = create_message(PHASE1, "10", "20", WRITE);
    assert(propose(&r1, m) == 0);
    check(&r1, PRE_ACCEPT, 1, 0);
    copy_command(m);
    assert(m != 0);
    assert(step(&r2, m) == 0);
    check(&r2, PRE_ACCEPT_OK, 1, 0);
    assert(step(&r1, m) == 0);
    check(&r1, COMMIT, 1, 0);
    destroy_replica(&r1);
    destroy_replica(&r2);
    free(m);
}

void simple_conflict(){
    struct io_sync s;
    struct replica_sync rs;
    chan_init(&s.chan_io);
    chan_init(&s.chan_eo);
    struct replica r1;
    struct replica r2;
    new_replica(&r1);
    new_replica(&r2);
    r1.id = 0;
    r2.id = 1;
    r1.out = &s;
    r2.out = &s;
    r1.in = &rs;
    r2.in = &rs;
    struct message *m1 = create_message(PHASE1, "10", "20", WRITE);
    struct message *m2 = create_message(PHASE1, "06", "18", WRITE);
    assert(propose(&r1, m1) == 0);
    assert(propose(&r2, m2) == 0);
    check(&r1, PRE_ACCEPT, 1, 0);
    check(&r2, PRE_ACCEPT, 1, 1);
    copy_command(m1);
    assert(step(&r2, m1) == 0);
    check(&r2, PRE_ACCEPT_REPLY, 2, 0);
    assert(step(&r1, m1) == 0);
    check(&r1, ACCEPT, 2, 0);
    assert(step(&r2, m1) == 0);
    check(&r2, ACCEPT_REPLY, 2, 0);
    assert(step(&r1, m1) == 0);
    check(&r1, COMMIT, 2, 0);
    destroy_replica(&r1);
    destroy_replica(&r2);
    free(m1);
    free(m2);
}

void simple_conflict_tx(){
    struct io_sync s;
    struct replica_sync rs;
    chan_init(&s.chan_io);
    chan_init(&s.chan_eo);
    struct replica r1;
    struct replica r2;
    new_replica(&r1);
    new_replica(&r2);
    r1.id = 0;
    r2.id = 1;
    r1.out = &s;
    r2.out = &s;
    r1.in = &rs;
    r2.in = &rs;
    struct message *m1 = create_message(PHASE1, "10", "20", WRITE);
    struct message *m2 = create_message(PHASE1, "06", "18", WRITE);
    memcpy(m1->command->spans, sp1, sizeof(struct span)*4);
    memcpy(m2->command->spans, sp2, sizeof(struct span)*4);
    m1->command->tx_size = m2->command->tx_size = 4;
    assert(propose(&r1, m1) == 0);
    assert(propose(&r2, m2) == 0);
    check(&r1, PRE_ACCEPT, 1, 0);
    check(&r2, PRE_ACCEPT, 1, 1);
    copy_command(m1);
    assert(step(&r2, m1) == 0);
    check(&r2, PRE_ACCEPT_REPLY, 2, 0);
    assert(step(&r1, m1) == 0);
    check(&r1, ACCEPT, 2, 0);
    assert(step(&r2, m1) == 0);
    check(&r2, ACCEPT_REPLY, 2, 0);
    assert(step(&r1, m1) == 0);
    check(&r1, COMMIT, 2, 0);
    destroy_replica(&r1);
    destroy_replica(&r2);
    free(m1);
    free(m2);
}

void recovery0(){
    struct io_sync s;
    struct replica_sync rs;
    chan_init(&s.chan_io);
    chan_init(&s.chan_eo);
    struct replica r1;
    struct replica r2;
    new_replica(&r1);
    new_replica(&r2);
    r1.id = 0;
    r2.id = 1;
    r1.out = &s;
    r2.out = &s;
    r1.in = &rs;
    r2.in = &rs;
    struct message *m0 = create_message(PHASE1, "10", "20", WRITE);
    assert(m0);
    struct instance *ri0 = instance_from_message(m0);
    copy_command(m0);
    struct instance *ri1 = instance_from_message(m0);
    ri0->status = PRE_ACCEPTED;
    ri0->seq = 3;
    ri0->key.instance_id = 4;
    ri0->key.replica_id = 1;
    ri1->seq = 3;
    ri1->status = PRE_ACCEPTED_EQ;
    ri1->key.instance_id = 4;
    ri1->key.replica_id = 1;
    struct dependency deps0[2] = {
        {
            .id = {
                .instance_id = 1,
                .replica_id = 1
            },
            .committed = 0
        },
        {
            .id = {
                .instance_id = 2,
                .replica_id = 0
            },
            .committed = 0
        }
    };
    memcpy(ri0->deps, deps0, sizeof(struct dependency)*2);
    memcpy(ri1->deps, deps0, sizeof(struct dependency)*2);
    register_instance(&r1, ri0);
    register_instance(&r2, ri1);

    struct message *m1 = create_message(PHASE1, "05", "11", WRITE);
    assert(m1);
    struct instance *ri2 = instance_from_message(m1);
    copy_command(m1);
    struct instance *ri3 = instance_from_message(m1);
    ri2->seq = 2;
    ri2->status = ACCEPTED;
    ri2->key.instance_id = 1;
    ri2->key.replica_id = 1;
    ri3->seq = 2;
    ri3->status = ACCEPTED;
    ri3->key = ri2->key;
    struct dependency deps1[1] = {
        {
            .id = {
                .instance_id = 3,
                .replica_id = 1
            },
            .committed = 0
        }
    };
    memcpy(ri2->deps, deps1, sizeof(struct dependency));
    memcpy(ri3->deps, deps1, sizeof(struct dependency));
    register_instance(&r1, ri2);
    register_instance(&r2, ri3);

    struct message *m2 = create_message(PHASE1, "18", "30", WRITE);
    assert(m2);
    struct instance *ri4 = instance_from_message(m2);
    copy_command(m2);
    struct instance *ri5 = instance_from_message(m2);
    ri4->seq = 1;
    ri4->status = PRE_ACCEPTED;
    ri4->key.instance_id = 2;
    ri4->key.replica_id = 0;
    ri5->seq = 1;
    ri5->status = PRE_ACCEPTED;
    ri5->key = ri4->key;
    register_instance(&r1, ri4);
    register_instance(&r2, ri5);
    struct message *recovery_msg = (struct message*)malloc(sizeof(struct message));
    assert(recovery_msg);
    recover(&r1, ri0, recovery_msg);
    struct message *rep;
    int rc = 0;
    rc = chan_recv_spsc(&r1.out->chan_io, &rep);
    assert(rc >= 0);
    assert(rep->ballot != 0);
    assert(is_state(ri0->lt->recovery_status, PREPARING));
    assert(step(&r2, rep) == 0);
    rc = chan_recv_spsc(&r2.out->chan_io, &rep);
    assert(rc >= 0);
    assert(rep->instance_status == PRE_ACCEPTED_EQ);
    assert(rep->type == PREPARE_REPLY);
    assert(step(&r1, rep) == 0);
    rc = chan_recv_spsc(&r1.out->chan_io, &rep);
    assert(rc >= 0);
    assert(ri0->lt->prepare_oks == 1);
    assert(rep->ballot != 0);
}

void recovery(){

}

int main(){
    no_conflict();
    simple_conflict();
    simple_conflict_tx();
    recovery0();
    recovery();
    return 0;
}
