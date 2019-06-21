/**
 * File   : main.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 17 jan 2019 00:47
 */

#include "node.h"
#include <signal.h>

struct node* n;

void handler(int signo, siginfo_t *info, void *extra) {
    stop(n);
}

int main(int argc, char* argv[]){
    sigset_t mask;
	sigemptyset(&mask); 
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
    struct sigaction sact;
    sact.sa_flags = SA_SIGINFO; 
    sact.sa_sigaction = handler;
    //TODO configuration file/cli arguments
    char *nodes[N] = {0};
    n = new_node(1, nodes);
    if(start(n) != 0) return -1;
    sleep(1);
    pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
    return 0;
}
