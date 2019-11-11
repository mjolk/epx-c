/**
 * File   : node_io.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : ma 11 feb 2019 12:17
 */

#include <sys/socket.h>
#include <stdio.h>
#include "io.h"
#include "llrb-interval/slist.h"
#include "llrb-interval/llrb-interval.h"

int is_conn_status(
    struct connection *c,
    const enum connection_status t
){
    enum connection_status s = (enum connection_status)ck_pr_load_int(
        (int*)&c->status);
    ck_pr_fence_load();
    return (s == t);
}

void set_conn_status(
    struct connection *c,
    const int t
){
    ck_pr_fas_int((int*)&c->status, t);
}

int rspcmp(struct registered_span *sp1, struct registered_span *sp2){
    return strcmp(sp1->start_key, sp2->start_key);
}

SLL_HEAD(client_group, registered_span);
LLRB_GENERATE(client_index, registered_span, entry, rspcmp);
LLRB_RANGE_GROUP_GEN(client_index, registered_span, entry, client_group, next);

void merge_clients(
    struct client_index *ci,
    struct registered_span *to_merge,
    struct client_group *sll
){
    struct registered_span *c, *prev, *nxt;
    nxt = SLL_FIRST(sll);
    SLL_INSERT_HEAD(sll, to_merge, next);
    if(!nxt) return;
    prev = SLL_FIRST(sll);
    while(nxt) {
        c = nxt;
        nxt = SLL_NEXT(nxt, next);
        if((strcmp(to_merge->start_key, c->end_key) <= 0) &&
            (strcmp(to_merge->end_key, c->start_key) >= 0)){
            if((strcmp(to_merge->start_key, c->start_key) > 0)){
                strcpy(to_merge->start_key, c->start_key);
            }
            if(strcmp(to_merge->end_key, c->end_key) < 0) {
                strcpy(to_merge->end_key, c->end_key);
            }
            for(int cl = 0;cl < MAX_CLIENTS;cl++){
                if(c->clients[cl].expires != 0){
                    for(int tcl = 0;tcl < MAX_CLIENTS;tcl++){
                        if(to_merge->clients[tcl].expires == 0){
                            to_merge->clients[tcl] = c->clients[cl];
                        }
                    }
                }
            }
            SLL_REMOVE_AFTER(prev, next);
            LLRB_DELETE(client_index, ci, c);
        }else{
            prev = c;
        }
    }
}

void find_clients(
    struct client_index *ci,
    struct registered_span *to_merge,
    struct client_group *sll
){
    SLL_INSERT_HEAD(sll, to_merge, next);
}

void writer(struct connection*);
void reader(struct connection*);

void update_quorum(size_t *quorum, struct connection *ns){
    for(int ci = 0;ci < N;ci++){
        struct connection c = ns[ci];
        if(c.status != ALIVE) continue;
        for(int qi = 0; qi < N; qi++){
            if(c.latency < ns[quorum[qi]].latency &&
                    c.latency > ns[quorum[qi -1]].latency){
                quorum[qi] = c.xreplica_id;
            }
        }
    }
}

int slow(struct connection *c){
    return (c->latency >= MAX_LATENCY);
}

int expired(struct connection *c){
    return (c->fbs.deadline < now());
}

int run_conn(struct connection *c){
    set_conn_status(c, ALIVE);
    bundle_go(c->ap, writer(c));
    bundle_go(c->ap, reader(c));
    return 0;
}

//TODO TLS/UDP
int listener(int port) {
    struct ipaddr addr;
    ipaddr_local(&addr, NULL, port, 0);
    return tcp_listen(&addr, 10);
}

void fd_closer(int fd){
    struct linger lng;
    lng.l_onoff=1;
    lng.l_linger=0;
    setsockopt(fd, SOL_SOCKET, SO_LINGER, (void*)&lng, sizeof(lng));
    /* We are not checking the error here. close() has inconsistent behaviour
       and leaking a file descriptor is better than crashing the entire
       program. */
    close(fd);
}

void destroy_client_connection(struct connection *c){
    hclose(c->handle);
    tcp_close(c->prot_handle, 20);
    hclose(c->client.chan[0]);
    hclose(c->client.chan[1]);
    hclose(c->ap);
    set_conn_status(c, DEAD);
    if(c->fd > 0) fd_closer(c->fd);
    free(c);
}

