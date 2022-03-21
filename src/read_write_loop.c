#include <poll.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include "packet_implem.c"
#include "put_on_pkt.c"
#define MAX_BUFFER_SIZE 1024
void read_write_loop(int sfd){
    fprintf(stdout,"Start\n");
    int nfds = 2;
    int open = 2;
    int i = 0;
    int seqnum_receive = 0;
    struct pollfd *fds;
    fds = calloc(nfds,sizeof(struct pollfd));
    if(!fds){
        perror("Calloc failure");
        return;
    }
    pkt_t listpkt[5];
    printf("size: %ld\n",sizeof(listpkt));
    fds[0].fd = sfd;
    fds[0].events = POLLIN;
    fds[1].fd = sfd;
    fds[1].events = POLLOUT;
    while(open > 1){
        int ready;
        ready = poll(fds,nfds,-1);
        if(ready==-1){
            perror("Poll failure");
            return;
        }
        if(fds[1].revents & POLLOUT){
            //printf("pkt window: %d\n",pkt_get_window(&listpkt[i-1]));
            if(pkt_get_window(&listpkt[i-1])==0){
                printf("Inside\n");
                char buffer[1024];
                size_t size = 1024;
                pkt_set_window(&listpkt[i-1],3);
                pkt_encode(&listpkt[i-1],buffer,&size);
                write(sfd,buffer,sizeof(buffer));
                i--;
            }
            
        }
        if(fds[0].revents & POLLIN){
            char buf[MAX_BUFFER_SIZE];
            pkt_t *pkt = pkt_new();
            read(sfd,buf,sizeof(buf));
            int dec = pkt_decode(buf,sizeof(buf),pkt);
            printf("Payload val: %s\n",pkt_get_payload(pkt));
            pkt_t *resp = pkt_new();
            pkt_set_seqnum(resp,pkt_get_seqnum(pkt)+1);
            pkt_set_window(resp,0);
            if(dec==PKT_OK){
                pkt_set_type(resp,PTYPE_ACK);
            }
            listpkt[i++] = *resp;
        }

        
    }
    printf("End !");
}