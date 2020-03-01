/**
 * File   : instance.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : thu 13 jun 2019 18:50
 */
#include "../src/io.c"

struct message* test_message(
    char *start_key,
    char *end_key,
    enum message_type t
){
    struct command *cmd = (struct command*)malloc(sizeof(struct command));
    if(!cmd) return 0;
    strcpy(cmd->spans[0].start_key, start_key);
    strcpy(cmd->spans[0].end_key, end_key);
    memset(cmd->spans[0].max, 0, KEY_SIZE);
    struct message *m = (struct message*)malloc(sizeof(struct message));
    if(!m) return 0;
    m->type = t;
    m->command = cmd;
    m->command->tx_size = 1;
    return m;
}

coroutine void test_client_client(struct connection *conn){
    struct message *m = test_message("ab", "cd", PHASE1);
    init_fbs(conn);
    struct ipaddr addr;
    assert(ipaddr_local(&addr, "127.0.0.1", CLIENT_PORT, 0) == 0);
    int tcp_handle = tcp_connect(&addr, -1);
    perror("error connecting");
    assert(tcp_handle > 0);
    conn->handle = fbs_sock_attach(tcp_handle, &conn->fbs);
    assert(conn->handle > 0);
    int rc = 0;
    rc = msend(conn->handle, m, sizeof(struct message), -1);
    assert(rc == 0);
    free(m);
}

coroutine void test_client_update(struct connection *conn){
    struct command cmd;
    struct message reply;
    reply.command = &cmd;
    int rc = mrecv(conn->handle, &reply, sizeof(struct message), -1);
    assert(rc >= 0);
    assert(reply.type == COMMIT);
}

coroutine void test_node_update(struct connection *conn){
    struct command cmd;
    struct message step;
    step.command = &cmd;
    int rc = mrecv(conn->handle, &step, sizeof(struct message), -1);
    assert(rc >= 0);
    assert(step.type == ACCEPT_REPLY);
}

coroutine void test_node_client(struct connection *conn){
    struct node_io remote_node;
    remote_node.node_id = 2;
    conn->n = &remote_node;
    init_fbs(conn);
    struct ipaddr addr;
    assert(ipaddr_local(&addr, "127.0.0.1", NODE_PORT, 0) == 0);
    int tcp_handle = tcp_connect(&addr, -1);
    assert(tcp_handle > 0);
    conn->handle = fbs_sock_attach(tcp_handle, &conn->fbs);
    assert(conn->handle > 0);
    conn->xreplica_id = fbs_sock_remote_id(conn->handle);
    assert(conn->xreplica_id == 1);
    struct message *m = test_message("ef", "gh", PHASE1);
    int rc = msend(conn->handle, m, sizeof(struct message), -1);
    assert(rc == 0);
}

coroutine void check_propose(struct node_io *io){
    struct message *m;
    while(!chan_recv_mpsc(&io->sync.chan_propose, &m)){
        msleep(now() + 500);
        continue;
    }
    assert(strcmp(m->command->spans[0].start_key, "ab") == 0);
}

coroutine void check_step(struct node_io *io){
    struct message *m;
    while(!chan_recv_mpsc(&io->sync.chan_step, &m)){
        msleep(now() + 500);
        continue;
    }
    assert(strcmp(m->command->spans[0].start_key, "ef") == 0);
}

void test_create(){
    struct node_io io;
    assert(start_io(&io) == 0);
    //sleep(1);
    stop_io(&io);
}

void test_client(){
    struct connection client;
    int b = bundle();
    struct node_io io;
    chan_init(&io.sync.chan_propose);
    chan_init(&io.io_sync.chan_eo);
    assert(start_io(&io) == 0);
    sleep(1);//wait for node to be initialized
    bundle_go(b, test_client_client(&client));
    bundle_go(b, check_propose(&io));
    bundle_wait(b, -1);
    bundle_go(b, test_client_update(&client));
    struct message *m = test_message("ab", "cd", COMMIT);
    assert(chan_send_mpsc(&io.io_sync.chan_eo, m));
    bundle_wait(b, -1);
    struct registered_span rsp;
    strcpy(rsp.start_key, "ab");
    struct registered_span *frsp = LLRB_FIND(client_index, &io.clients, &rsp);
    assert((frsp));
    assert(strcmp(frsp->start_key, "ab") == 0);
    assert(frsp->clients[0].chan > 0);
    stop_io(&io);
    hclose(b);
}

void test_node(){
    int b = bundle();
    struct connection client;
    struct node_io io;
    ipaddr_local(&io.chan_nodes[2].addr, "127.0.0.1", NODE_PORT, 0);
    io.node_id = 1;
    chan_init(&io.sync.chan_step);
    chan_init(&io.io_sync.chan_io);
    assert(start_io(&io) == 0);
    bundle_go(b, test_node_client(&client));
    bundle_go(b, check_step(&io));
    bundle_wait(b, -1);
    assert(io.chan_nodes[2].status == ALIVE);
    bundle_go(b, test_node_update(&client));
    struct message *m = test_message("ab", "cd", ACCEPT_REPLY);
    m->from = 2;
    assert(chan_send_spsc(&io.io_sync.chan_io, m));
    bundle_wait(b, -1);
    stop_io(&io);
    hclose(b);
}

int main(){
    //test_create();
    //test_client();
    test_node();
    return 0;
}
