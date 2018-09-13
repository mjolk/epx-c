#include <stdlib.h>
#include "llrb-interval/slist.h"
#include "llrb-interval/llrb-interval.h"
#include "llrb-interval/llrb.h"
#include <err.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <stdint.h>


SLL_HEAD(range_group, node) lhead;
struct range_group *merge_list;
struct nptr {
	struct node* sle_next;
};
struct node { 
	LLRB_ENTRY(node) entry; 
	int start;
	int end;
	int max;
	struct nptr next;

}; 

int intcmp(struct node *e1, struct node *e2) 
{ 
	return (e1->start < e2->start ? -1 : e1->start > e2->start); 
} 
LLRB_HEAD(range_tree, node);
struct range_tree head;
LLRB_PROTOTYPE(range_tree, node, entry, intcmp);
LLRB_GENERATE(range_tree, node, entry, intcmp)
LLRB_RANGE_GROUP_GEN(range_tree, node, entry, range_group, next)	

int testdata[5][2] = { 
	{20, 40}, {16, 18}, {5, 17}, {13, 99}, {3, 8} 
}; 

void print_tree(struct node *n) 
{ 
	struct node *left, *right; 

	if (n == NULL) { 
		printf("nil"); 
		return; 
	} 
	left = LLRB_LEFT(n, entry); 
	right = LLRB_RIGHT(n, entry); 
	if (left == NULL && right == NULL) 
		printf("[range %d, %d max: %d]", n->start, n->end, n->max); 
	else { 
		printf("[range %d, %d max: %d](", n->start, n->end, n->max); 
		print_tree(left); 
		printf(","); 
		print_tree(right); 
		printf(")"); 
	} 
}

void merge(struct node *to_merge, struct range_group *sll) {
	struct node *c, *prev, *next;
	next = SLL_FIRST(sll);
	SLL_INSERT_HEAD(sll, to_merge, next);
	if(!next) return;
	prev = SLL_FIRST(sll);
	while(next) {
		c = next;
		next = SLL_NEXT(next, next);
		if(to_merge->start < c->end && to_merge->end > c->start) { /**overlap**/
			if(to_merge->start > c->start){
				to_merge->start = c->start;
			}
			if(to_merge->end < c->end) {
				to_merge->end = c->end;
			}
			SLL_REMOVE_AFTER(prev, next);
			LLRB_DELETE(range_tree, &head, c);
		} else {
			prev = c;
		}
	}
}

struct node* new_range(int start, int end) {
	struct node *add_range;
	if ((add_range = malloc(sizeof(struct node))) == NULL) 
		err(1, NULL);
	add_range->start = start;
	add_range->end = end;
	add_range->max = 0;
	return add_range;
}

int add_to_range(struct node* nn) {
	int grown = LLRB_RANGE_GROUP_ADD(range_tree, &head, nn, merge_list, &merge);
	if(grown > 0){
		printf("range has grown\n");
	} else {
		printf("range already fully contained\n");
	}
	return grown;
}

int main(void) 
{
        merge_list = &lhead;	
	SLL_INIT(merge_list);
	size_t i; 
	struct node *n;
	LLRB_INIT(&head);	

	for (i = 0; i < sizeof(testdata) / sizeof(testdata[0]); i++) { 
		if ((n = malloc(sizeof(struct node))) == NULL) 
			err(1, NULL); 
		n->start = testdata[i][0];
		n->end = testdata[i][1];
		n->max = 0;	
		LLRB_INSERT(range_tree, &head, n); 
	}	

	LLRB_FOREACH(n, range_tree, &head) { 
		printf("%d\n", n->start); 
	}
	LLRB_FOREACH_REVERSE(n, range_tree, &head) {
		printf("%d\n", n->start);
	}
	print_tree(LLRB_ROOT(&head)); 
	printf("\n");
	struct node d = {.start = 13};
	struct node *deleted;
	deleted = LLRB_DELETE(range_tree, &head, &d);
	print_tree(LLRB_ROOT(&head));
	printf("\n");
	free(deleted);
	struct node *in;
	if ((in = malloc(sizeof(struct node))) == NULL) 
		err(1, NULL); 
	in->start = 34;
	in->end = 67;
	in->max = 0;
	LLRB_INSERT(range_tree, &head, in);
	print_tree(LLRB_ROOT(&head));



	add_to_range(new_range(12, 46));
	add_to_range(new_range(1, 2));
	add_to_range(new_range(6, 100));
	add_to_range(new_range(22, 78));


	print_tree(LLRB_ROOT(&head));
	return (0); 
}