void destroy_node_connection(struct connection *c){
    hclose(c->handle);
    hclose(c->prot_handle);
    hclose(c->ap);
    set_conn_status(c, DEAD);
    if(c->fd > 0) fd_closer(c->fd);
}

void init_fbs(struct connection *c){
    c->fbs.deadline = -1;
    c->fbs.hdrlen = 16;
    c->fbs.decode = message_from_buffer;
    c->fbs.encode = message_to_buffer;
    if(c->n) c->fbs.replica_id = c->n->node_id;
    c->ap = bundle();
    c->latency = 0;
    c->back_off = 1;
    c->p = TCP;
    chan_init(&c->chan_read);
}

int run_node_fbs(struct connection *c){
    c->handle = fbs_sock_attach(c->prot_handle, &c->fbs);
    if(c->handle < 0 && c->fd > 0) {
        fd_closer(c->fd);
        return -1;
    } else if(c->handle < 0){
        hclose(c->handle);
        return -1;
    }
    c->xreplica_id = fbs_sock_remote_id(c->handle);
    run_conn(c);
    return 0;
}

int run_client_fbs(struct connection *c){
    c->handle = fbs_sock_attach(c->prot_handle, &c->fbs);
    if(c->handle < 0){
        hclose(c->prot_handle);
        hclose(c->handle);
        return -1;
    }
    run_conn(c);
    return 0;
}

int new_node_connection(struct connection *c){
    init_fbs(c);
    if(c->fd > 0){
        c->prot_handle = tcp_fromfd(c->fd);
    } else {
        c->prot_handle = tcp_connect(&c->addr, now() + 160);
    }
    if(c->prot_handle <= 0){
        return -1;
    }
    c->chan_write = &c->n->sync.chan_step;
    return run_node_fbs(c);
}

void* try_connect(void *conn){
    struct connection *c = (struct connection*)conn;
    int tries = 0;
    int reconnect = 3;
    do {
        int rc = new_node_connection(c);
        tries++;
        if(rc < 0){
            sleep(c->back_off*tries);
            continue;
        }
        c->error = 0;
        c->back_off = 2;
        c->latency = 0;
        bundle_wait(c->ap, -1);
        reconnect = 0;
    } while(tries < reconnect);
    if(reconnect == 3){
        c->back_off *= 3;
    }
    destroy_node_connection(c);
    pthread_exit(NULL);
    return 0;
}

struct connection* get_conn(struct node_io *n, const struct ipaddr *addr){
    for(size_t i = 0;i < N;i++){
        const struct ipaddr e_addr = n->chan_nodes[i].addr;
        char buf1[IPADDR_MAXSTRLEN];
        ipaddr_str(addr, buf1);
        printf("connected from ip: %s\n", buf1);
        char buf2[IPADDR_MAXSTRLEN];
        ipaddr_str(&e_addr, buf2);
        printf("node connection list: %s\n", buf2);
        if(ipaddr_equal(&e_addr, addr, 1)){
            return &n->chan_nodes[i];
        }
    }
    return 0;
}

int node_connect(struct node_io *n, struct ipaddr *addr, int fd){
    struct connection *c = get_conn(n, addr);
    if(!c) return -1;
    if(!is_conn_status(c, DEAD)){
        destroy_node_connection(c);
        pthread_cancel(c->tid);
    }
    set_conn_status(c, CONNECTING);
    c->n = n;
    c->fd = fd;
    return pthread_create(&c->tid, &n->tattr, try_connect, c);
}

coroutine void node_listener(struct node_io *n){
    n->node_listener = listener(NODE_PORT);
    if(n->node_listener <= 0) return;
    struct ipaddr addr;
    while(n->running){
        int fd = tcp_accept_raw(n->node_listener, &addr, -1);
        if(fd > 0){
            if(node_connect(n, &addr, fd) != 0){
                msleep(now() + 400);//TODO reason this, arbitrary solution now
            }
        }
    }
}

coroutine void run_client(struct connection *c){
    struct message *m;
    while(c->n->running){
        if(chrecv(c->client.chan[0], &m, MSG_SIZE, -1) == 0){
            if(!chan_send_spsc(&c->chan_read, m)){
                //error could not send to client
                break;
            }
        }
    }
    destroy_client_connection(c);
}

