/**
 * File   : instance.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : thu 13 jun 2019 18:50
 */
#include "../src/io.c"

coroutine void test_client_client(){
    struct connection conn;
    init_fbs(&conn);
    struct command cmd;
    strcpy(cmd.spans[0].start_key, "ab");
    strcpy(cmd.spans[0].end_key, "cd");
    memset(cmd.spans[0].max, 0, KEY_SIZE);
    struct message m;
    m.type = PHASE1;
    m.command = &cmd;
    struct ipaddr addr;
    assert(ipaddr_remote(&addr, "127.0.0.1", CLIENT_PORT, 0, -1) == 0);
    int tcp_handle = tcp_connect(&addr, -1);
    assert(tcp_handle > 0);
    conn.handle = fbs_sock_attach(tcp_handle, &conn.fbs);
    assert(conn.handle > 0);
    int rc = msend(conn.handle, &m, sizeof(struct message), -1);
    assert(rc == 0);
}

coroutine void test_node_client(){
    struct node_io remote_node;
    remote_node.node_id = 2;
    struct connection conn;
    conn.n = &remote_node;
    init_fbs(&conn);
    struct ipaddr addr;
    assert(ipaddr_remote(&addr, "127.0.0.1", NODE_PORT, 0, -1) == 0);
    int tcp_handle = tcp_connect(&addr, -1);
    assert(tcp_handle > 0);
    conn.handle = fbs_sock_attach(tcp_handle, &conn.fbs);
    assert(conn.handle > 0);
    conn.xreplica_id = fbs_sock_remote_id(conn.handle);
    assert(conn.xreplica_id == 1);
    struct command cmd;
    strcpy(cmd.spans[0].start_key, "ef");
    strcpy(cmd.spans[0].end_key, "gh");
    memset(cmd.spans[0].max, 0, KEY_SIZE);
    struct message m;
    m.type = PHASE1;
    m.command = &cmd;
    int rc = msend(conn.handle, &m, sizeof(struct message), -1);
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
    assert(start(&io) == 0);
    stop(&io);
}

void test_client(){
    int b = bundle();
    struct node_io io;
    chan_init(&io.sync.chan_propose);
    assert(start(&io) == 0);
    bundle_go(b, test_client_client());
    bundle_go(b, check_propose(&io));
    bundle_wait(b, -1);
    struct registered_span rsp;
    strcpy(rsp.start_key, "ab");
    struct registered_span *frsp = LLRB_FIND(client_index, &io.clients, &rsp);
    assert((frsp));
    assert(strcmp(frsp->start_key, "ab") == 0);
    assert(frsp->clients[0].chan > 0);
    stop(&io);
}

void test_node(){
    int b = bundle();
    struct node_io io;
    io.node_id = 1;
    chan_init(&io.sync.chan_step);
    assert(start(&io) == 0);
    bundle_go(b, test_node_client());
    bundle_go(b, check_step(&io));
    bundle_wait(b, -1);
    assert(io.chan_nodes[2].status == ALIVE);
    stop(&io);
}

int main(){
    test_create();
    test_client();
    test_node();
    return 0;
}