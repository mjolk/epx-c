/**
 * File   : epaxos.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : za 02 feb 2019 20:32
 */

#include "../src/epaxos.c"

struct message *create_message(enum message_type t, char *start_key,
        char *end_key, enum io_t rw ){
    struct command *cmd = malloc(sizeof(struct command));
    strncpy(cmd->span.start_key, start_key, KEY_SIZE);
    strncpy(cmd->span.end_key, end_key, KEY_SIZE);
    memset(cmd->span.max, 0, KEY_SIZE);
    cmd->writing = rw;
    struct message *m  = malloc(sizeof(struct message));
    m->command = cmd;
    m->type = t;
    m->start = 0;
    m->nack = 0;
    m->instance_status = NONE;
    m->ballot = 0;
    for(int ld = 0;ld < MAX_DEPS;ld++){
        m->deps[ld].id.instance_id = 0;
        m->deps[ld].id.replica_id = 0;
        m->deps[ld].committed = 0;
    }
    return m;
}

struct message *copy_message(struct message *m){
    struct command *cmd2 = malloc(sizeof(struct command));
    struct message *m2 = malloc(sizeof(struct message));
    *m2 = *m;
    *cmd2 = *m->command;
    m2->command = cmd2;
    return m2;
}

coroutine void phase1_check(int ch, int done){
    struct message *m;
    int rc = chrecv(ch, &m, MSG_SIZE, -1);
    assert(rc >= 0);
    assert(m->id.instance_id == 1);
    assert(m->id.replica_id == 0);
    assert(m->type == PRE_ACCEPT);
    assert(m->seq == 1);
    chsend(done, &m, MSG_SIZE, -1);
}

coroutine void pre_accept_check(int ch, int done){
    struct message *m;
    assert(chrecv(ch, &m, MSG_SIZE, -1) >= 0);
    assert(m->id.instance_id == 1);
    assert(m->id.replica_id == 0);
    assert(m->type == PRE_ACCEPT_OK);
    assert(m->seq == 1);
    chsend(done, &m, MSG_SIZE, -1);
}

coroutine void pre_accept_ok_check(int ch, int done){
    struct message *m;
    assert(chrecv(ch, &m, MSG_SIZE, -1) >= 0);
    assert(m->id.instance_id == 1);
    assert(m->id.replica_id == 0);
    assert(m->type == COMMIT);
    assert(m->seq == 1);
    chsend(done, &m, MSG_SIZE, -1);
}

void no_conflict(){
    struct replica r1;
    struct replica r2;
    struct message *m = create_message(PHASE1, "10", "20", WRITE);
    int done_ch[2];
    assert(chmake(done_ch) >= 0);
    int b = bundle();
    r1.id = 0;
    r2.id = 1;
    assert(new_replica(&r1) == 0);
    assert(new_replica(&r2) == 0);
    assert(bundle_go(b, phase1_check(r1.chan_io[0], done_ch[0])) >= 0);
    assert(propose(&r1, m) == 0);
    assert(chrecv(done_ch[1], &m, MSG_SIZE, -1) >= 0);
    assert(bundle_go(b, pre_accept_check(r2.chan_io[0], done_ch[0])) >= 0);
    struct message *m2 = copy_message(m);
    assert(m2 != 0);
    assert(step(&r2, m2) == 0);
    assert(chrecv(done_ch[1], &m2, MSG_SIZE, -1) >= 0);
    assert(bundle_go(b, pre_accept_ok_check(r1.chan_exec[0], done_ch[0])) >= 0);
    assert(step(&r1, m2) == 0);
    assert(chrecv(done_ch[1], &m2, MSG_SIZE, -1) >= 0);
    destroy_replica(&r1);
    destroy_replica(&r2);
    hclose(done_ch[0]);
    hclose(done_ch[1]);
    hclose(b);
    free(m);
    free(m2);
}

coroutine void pre_accept_reply_check(int ch, int done){
    struct message *m;
    assert(chrecv(ch, &m, MSG_SIZE, -1) >= 0);
    assert(m->id.instance_id == 1);
    assert(m->id.replica_id == 0);
    assert(m->type == ACCEPT);
    assert(m->seq == 2);
    chsend(done, &m, MSG_SIZE, -1);
}

coroutine void pre_accept_check_conflict(int ch, int done){
    struct message *m;
    assert(chrecv(ch, &m, MSG_SIZE, -1) >= 0);
    assert(m->id.instance_id == 1);
    assert(m->id.replica_id == 0);
    assert(m->type == PRE_ACCEPT_REPLY);
    assert(m->seq == 2);
    chsend(done, &m, MSG_SIZE, -1);
}

void simple_conflict(){
    struct replica r1;
    struct replica r2;
    struct message *m1 = create_message(PHASE1, "10", "20", WRITE);
    struct message *m2 = create_message(PHASE1, "06", "18", WRITE);
    int done_ch[2];
    assert(chmake(done_ch) >= 0);
    int b = bundle();
    r1.id = 0;
    r2.id = 1;
    assert(new_replica(&r1) == 0);
    assert(new_replica(&r2) == 0);
    assert(bundle_go(b, phase1_check(r1.chan_io[0], done_ch[0])) >= 0);
    assert(propose(&r1, m1) == 0);
    assert(propose(&r2, m2) == 0);
    assert(chrecv(done_ch[1], &m1, MSG_SIZE, -1) >= 0);
    assert(bundle_go(b, pre_accept_check_conflict(r2.chan_io[0], done_ch[0])) >= 0);
    struct message *m3 = copy_message(m1);
    assert(step(&r2, m3) == 0);
    assert(chrecv(done_ch[1], &m3, MSG_SIZE, -1) >= 0);
    assert(bundle_go(b, pre_accept_reply_check(r1.chan_io[0], done_ch[0])) >= 0);
    assert(step(&r1, m3) == 0);
    assert(chrecv(done_ch[1], &m3, MSG_SIZE, -1) >= 0);
    destroy_replica(&r1);
    destroy_replica(&r2);
    hclose(done_ch[0]);
    hclose(done_ch[1]);
    hclose(b);
    free(m1);
    free(m2);
    free(m3);
}
int main(){
    no_conflict();
    simple_conflict();
    return 0;
}