coroutine void client_listener(struct node_io *n){
    n->client_listener = listener(CLIENT_PORT);
    if(n->client_listener <= 0) return;
    while(n->running){
        int c_handle = tcp_accept(n->client_listener, NULL, -1);
        if(c_handle <= 0){
            perror("connection error client_register");
            //yield();
            msleep(now() + 60);
            continue;
        }
        struct connection *nns = (struct connection*)malloc(
            sizeof(struct connection));
        if(!nns) exit(1);//TODO crash and burn
        nns->chan_write = &n->sync.chan_propose;
        nns->n = n;
        nns->prot_handle = c_handle;
        nns->client.expires = now() + (int64_t)3.6e+6;
        if(chmake(nns->client.chan) < 0){
            //can't create client pipeline
            //exit or continue and log error?
        }
        init_fbs(nns);
        run_client_fbs(nns);
        bundle_go(n->ap_client, run_client(nns));
    }
}

int set_pthread_attr(struct node_io *n){
    int ret = pthread_attr_init(&n->tattr);
    if(ret) return ret;
    return pthread_attr_setdetachstate(&n->tattr, PTHREAD_CREATE_DETACHED);
}

int write_message(struct connection *c, struct message *m){
    int ret = msend(c->handle, m, sizeof(struct message), now() + 20);
    destroy_message(m);
    if(ret) set_conn_status(c, DEAD);
    return ret;
}

ssize_t read_message(struct connection *c, struct message *m){
    ssize_t s = mrecv(c->handle, m, sizeof(struct message), -1);
    if(s <= 0){
        free(m->command);
        free(m);
        set_conn_status(c, DEAD);
        return -1;
    }
    m->ref = 1;
    return s;
}

int empty_rrange(struct registered_span *rsp){
    if(rsp->start_key[0] == '\0') return 1;
    return 0;
}

void register_client(struct registered_span rsps[], struct node_io *n){
    struct client_group cg;
    for(int i = 0;i < TX_SIZE;i++){
        if(empty_rrange(&rsps[i])) break;
        if(LLRB_RANGE_GROUP_ADD(client_index, &n->clients,
            &rsps[i], &cg, merge_clients)){
                struct registered_span *rsp = (struct registered_span*)malloc(
                    sizeof(struct registered_span)
                );
                *rsp = rsps[i];
                LLRB_INSERT(client_index, &n->clients, rsp);
        }
    }
}

struct message* new_message(){
    struct message *msg = (struct message*)malloc(sizeof(struct message));
    if(!msg) { errno = ENOMEM; return 0;}
    struct command *cmd = (struct command*)malloc(sizeof(struct command));
    if(!cmd) { errno = ENOMEM; return 0;}
    msg->command = cmd;
    msg->ref = 1;
    return msg;
}

coroutine void writer(struct connection *c){
    int rc = 0;
    while(is_conn_status(c, ALIVE)){
        struct message *rcv = new_message();
        if(!rcv) return;
        ssize_t s = read_message(c, rcv);
        if(s <= 0){
            //yield();
            return;
        }
        rcv->from = c->xreplica_id;
        if(c->client.expires != 0){
            struct registered_span rsps[TX_SIZE];
            for(size_t i = 0;i < rcv->command->tx_size;i++){
                if(empty_range(&rcv->command->spans[i])) break;
                strcpy(rsps[i].start_key, rcv->command->spans[i].start_key);
                strcpy(rsps[i].end_key, rcv->command->spans[i].end_key);
                rsps[i].clients[0] = c->client;
            }
            register_client(rsps, c->n);
        }
        if(!chan_send_mpsc(c->chan_write, rcv)) {
            msleep(now() + 200);//TODO log lost messages. or keep trying to send
        }
    }
}

coroutine void reader(struct connection *c){
    int rc = 0;
    struct message *m;
    while(is_conn_status(c, ALIVE)){
        if(!chan_recv_spsc(&c->chan_read, &m)){
            //yield();
            msleep(now() + 60);
            continue;
        }
        if(write_message(c, m)){
            return;
        }
    }
}

int connection_write(struct node_io *n, struct message *m){
    int ret = 0;
    if(is_conn_status(&n->chan_nodes[m->to], DEAD)){
        node_connect(n, &n->chan_nodes[m->to].addr, 0);
        return 0;//hmmm
    }
    if(!chan_send_spsc(&n->chan_nodes[m->to].chan_read, m)){
        ret = -1;
    }
    return ret;
}

int broadcast_prepare(struct node_io *n, struct message *m){
    size_t q = n->node_id;
    m->ref = N -1;
    for(int s = 0;s < N -1;s++){
        q = (q+1) % N;
        if(q == n->node_id) break;
        m->to = q;
        if(connection_write(n, m)){
            return -1;
        }
    }
    return 0;
}

