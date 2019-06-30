/**
 * File   : instance.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : thu 13 jun 2019 18:50
 */

#include "../src/node.c"
#include <stdio.h>

void test_configure(){
    char *cli[7];
    cli[0] = "programname";
    cli[1] = "-i";
    cli[2] = "1";
    cli[3] = "-n";
    cli[4] = "192.168.0.2,192.168.0.3,192.168.0.4";
    cli[5] = "-j2";
    cli[6] = "-j3";

    char node1[20];
    char node2[20];
    char node3[20];
    char *nodes[N];
    nodes[0] = node1;
    nodes[1] = node2;
    nodes[2] = node3;
    size_t join[N];
    ssize_t id = configure(7, cli, (char**)nodes, join);
    assert(id == 1);
    for(int i = 0;i < N;i++){
        printf("nodes%d: %s\n", i, nodes[i]);
    }
    assert(strcmp(nodes[0], "192.168.0.2") == 0);
    assert(strcmp(nodes[1], "192.168.0.3") == 0);
    assert(strcmp(nodes[2], "192.168.0.4") == 0);
    assert(join[0] == 2);
    assert(join[1] == 3);
}

void test_startnode(){
    char *nodes[3];
    nodes[0] = "127.0.0.1";
    nodes[1] = "127.0.0.1";
    nodes[2] = "127.0.0.1";
    size_t join[N] = {0};
    struct node *n = new_node(1, nodes, join);
}

int main(){
    test_startnode();
    test_configure();
    return 0;
}
