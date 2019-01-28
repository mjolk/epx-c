/**
 * File   : fbs_sock.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : za 26 jan 2019 05:53
 */

#include "../src/fbs_sock.h"
#include <assert.h>
#include "fbs/tepx_builder.h"
#include "fbs/tepx_reader.h"
#include <stdio.h>

#undef tns
#define tns(x) FLATBUFFERS_WRAP_NAMESPACE(tepx, x)
#define ID 20
#define MSGSIZE sizeof(struct test_message)

const char *msgstr = "test for uid string";
const char *localhost = "127.0.0.1";

struct test_message {
    char id[ID];
    size_t replica_id;
};

int tdec(void *im, void *buf){
    struct test_message *m = im;
    tns(test_message_table_t) tm = tns(test_message_as_root(buf));
    assert(tm != 0);
    strncpy(m->id, tns(test_message_id(tm)), ID);
    m->replica_id = tns(test_message_replica_id(tm));
    return 0;
}

void tenc(void *im, flatcc_builder_t *b){
    struct test_message *m = im;
    tns(test_message_start_as_root(b));
    tns(test_message_id_create_str(b, m->id));
    tns(test_message_replica_id_add(b, m->replica_id));
    tns(test_message_end_as_root(b));
}

struct fbs_sock *new_fbs_sock(size_t rid){
    struct fbs_sock *sock = malloc(sizeof(struct fbs_sock));
    assert(sock != 0);
    sock->decode = tdec;
    sock->encode = tenc;
    sock->deadline = -1;
    sock->replica_id = rid;
    sock->xreplica_id = 0;
    sock->hdrlen = 16;
    return sock;
}

coroutine void ipc_client(int s){
    struct test_message *m = malloc(MSGSIZE);
    strncpy(m->id, msgstr, ID);
    m->replica_id = 1;
    struct fbs_sock *sock = new_fbs_sock(2);
    assert(sock != 0);
    int fbs = fbs_sock_attach(s, sock);
    assert(fbs >= 0);
    size_t remote_replica = fbs_sock_remote_id(fbs);
    assert(remote_replica == 1);
    int rc = msend(fbs, m, MSGSIZE, -1);
    assert(rc == 0);
    s = fbs_sock_detach(fbs, -1);
    assert(s >= 0);
    rc = hclose(s);
    assert(rc == 0);
    rc = hclose(fbs);
    assert(rc == 0);
    free(sock);
    free(m);
}

void ipc_test(){
    int ss[2];
    int rc = ipc_pair(ss);
    assert(rc == 0);
    int b = bundle();
    bundle_go(b, ipc_client(ss[0]));
    struct fbs_sock *sock = new_fbs_sock(1);
    assert(sock != 0);
    int fbs = fbs_sock_attach(ss[1], sock);
    assert(fbs >= 0);
    size_t remote_replica = fbs_sock_remote_id(fbs);
    assert(remote_replica == 2);
    struct test_message tm = {
        .id = "",
        .replica_id = 0
    };
    ssize_t sz = mrecv(fbs, (void*)&tm, MSGSIZE, -1);
    assert(sz >= 0);
    assert(strncmp(tm.id, msgstr, ID) == 0);
    assert(tm.replica_id == 1);
    hclose(b);
    int s = fbs_sock_detach(fbs, -1);
    assert(s >= 0);
    rc = hclose(s);
    assert(rc == 0);
    free(sock);
}

coroutine void tcp_client(int port){
    struct test_message *m = malloc(MSGSIZE);
    strncpy(m->id, msgstr, ID);
    m->replica_id = 1;
    struct fbs_sock *sock = new_fbs_sock(2);
    struct ipaddr addr;
    int rc = ipaddr_remote(&addr, localhost, port, 0, -1);
    assert(rc == 0);
    int cs = tcp_connect(&addr, -1);
    assert(cs >= 0);
    int fbs = fbs_sock_attach(cs, sock);
    assert(fbs >= 0);
    size_t remote_replica = fbs_sock_remote_id(fbs);
    assert(remote_replica == 1);
    rc = msend(fbs, m, MSGSIZE, -1);
    assert(rc == 0);
    cs = fbs_sock_detach(fbs, -1);
    assert(cs >= 0);
    rc = tcp_close(cs, -1);
    assert(rc < 0);
    rc = hclose(fbs);
    assert(rc >= 0);
    free(sock);
    free(m);
}

void tcp_test(){
    int b = bundle();
    struct ipaddr addr;
    int rc = ipaddr_local(&addr, NULL, 5555, 0);
    assert(rc == 0);

    int ls = tcp_listen(&addr, 10);
    assert(ls >= 0);
    bundle_go(b, tcp_client(5555));
    int as = tcp_accept(ls, NULL, -1);
    assert(as >= 0);
    struct fbs_sock *sock = new_fbs_sock(1);
    assert(sock != 0);
    int fbs = fbs_sock_attach(as, sock);
    assert(fbs >= 0);
    size_t remote_replica = fbs_sock_remote_id(fbs);
    assert(remote_replica == 2);
    struct test_message tm = {
        .id = "",
        .replica_id = 0
    };
    ssize_t sz = mrecv(fbs, (void*)&tm, MSGSIZE, -1);
    assert(sz >= 0);
    assert(strncmp(tm.id, msgstr, ID) == 0);
    assert(tm.replica_id == 1);
    as = fbs_sock_detach(fbs, -1);
    assert(as >= 0);
    tcp_close(as, -1);
    hclose(ls);
    hclose(fbs);
    hclose(b);
    free(sock);
}

int main(){
    ipc_test();
    tcp_test();
    return 0;
}