int broadcast_pre_accept(struct node_io *n, struct message *m){
    m->ref = N -1;
    for(int s = 0; s < N -1; s++){
        m->to = n->quorum[s];
        if(connection_write(n, m)){
            return -1;
        }
    }
    return 0;
}

int broadcast_try_pre_accept(struct node_io *n, struct message *m){
    m->ref = N - 1;
    for(int s = 0; s < N -1; s++){
        m->to = s;
        if(connection_write(n, m)){
            return -1;
        }
    }
    return 0;
}

int broadcast_commit(struct node_io *n, struct message *m){
    m->ref = N;
    for(int s = 0; s < N; s++){
        m->to = n->quorum[s];
        if(connection_write(n, m)){
            return -1;
        }
    }
    return 0;
}

coroutine void io_reader(struct node_io *n){
    struct message *m;
    int ret;
    while(n->running){
        if(!chan_recv_spsc(&n->io_sync.chan_io, &m)){
            //yield();
            msleep(now() + 60);
            continue;
        }
        m->ref = 1;
        m->to = m->from;
        m->from = n->node_id;
        switch(m->type){
            case PREPARE:
                ret = broadcast_prepare(n, m);
                break;
            case ACCEPT:
            case PRE_ACCEPT:
                update_quorum(n->quorum, n->chan_nodes);
                ret = broadcast_pre_accept(n, m);
                break;
            case TRY_PRE_ACCEPT:
                ret = broadcast_try_pre_accept(n, m);
                break;
            case COMMIT:
                ret = broadcast_commit(n, m);
                break;
            default:
                ret = connection_write(n, m);
                break;

        }
        if(ret){
            //TODO figure out what to do here.
            return;
        }
    }
}

coroutine void eo_reader(struct node_io *n){
    struct message *m;
    while(n->running){
        if(!chan_recv_mpsc(&n->io_sync.chan_eo, &m)){
            //yield();
            msleep(now() + 60);
            continue;
        }
        m->ref = 1;
        struct client_group cg;
        for(size_t i = 0;i < m->command->tx_size;i++){
            if(empty_range(&m->command->spans[i])) break;
            struct registered_span rsp;
            strcpy(rsp.start_key, m->command->spans[i].start_key);
            strcpy(rsp.end_key, m->command->spans[i].end_key);
            LLRB_RANGE_GROUP_FIND(client_index, &n->clients,
                &rsp, &cg, find_clients);
            struct registered_span *rspf;
            SLL_FOREACH(rspf, &cg, next){
                int no_active_clients = 1;
                for(int j = 0;j < MAX_CLIENTS; j++){
                    if(rspf->clients[j].expires > now()){
                        no_active_clients = 0;
                        chsend(rspf->clients[j].chan[1], &m, MSG_SIZE, 10);
                    }
                }
                if(no_active_clients){
                    free(LLRB_DELETE(client_index, &n->clients, rspf));
                }
            }
        }
    }
    hclose(n->client_listener);
    hclose(n->ap_client);
}

void destroy_rsp(struct registered_span *rsp){
    if(rsp){
        free(rsp);
    }
}

void* client_handler(void* nio){
    struct node_io *n = (struct node_io*)nio;
    n->ap_client = bundle();
    if(bundle_go(n->ap_client, client_listener(n)) != 0) goto error;
    if(bundle_go(n->ap_client, eo_reader(n)) != 0) goto error;
    bundle_wait(n->ap_client, -1);
    pthread_exit(NULL);
error:
    pthread_exit(NULL);
}

int start_io(struct node_io *n){
    n->running = 1;
    LLRB_INIT(&n->clients);
    n->ap = bundle();
    if(n->ap < 0) goto error;
    if(set_pthread_attr(n) != 0) goto error;
    if(bundle_go(n->ap, io_reader(n)) != 0) goto error;
    if(bundle_go(n->ap, node_listener(n)) != 0) goto error;
    if(pthread_create(&n->c_tid, &n->tattr, client_handler, n) != 0) goto error;
    return 0;
error:
    n->running = 0;
    LLRB_DESTROY(client_index, &n->clients, destroy_rsp);
    perror("error..todo");
    return -1;
}

void stop_io(struct node_io *n){
    n->running = 0;
    hclose(n->ap);
    hclose(n->node_listener);
    LLRB_DESTROY(client_index, &n->clients, destroy_rsp);
}
