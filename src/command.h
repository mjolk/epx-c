/**
 * File   : src/command.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : di 11 sep 2018 08:25
 */
#include "instance_id.h"
#include "llrb-interval/llrb.h"
#undef ns
#define ns(x) FLATBUFFERS_WRAP_NAMESPACE(epx, x) // Specified in the schema.

enum io_t {
	READ,
	WRITE
};

struct nr {
    struct span* sle_next;
};
struct range_group *merge_list;

struct span {
    LLRB_ENTRY(span) entry;
	uint64_t start_key;
	uint64_t end_key;
    uint64_t max;
    struct nr next;
};

struct command {
	struct span span;
	int id;
	enum io_t writing;
	uint8_t value[VSIZE];
};

void command_from_buffer(struct command*, const void*);
void command_to_buffer(struct command*, flatcc_builder_t*);
int overlaps(struct span*, struct span*);
int encloses(struct span*, struct span*);
int interferes(struct command*, struct command*);
