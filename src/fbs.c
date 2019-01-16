/**
 * File   : src/fbs.c
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

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "fbs.h"

#define cont(ptr, type, member) \
    ((type*)(((char*) ptr) - offsetof(type, member)))

static const int fbs_type_placeholder = 0;
static const void *fbs_type = &fbs_type_placeholder;


static void *fbs_hquery(struct hvfs *hvfs, const void *id);
static void fbs_hclose(struct hvfs *hvfs);
static int fbs_msendl(struct msock_vfs *mvfs,
        struct iolist *first, struct iolist *last, int64_t deadline);
static ssize_t fbs_mrecvl(struct msock_vfs *mvfs,
        struct iolist *first, struct iolist *last, int64_t deadline);

int fbs_attach(int u, struct fbs *fbs) {
    int err;
    fbs->hvfs.query = fbs_hquery;
    fbs->hvfs.close = fbs_hclose;
    fbs->mvfs.msendl = fbs_msendl;
    fbs->mvfs.mrecvl = fbs_mrecvl;
    fbs->bigendian = !(0 & LITTLE_ENDIAN);
    fbs->u = hown(u);
    fbs->senderr = 0;
    fbs->recverr = 0;
    fbs->senddone = 0;
    fbs->recvdone = 0;
    flatcc_builder_init(&fbs->b);
    int rc = bsend(fbs->u, &fbs->replica_id, 1, fbs->deadline);
    if(rc < 0) {err = errno; goto error1;}
    rc = brecv(fbs->u, &fbs->xreplica_id, 1, fbs->deadline);
    if(rc < 0) {err = errno; goto error1;}

    int h = hmake(&fbs->hvfs);
    if(h < 0) {err = errno; goto error1;}
    return h;
error1:
    hclose(fbs->u);
    free(fbs);
    errno = err;
    return -1;
}

int fbs_detach(int h, int64_t deadline) {
    int err;
    struct fbs *self = hquery(h, fbs_type);
    if(!self) return -1;
    if(self->senderr || self->recverr) {err = ECONNRESET; goto error;}
    int u = self->u;
    free(self);
    return u;
error:
    fbs_hclose(&self->hvfs);
    errno = err;
    return -1;
}

static void *fbs_hquery(struct hvfs *hvfs, const void *type) {
    struct fbs *self = (struct fbs*)hvfs;
    if(type == msock_type) return &self->mvfs;
    if(type == fbs_type) return self;
    errno = ENOTSUP;
    return NULL;
}

static void fbs_hclose(struct hvfs *hvfs) {
    struct fbs *self = (struct fbs*)hvfs;
}

static int fbs_msendl(struct msock_vfs *mvfs,
        struct iolist *first, struct iolist *last, int64_t deadline) {
    struct fbs *self = cont(mvfs, struct fbs, mvfs);
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
    if(rc < 0) {self->senderr = 1; return -1;}
    return 0;
}

static ssize_t fbs_mrecvl(struct msock_vfs *mvfs,
        struct iolist *first, struct iolist *last, int64_t deadline) {
    void* decoded = first->iol_base;
    struct fbs *self = cont(mvfs, struct fbs, mvfs);
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
    char sized[sz];
    first->iol_len = sz;
    first->iol_base = sized;
    first->iol_next = NULL;
    rc = dill_brecvl(self->u, first, last, deadline);
    if(rc < 0) {self->recverr = 1; return -1;}
    if(self->decode(decoded, first->iol_base) < 0) return -1;
    first->iol_base = decoded;
    return sz;
}
