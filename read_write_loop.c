#include <poll.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include "packet_implem.c"
#include "put_on_pkt.c"
#include "fifo.c"
#define MAX_BUFFER_SIZE 1048

void read_write_loop(int sfd,FILE* sf){
    //fprintf(stdout,"Start\n");
    int nfds = 2;
    int open = 2;
    int i = 0;
    int send_ack = 0;
    int window = 1;
    int data_sent = 0;
    int data_received = 0;
    int data_truncated_receive = 0;
    int fec_sent = 0;
    int fec_received = 0;
    int ack_sent = 0;
    int ack_received = 0;
    int nack_sent = 0;
    int nack_received = 0;
    int packet_ignored = 0;
    int window_receive = 0;
    int nack = 0;
    struct pollfd *fds;
    fds = calloc(nfds,sizeof(struct pollfd));
    if(!fds){
        perror("Calloc failure");
        return;
    }
    pkt_t listpkt[5];
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
        //Write on the buffer ACK/NACK
        if(fds[1].revents != 0){
            if(fds[1].revents & POLLOUT){
                if(nack != 0){
                    char buffer[1024];
                    size_t size = 1024;
                    pkt_t *pkt = pkt_new();
                    pkt_set_type(pkt,PTYPE_NACK);
                    pkt_encode(pkt,buffer,&size);
                    int w = write(sfd,buffer,sizeof(buffer));
                    data_sent++;
                    nack_sent++;
                    if(w==-1){
                        perror("Write failure");
                        exit(EXIT_FAILURE);
                    }    
                }            
                if(send_ack){
                    char buffer[1024];
                    size_t size = 1024;
                    pkt_set_window(&listpkt[i-1],3);
                    window = 3;
                    pkt_set_seqnum(&listpkt[i-1],0);
                    pkt_encode(&listpkt[i-1],buffer,&size);
                    int w = write(sfd,buffer,sizeof(buffer));
                    data_sent++;
                    ack_sent++;
                    if(w==-1){
                        perror("Write failure");
                        exit(EXIT_FAILURE);
                    }
                    send_ack = 0;
                    i = 0;
                    window_receive = 0;
                }
                
            }else{
                DEBUG("Signal receive\n");
                open = 0;
            }
        }
        //Read the buffer
        if(fds[0].revents != 0){
            if(fds[0].revents & POLLIN){
                data_received++;
                char buf[MAX_BUFFER_SIZE];
                pkt_t *pkt = pkt_new();
                int r = read(sfd,buf,sizeof(buf));
                if(r==-1){
                    open = 0;
                    break;
                }
                int dec = pkt_decode(buf,sizeof(buf),pkt);
                printf("Payload nb:%d with seqnum: %d and timestamp: %d\n",pkt_get_seqnum(pkt),pkt_get_seqnum(pkt),pkt_get_timestamp(pkt));
                if(pkt_get_type(pkt)== PTYPE_DATA && pkt_get_length(pkt)==0){
                    open = 0;
                    close(sfd);
                    break;
                }
                if(pkt_get_tr(pkt)==1){
                    data_truncated_receive++;
                }
                pkt_t *resp = pkt_new();
                DEBUG("Window val %d and window receive: %d\n",window,window_receive);
                if(dec==PKT_OK){
                    printf("Packet nb %d valid\n",pkt_get_seqnum(pkt));
                    if(window_receive++ +1 == window){
                        DEBUG("Send ack val: 1\n");
                        send_ack = 1;
                        pkt_set_seqnum(resp,(pkt_get_seqnum(pkt)+1)%255);
                        pkt_set_type(resp,PTYPE_ACK);
                    }
                }else{
                    pkt_set_type(resp,PTYPE_NACK); 
                    nack = pkt_get_seqnum(pkt);
                    packet_ignored++;
                }
                listpkt[i++] = *resp;
                pkt_del(pkt);
                
            }
            else{
                open = 0;
            }
        } 
        
    }
    fprintf(sf,"data_sent: %d\n",data_sent);
    fprintf(sf,"data_received: %d\n",data_received);
    fprintf(sf,"data_truncated_receive: %d\n",data_truncated_receive);
    fprintf(sf,"fec_sent: %d\n",fec_sent);
    fprintf(sf,"fec_received: %d\n",fec_received);
    fprintf(sf,"ack_sent: %d\n",ack_sent);
    fprintf(sf,"ack_received: %d\n",ack_received);
    fprintf(sf,"nack_sent: %d\n",nack_sent);
    fprintf(sf,"nack_received: %d\n",nack_received);
    fprintf(sf,"packet_ignored: %d\n",packet_ignored);
    DEBUG("End !");
    free(myFifo);
}