/**
 * File   : src/epaxos.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : di 09 okt 2018 09:28
 */

#include "replica.h"

void run(struct replica*);
uint64_t dh_key(uint64_t, size_t);
