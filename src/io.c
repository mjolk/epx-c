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
#define MAX_LATENCY 60
#define PREFIX_SIZE 4

int spcmp(struct registered_span *sp1, struct registered_span *sp2){
    return strcmp(sp1->start_key, sp2->start_key);
}

SLL_HEAD(client_group, span);
LLRB_GENERATE(client_tree, registered_span, entry, spcmp);
LLRB_RANGE_GROUP_GEN(client_tree, registered_span, entry, client_group, next);

void step_writer(struct connection*);
void step_reader(struct connection*);

//does this work, need extra pointer indirect/return new array?
void update_quorum(size_t *quorum, struct connection *ns){
    for(int ci = 0;ci < N;ci++){
        struct connection reader = ns[ci];
        for(int qi = 0; qi < N; qi++){
            if(reader.latency < ns[quorum[qi]].latency &&
                    reader.latency > ns[quorum[qi -1]].latency){
                quorum[qi] = reader.xreplica_id;
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
    c->status = ALIVE;
    bundle_go(c->ap, step_writer(c));
    bundle_go(c->ap, step_reader(c));
    return 0;
};

//TODO TLS/UDP
int listener(int port) {
    struct ipaddr addr;
    ipaddr_local(&addr, NULL, port, 0);
    return tcp_listen(&addr, 10);
}

void fd_close(int fd){
    struct linger lng;
    lng.l_onoff=1;
    lng.l_linger=0;
    setsockopt(fd, SOL_SOCKET, SO_LINGER, (void*)&lng, sizeof(lng));
    /* We are not checking the error here. close() has inconsistent behaviour
       and leaking a file descriptor is better than crashing the entire
       program. */
    close(fd);
}

int new_connection(struct connection *c){
    if(slow(c)) return -1;
    c->status = CONNECTING;
    c->fbs.deadline = 80;
    c->fbs.hdrlen = 4;
    c->fbs.decode = message_from_buffer;
    c->fbs.encode = message_to_buffer;
    chan_init(&c->chan_read);
    if(c->fd > 0){
        c->handle = tcp_fromfd(c->fd);
    } else {
        c->handle = tcp_connect(&c->addr, now() + 160);
    }
    c->handle = fbs_sock_attach(c->handle, &c->fbs);
    if(c->handle < 0 && c->fd > 0) {
        goto release_fd;
    } else if(c->handle < 0){
        goto release_handle;
    }
    size_t xreplica_id = fbs_sock_remote_id(c->handle);
    c->xreplica_id = xreplica_id;
    run_conn(c);
    return 0;
release_handle:
    hclose(c->handle);
    return -1;
release_fd:
    fd_close(c->fd);
    return -1;
}

void update_nodes(struct connection *c){
    //TODO locking
    struct connection oc = c->n->chan_nodes[c->xreplica_id];
    if(oc.status != DEAD){
        pthread_cancel(*oc.tid);
    }
    memcpy(&c->n->chan_nodes[c->xreplica_id], c, sizeof(struct connection));
}

void *try_connect(void *conn){
    struct connection *c = conn;
    int tries = 0;
    int reconnect = 3;
    int rc = 0;
    do {
        rc = new_connection(c);
        tries++;
        if(rc < 0){
            sleep(c->back_off*tries);
            continue;
        } else {
            tries = 0;
            //TODO sync
            update_nodes(c);
            bundle_wait(c->ap, -1);
        }
    } while(tries < reconnect);
    c->status = DEAD;
    pthread_exit(&c->xreplica_id);
    return 0;
}

int create_connection(struct node_io *n, int fd){
    int rc = 0;
    pthread_t id;
    struct connection nns;
    nns.chan_write = &n->chan_step;
    nns.tid = &id;
    nns.n = n;
    nns.status = CONNECTING;
    return pthread_create(&id, NULL, try_connect, &nns);
}

coroutine void node_listener(struct node_io *n){
    int ls = listener(NODE_PORT);
    if(ls <= 0) return;
    while(n->running){
        int fd = tcp_accept_raw(ls, 0, -1);
        if(create_connection(n, fd) != 0){
            msleep(now() + 200);
        }
    }
}

coroutine void client_listener(struct node_io *n){
    int ls = listener(CLIENT_PORT);
    if(ls <= 0) return;
    while(n->running){
        int chandle = tcp_accept(ls, 0, -1);
        if(chandle <= 0){
            msleep(now() + 60);
            continue;
        }
    }
}

void *client_access(void *nn){
    struct node_io *n = nn;
    
    bundle_go(n->ap, client_listener(n));
}

int start(struct node_io *n){
    n->running = 1;
    n->ap = bundle();
    if(n->ap < 0) goto error;
    int rc = 0;
    return 0;
error:
    n->running = 0;
    perror("error..todo");
    return -1;
}

void stop(struct node_io *n){
    hclose(n->ap);
}

int write_step(struct connection *c, struct message *m){
    return chan_send_mpsc(c->chan_write, m);
}

int read_step(struct connection *c, struct message *m){
    if(!chan_recv_spsc(&c->chan_read, &m)){
        return -1;
    }
    return 0;
}
int write_propose(struct node_io *n, struct message *m){
    return chan_send_mpsc(&n->sync->chan_propose, m);
}

int write_message(struct connection *c, struct message *m){
    return msend(c->handle, m, sizeof(struct message), 40);
}

ssize_t read_message(struct connection *c, struct message *m){
    ssize_t s;
    s = mrecv(c->handle, m, sizeof(struct message), 60);
    if(s <= 0){
        return -1;
    }
    //only alloc when we positively have a message
    struct message *nm = malloc(sizeof(struct message));
    if(nm == 0){ errno = ENOMEM; return -1;}
    *nm = *m;
    m = nm;
    return s;
}

coroutine void step_writer(struct connection *c){
    int rc = 0;
    struct message m;//temp storage for message to be received
    struct message *rcv;
    while(c->status == ALIVE){
        memset(&m, 0, sizeof(struct message));
        rcv = &m;
        ssize_t s = read_message(c, rcv);
        if(s <= 0){
            msleep(now() + 20);
            continue;
        }
        rc = write_step(c, rcv);
        if(rc <= 0) msleep(now() + 20);//TODO log lost messages.
    }
}

coroutine void step_reader(struct connection *c){
    int rc = 0;
    struct message *m = 0;
    while(c->status == ALIVE){
        int s = read_step(c, m);
        if(s <= 0){
            msleep(now() + 20);
            continue;
        }
        rc = write_message(c, m);
        if(rc <= 0) msleep(now() + 20);//TODO log lost messages.
    }
}

int connection_write(struct node_io *n, struct message *m){
    if(!chan_send_spsc(&n->chan_nodes[m->to].chan_read, m)){
        //TODO raise error
        return -1;
    }
    return 0;
}

void broadcast_prepare(struct node_io *n, struct message *m){
    int nN = N -1;
    size_t q = n->node_id;
    for(int s = 0;s < nN;s++){
        q = (q+1) % N;
        if(q == n->node_id) break;
        m->to = q;
        connection_write(n, m);
    }
}

void broadcast_pre_accept(struct node_io *n, struct message *m){
    int nN = N -1;
    for(int s = 0; s < N -1; s++){
        m->to = n->quorum[s];
        connection_write(n, m);
    }
}

void broadcast_try_pre_accept(struct node_io *n, struct message *m){
    int nN = N -1;
    for(int s = 0; s < N -1; s++){
        m->to = s;
        connection_write(n, m);
    }
}

void broadcast_commit(struct node_io *n, struct message *m){
    for(int s = 0; s < N; s++){
        m->to = n->quorum[s];
        connection_write(n, m);
    }
}

int read_io(struct node_io *n, struct message *m){
    if(!chan_recv_mpsc(&n->io_sync.chan_io, &m)){
        return -1;
    }
    return 0;
}

//to other nodes
coroutine void io_reader(struct node_io *n){
    struct message *m = 0;
    while(n->running){
        if(read_io(n, m) <= 0){
            msleep(now() + 20);
            continue;
        }
        m->to = m->from;
        m->from = n->node_id;
        switch(m->type){
            case PREPARE:
                broadcast_prepare(n, m);
                break;
            case ACCEPT:
            case PRE_ACCEPT:
                update_quorum(n->quorum, n->chan_nodes);
                broadcast_pre_accept(n, m);
                break;
            case TRY_PRE_ACCEPT:
                broadcast_try_pre_accept(n, m);
                break;
            case COMMIT:
                broadcast_commit(n, m);
                break;
            default:
                connection_write(n, m);
                break;

        }
    }
}
