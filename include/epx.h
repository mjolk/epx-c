/**
 * File   : epx.h.in
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : Wed 05 Sep 2018 04:29
 */
//compile time configured options and settings for epx
#ifndef epx_h
#define epx_h
#define epx_VERSION_MAJOR 1
#define epx_VERSION_MINOR 0
#define N 3
#define VALUE_SIZE 1024
#define KEY_SIZE 3 //includes \0 terminator!
#define MAX_DEPS 8
#define TX_SIZE 100
#define CLIENT_PORT 5552
#define NODE_PORT 5551
#define MAX_LATENCY 60
#define PREFIX_SIZE 4
#define MAX_CLIENTS 10 //max clients per subscribed range.
#define DB_PATH "/tmp/epx.db"
#endif
