/**
 * File   : node_io.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : ma 11 feb 2019 12:17
 */

#include <stdio.h>
#include "node_io.h"
#include "message.h"
#define MAX_LATENCY 60
#define PREFIX_SIZE 4
#define N 3

void internal_in(struct connection*, struct node_io*, int rep);
void internal_out(struct node_io*, int rep);

int report(int ch, int *rc){
    return chsend(ch, rc, sizeof(int), 20);
}

void reset_connection(struct connection *c){
    pthread_mutex_lock(&c->lock);
    c->alive = 0;
    hclose(c->ap);
    pthread_mutex_unlock(&c->lock);
}
//does this work, need extra pointer indirect/return new array?
void update_quorum(size_t *quorum, struct connection *c){
    for(int ci = 0;ci < N;ci++){
        struct connection *con = &c[ci];
        for(int qi = 0; qi < N; qi++){
            if(con->latency < c[quorum[qi]].latency &&
                    con->latency > c[quorum[qi -1]].latency){
                quorum[qi] = con->id;
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

int connect(int rep, struct node_io *n, size_t xrid){
    struct connection *c = &n->cons[xrid];
    if(slow(c)) return -1;
    pthread_mutex_lock(&c->lock);
    hclose(c->handle);
    c->alive = 0;
    hclose(c->ap);
    c->handle = tcp_connect(&c->addr, -1);
    if(c->handle < 0) return -1;
    c->handle = fbs_sock_attach(c->handle, &c->fbs);
    if(c->handle < 0) return -1;
    if(c->fbs.xreplica_id != xrid) return -1;
    c->id = c->fbs.xreplica_id;
    if(c->p == TLS){

    }
    c->alive = 1;
    bundle_go(c->ap, internal_in(c, n, rep));
    pthread_mutex_unlock(&c->lock);
    return (c->handle >= 0)?0:-1;
}

int listen(int port) {
    struct ipaddr addr;
    ipaddr_local(&addr, NULL, port, 0);
    return tcp_listen(&addr, 10);
}

static void set_con_state(struct fbs_sock *f, size_t r_id){
    f->replica_id = r_id;
    f->hdrlen = 4;
    f->deadline = 80;
    f->encode = message_to_buffer;
    f->decode = message_from_buffer;
}

int start(struct node_io *n){
    for(int i = 0;i < N; i++){
       // set_con_state(&n->cons[i].fbs, n->r.id);
        pthread_mutex_init(&n->cons[i].lock, NULL);
        n->cons[i].ap = bundle();
    }
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

int recv_io(struct node_io *n, struct message *m){
    return chan_recv_spmc(&n->sync->chan_io, m);
}

int recv_eo(struct node_io *n, struct message *m){
    return chan_recv_spmc(&n->sync->chan_eo, m);
}

int send_cont(struct node_io *n, struct message *m){
    return chan_send_mpsc(&n->sync->chan_cont, m);
}

int send_new(struct node_io *n, struct message *m){
    return chan_send_mpsc(&n->sync->chan_new, m);
}

int write_message(int ch, struct message *m){
    return msend(ch, m, sizeof(struct message), 40);
}

ssize_t read_message(int sock, struct message *m){
    m = malloc(sizeof(struct message));
    if(m == 0){ errno = ENOMEM; return -1;}
    ssize_t s;
    if((s = mrecv(sock, m, sizeof(struct message), -1 )) && (s >= 0)){
        return s;
    }
    return -1;
}

coroutine void internal_in(struct connection *c, struct node_io *n, int rep){
    int rc = 0;
    while(c->alive){
        struct message *m;
        ssize_t s = read_message(c->handle, m);
        if(s <= 0) msleep(now() + 100);
        if(m->start){
            m->stop = now();
            c->latency = m->start - m->stop;
        }
        send_cont(n, m);
        rc = chsend(rep, &rc, sizeof(int), 20);
        if(rc < 0) return;
    }
}

coroutine void internal_listen(struct node_io *n, int port, int rep){
    int ls = listen(port);
    int ch;
    if(ls < 0) return;
    while(n->running){
        ch = tcp_accept(ls, NULL, -1);
        if(ch < 0) return;
        struct fbs_sock f;
        //set_con_state(&f, n->r.id);
        ch = fbs_sock_attach(ch, &f);
        if(ch < 0) return;
        struct connection *c = &n->cons[f.xreplica_id];
        c->id = f.xreplica_id;
        c->fbs = f;
        c->handle = ch;
        c->alive = 1;
        bundle_go(c->ap, internal_in(c, n, rep));
    }
}

//TODO check if override in protocol allows hclose on ssend routines
// eg without closing the connection
coroutine void try_send(struct node_io *n, struct message *m, int rep){
    struct connection *c = &n->cons[m->to];
    int rc = 0;
    if(!c->alive){
        rc = -1;
        goto result;
    }
    rc = write_message(c->handle, m);
    if(rc < 0){
        rc = connect(rep, n, m->to);
        if(rc < 0){
            goto result;
        }
    }
result:
    rc = chsend(rep, &rc, sizeof(int), 20);
}

coroutine void broadcast_prepare(struct node_io *n, struct message *m, int rep){
    int rc = 0;
    int nN = N -1;
    int ch[2];
    rc = chmake(ch);
    if(rc < 0){ goto report; }
    size_t q = n->r.id;
    for(int sending = 0;sending < nN;sending++){
        q = (q+1) % N;
        if(q == n->r.id) break;
        m->to = q;
        bundle_go(n->cons[q].sending, ssend(n, m, ch[1]));
    }
    int result = 0;
    int sent = 0;
    int failed = 0;
    while(sent < nN){
        rc = chrecv(ch[0], &result, sizeof(int), 20);
        if(rc < 0){ goto report; }
        if(result >= 0) {
            sent++;
        } else {
            failed++;
        }
        if(failed >= nN){ goto report; }
    }
    report(rep, &rc);
report:
    report(rep, &rc);
    //TODO cancel routines when target reached
}

coroutine void broadcast_pre_accept(struct node_io *n, struct message *m, int rep){
    int rc = 0;
    int nN = N -1;
    int ch[2];
    rc = chmake(ch);
    if(rc < 0){ goto report; }
    for(int sending = 0; sending < N -1; sending++){
        m->to = n->quorum[sending];
        bundle_go(n->cons[m->to].sending, ssend(n, m, ch[1]));
    }
    int result = 0;
    int sent = 0;
    int failed = 0;
    while(sent < nN){
        rc = chrecv(ch[0], &result, sizeof(int), 20);
        if(rc < 0){ goto report; }
        if(result >= 0){
            sent++;
        } else {
            failed++;
        }
        if(failed >= nN){ goto report;}
    }
    report(rep, &rc);
report:
    report(rep, &rc);
    //TODO cancel routines when target reached
}

coroutine void broadcast_try_pre_accept(struct node_io *n, struct message *m, int rep){
    int rc = 0;
    int nN = N -1;
    int ch[2];
    rc = chmake(ch);
    if(rc < 0) goto report;
    for(int sending = 0; sending < N -1; sending++){
        m->to = sending;
        bundle_go(n->cons[sending].sending, ssend(n, m, ch[1]));
    }
    int result = 0;
    int sent = 0;
    while(sent < N){
        rc = chrecv(ch[0], &result, sizeof(int), 20);
        if(rc < 0) goto report;
        sent++;
    }
    report(rep, &rc);
report:
    report(rep, &rc);

}

coroutine void broadcast_commit(struct node_io *n, struct message *m, int rep){
    int rc = 0;
    int ch[2];
    rc = chmake(ch);
    if(rc < 0) goto report;
    for(int sending = 0; sending < N; sending++){
        m->to = n->quorum[sending];
        bundle_go(n->cons[m->to].sending, ssend(n, m, ch[1]));
    }
    int result = 0;
    int sent = 0;
    while(sent < N){
        rc = chrecv(ch[0], &result, sizeof(int), 20);
        if(rc < 0) goto report;
        sent++;
    }
report:
    report(rep, &rc);

}

coroutine void internal_out(struct node_io *n, int rep){
    struct message *m;
    while(n->running){
        m = read_int(n);
        m->from = n->r.id;
        m->start = now();
        switch(m->type){
            case PREPARE:
                bundle_go(n->ap, broadcast_prepare(n, m, rep));
                break;
            case ACCEPT:
            case PRE_ACCEPT:
                update_quorum(n->quorum, n->cons);
                bundle_go(n->ap, broadcast_pre_accept(n, m, rep));
                break;
            case TRY_PRE_ACCEPT:
                bundle_go(n->ap, broadcast_try_pre_accept(n, m, rep));
                break;
            case COMMIT:
                bundle_go(n->ap, broadcast_commit(n, m, rep));
                break;
            default:
                bundle_go(n->ap, ssend(n, m, rep));

        }
    }
}
