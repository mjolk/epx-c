/**
 * File   : instance.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : thu 13 jun 2019 18:50
 */

#include "../src/node.c"

int main(){
    char *nodes[3];
    nodes[0] = "127.0.0.1";
    nodes[1] = "127.0.0.1";
    nodes[2] = "127.0.0.1";
    struct node *n = new_node(1, nodes);
    
    return 0;
}