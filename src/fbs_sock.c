/**
 * File   : fbs_sock.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : wo 10 okt 2018 17:03
 */
/*

   Copyright (c) 2017 Martin Sustrik

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom
   the Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   IN THE SOFTWARE.

*/

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "fbs_sock.h"

#define cont(ptr, type, member) \
    ((type*)(((char*) ptr) - offsetof(type, member)))

static const int fbs_sock_type_placeholder = 0;
static const void *fbs_sock_type = &fbs_sock_type_placeholder;


static void *fbs_sock_hquery(struct hvfs *hvfs, const void *id);
static void fbs_sock_hclose(struct hvfs *hvfs);
static int fbs_sock_msendl(struct msock_vfs *mvfs,
        struct iolist *first, struct iolist *last, int64_t deadline);
static ssize_t fbs_sock_mrecvl(struct msock_vfs *mvfs,
        struct iolist *first, struct iolist *last, int64_t deadline);

int fbs_sock_attach(int u, struct fbs_sock *s) {
    int err;
    s->hvfs.query = fbs_sock_hquery;
    s->hvfs.close = fbs_sock_hclose;
    s->mvfs.msendl = fbs_sock_msendl;
    s->mvfs.mrecvl = fbs_sock_mrecvl;
    s->bigendian = !(0 & LITTLE_ENDIAN);
    s->u = hown(u);
    s->senderr = 0;
    s->recverr = 0;
    s->senddone = 0;
    s->recvdone = 0;
    flatcc_builder_init(&s->b);
    int rc = bsend(s->u, &s->replica_id, 1, s->deadline);
    if(rc < 0) {err = errno; goto sock_error;}
    rc = brecv(s->u, &s->xreplica_id, 1, s->deadline);
    if(rc < 0) {err = errno; goto sock_error;}
    int h = hmake(&s->hvfs);
    if(h < 0) {err = errno; goto sock_error;}
    return h;
sock_error:
    flatcc_builder_clear(&s->b);
    hclose(s->u);
    errno = err;
    return -1;
}

size_t fbs_sock_remote_id(int h){
    struct fbs_sock *self = hquery(h, fbs_sock_type);
    if(!self) return -1;
    return self->xreplica_id;
}

int fbs_sock_detach(int h, int64_t deadline) {
    int err;
    struct fbs_sock *self = hquery(h, fbs_sock_type);
    if(!self) return -1;
    if(self->senderr || self->recverr) {err = ECONNRESET; goto error;}
    int u = self->u;
    return u;
error:
    fbs_sock_hclose(&self->hvfs);
    errno = err;
    return -1;
}

static void *fbs_sock_hquery(struct hvfs *hvfs, const void *type) {
    struct fbs_sock *self = (struct fbs_sock*)hvfs;
    if(type == msock_type) return &self->mvfs;
    if(type == fbs_sock_type) return self;
    errno = ENOTSUP;
    return NULL;
}

static void fbs_sock_hclose(struct hvfs *hvfs) {
    struct fbs_sock *self = (struct fbs_sock*)hvfs;
    flatcc_builder_clear(&self->b);
}

static int fbs_sock_msendl(struct msock_vfs *mvfs,
        struct iolist *first, struct iolist *last, int64_t deadline) {
    struct fbs_sock *self = cont(mvfs, struct fbs_sock, mvfs);
    if(self->senderr) {errno = ECONNRESET; return -1;}
    if(self->senddone) {errno = EPIPE; return -1;}
    void* msg = first->iol_base;
    flatcc_builder_reset(&self->b);
    self->encode(msg, &self->b);
    first->iol_base = flatcc_builder_finalize_aligned_buffer(&self->b,
            &first->iol_len);
    uint8_t szbuf[self->hdrlen];
    size_t sz = 0;
    struct iolist *it;
    for(it = first; it; it = it->iol_next)
        sz += it->iol_len;
    for(size_t i = 0; i != self->hdrlen; ++i) {
        szbuf[self->bigendian ? (self->hdrlen - i - 1) : i] = sz & 0xff;
        sz >>= 8;
    }
    struct iolist hdr = {szbuf, sizeof(szbuf), first, 0};
    int rc = bsendl(self->u, &hdr, last, deadline);
    free(first->iol_base);
    if(rc < 0) {self->senderr = 1; return -1;}
    return 0;
}

static ssize_t fbs_sock_mrecvl(struct msock_vfs *mvfs,
        struct iolist *first, struct iolist *last, int64_t deadline) {
    void* decoded = first->iol_base;
    struct fbs_sock *self = cont(mvfs, struct fbs_sock, mvfs);
    if(self->recverr) {errno = ECONNRESET; return -1;}
    if(self->recvdone) {errno = EPIPE; return -1;}
    uint8_t szbuf[self->hdrlen];
    int rc = brecv(self->u, &szbuf, self->hdrlen, deadline);
    if(rc < 0) {self->recverr = 1; return -1;}
    uint64_t sz = 0;
    size_t i;
    for(i = 0; i != self->hdrlen; ++i) {
        uint8_t c = szbuf[self->bigendian ? i : (self->hdrlen - i - 1)];
        sz <<= 8;
        sz |= c;
    }
    char vla[sz];// VLA, should be per socket heap buffer...
    first->iol_len = sz;
    first->iol_base = vla;
    first->iol_next = NULL;
    rc = dill_brecvl(self->u, first, last, deadline);
    if(rc < 0) {self->recverr = 1; return -1;}
    if(self->decode(decoded, first->iol_base) < 0) return -1;
    first->iol_base = decoded;
    return sz;
}
