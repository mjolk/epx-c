/**
 * File   : channel.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 24 jan 2019 22:00
 */

#include "../src/channel.h"
#include <stdio.h>

void *sendmsg(void*);
void *receivemsg(void*);

int main(){

    chan *channel = malloc(sizeof(chan));
    if(chan_init(channel) < 0) return 1;
    pthread_t sending[3];
    pthread_t receiving[1];

    for(int j = 0; j < 1;j++){
        if(pthread_create(&receiving[j], NULL, receivemsg, channel) != 0)
            return 1;
    }

    for(int i = 0; i < 3;i++){
        if(pthread_create(&sending[i], NULL, sendmsg, channel) != 0){
            return 1;
        }
        pthread_detach(sending[i]);
    }

    pthread_join(receiving[0], NULL);
    free(channel);
}

void *sendmsg(void *ch){
    for(int i = 0;i < 1000;i++){
        char *msg = malloc(sizeof(char)*5);
        if(sprintf(msg, "%d", i) < 0) return 0;
        chan_send(ch, (void*)msg);
    }
    return 0;
}

void *receivemsg(void *ch){
    int running = 1;
    while(running){
        char *msg = chan_recv(ch);
        if(chan_size(ch) == 0) running = 0;
        free(msg);
    }
    return 0;
}
