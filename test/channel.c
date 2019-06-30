/**
 * File   : channel.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 24 jan 2019 22:00
 */

#include "../src/channel.h"
#include "../include/epx.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

void *sendmsg(void*);
void *receivemsg(void*);

int main(){

    chan *channel = (struct chan*)malloc(sizeof(struct chan));
    chan_init_sem(channel);
    pthread_t sending[3];
    pthread_t receiving[1];

    for(int i = 0; i < 3;i++){
        if(pthread_create(&sending[i], NULL, sendmsg, channel) != 0){
            return 1;
        }
        pthread_detach(sending[i]);
    }

    for(int j = 0; j < 1;j++){
        if(pthread_create(&receiving[j], NULL, receivemsg, channel) != 0)
            return 1;
    }

    pthread_join(receiving[0], NULL);
    free(channel);
}

struct entry {
    int id;
    char msg[5];
};

void *sendmsg(void *ch){
    for(int i = 0;i < 1000;i++){
        //char *msg = malloc(sizeof(char)*5);
        struct entry *msg = (struct entry*)malloc(sizeof(struct entry));
        sprintf(msg->msg, "%d", i);
        msg->id = i;
        //if(sprintf(msg, "%d", i) < 0) return 0;
        //printf("sending msg: %d  %s @: %p\n", msg->id, msg->msg, msg);
        if(!sem_chan_send_mpsc((struct chan*)ch, msg)){
            printf("channel full\n");
        }
    }
    return 0;
}

void *receivemsg(void *ch){
    int running = 1;
    int cnt = 0;
    while(running){
        struct entry *msg = 0;
        if(!sem_chan_recv_mpsc((struct chan*)ch, &msg)){
            printf("channel empty\n");
            cnt++;
            if(cnt == 3000) running = 0;
            //msg = 0;
        }
        if(msg){
            //printf("count %d \n", cnt);
            //printf("received msg:  %d %s @: %p\n",msg->id, msg->msg, msg);
            cnt++;
            free(msg);
            if(cnt == 3000) running = 0;

        }
    }
    return 0;
}
