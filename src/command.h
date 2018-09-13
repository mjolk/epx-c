/**
 * File   : command.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : di 11 sep 2018 08:25
 */
#include <stdint.h>
#include <stdlib.h>
#include "fbs/epx_reader.h"

enum io_t {
	READ,
	WRITE
} io_t;

struct span {
	size_t start;
	size_t end;
};


struct command {
	struct span span;
	int id;
	enum io_t writing;
	uint8_t value[1024];
};

//uint64_t command_seq_deps(struct command *c, struct instance_id ids)
