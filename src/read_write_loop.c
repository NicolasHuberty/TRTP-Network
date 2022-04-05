#include <poll.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include "packet_implem.c"
#include "packet_interface.h"
#include "put_on_pkt.c"
#define MAX_BUFFER_SIZE 37

void read_write_loop(int sfd,FILE* sf){
    //Init all variables
    int nfds = 2;
    int end = 0;
    int open = 2;
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
    size_t size = MAX_BUFFER_SIZE;
    int nack = 256;
    uint32_t last_timestamp = 0;
    //FILE* wfd = fopen("output.txt","w");
    char buffer[MAX_BUFFER_SIZE];
    pkt_t **listPkt = malloc(window*sizeof(pkt_t*));
    struct pollfd *fds;
    fds = calloc(nfds,sizeof(struct pollfd));
    if(!fds){
        perror("Calloc failure");
        return;
    }
    fds[0].fd = sfd;
    fds[0].events = POLLIN | POLLHUP;
    fds[1].fd = sfd;
    fds[1].events = POLLOUT | POLLHUP;
    while(open > 1){
        int ready;
        ready = poll(fds,nfds,-1);
        if(ready==-1){
            perror("Poll failure");
            return;
        }if(ready > 0){
            if(recv(sfd,buffer,512,MSG_PEEK)==0){
                printf("DISCONNECTTTTT");
            }
        } 
        //Write on the buffer ACK/NACK
        if(fds[1].revents != 0){
            if(fds[1].revents & POLLOUT){
                if(nack != 256){
                    size = MAX_BUFFER_SIZE;
                    pkt_t *res = pkt_new();
                    pkt_set_window(res,window);
                    pkt_set_type(res,PTYPE_NACK);
                    pkt_set_tr(res,0);
                    pkt_set_seqnum(res,nack);
                    pkt_set_timestamp(res, clock());
                    pkt_set_length(res, 0);
                    pkt_set_payload(res, NULL, 0);
                    pkt_encode(res,buffer,&size);
                    int w = write(sfd,buffer,size);
                    if(w==-1){
                        perror("Write failure");
                        exit(EXIT_FAILURE);
                    }  
                    pkt_del(res);
                    res = NULL;
                    nack = 256;
                    data_sent++;
                    nack_sent++;

                }
                if(send_ack && nack == 256){   //Send ack
                    //printf("Try to send ack\n");               
                    size = MAX_BUFFER_SIZE;
                    pkt_t *res = pkt_new();
                    pkt_set_type(res,PTYPE_ACK);
                    pkt_set_tr(res,0);
                    pkt_set_window(res,3);
                    pkt_set_seqnum(res,0);
                    pkt_set_timestamp(res, clock());
                    pkt_set_length(res,0);
                    pkt_set_payload(res,NULL,0);
                    pkt_encode(res,buffer,&size);
                    if(!end){
                    int w = write(sfd,buffer,size);
                        if(w==-1){
                            perror("Write failure");
                            exit(EXIT_FAILURE);
                        }
                    }
                    pkt_del(res);
                    res = NULL;
                    data_sent++;
                    ack_sent++;
                    for (int i = 0; i < window_receive; i++){ // Clear the buffer
                        pkt_t *rem = listPkt[i];  
                        if(pkt_get_timestamp(rem)> last_timestamp){
                            last_timestamp = pkt_get_timestamp(rem); //Update last_time with the last one
                        }
                        if(pkt_get_length(rem)!=0){
                            fprintf(stdout,"%s",pkt_get_payload(rem));
                        }else{
                            pkt_t *last = pkt_new();
                            pkt_set_length(last, 0);
                            pkt_set_type(last, PTYPE_DATA);
                            char buffer[MAX_BUFFER_SIZE];
                            size = MAX_BUFFER_SIZE;
                            pkt_encode(last, buffer, &size);
                            for(int i =0;i<window;i++){
                                write(sfd,buffer,size);
                            }
                            pkt_del(last);
                            open = 0;
                            break;
                        }
                        //printf("Time:%d Size:%d Seqnum:%d  %s",pkt_get_timestamp(rem),pkt_get_length(rem),pkt_get_seqnum(rem),pkt_get_payload(rem));
                        pkt_del(rem);   
                        rem->payload = NULL;
                        rem = NULL;
                        listPkt[i] = NULL;
                    }
                    if(end){
                        printf("Break");
                        close(sfd);
                        free(fds);
                        break;
                        open = 0;
                    }
                    send_ack = window_receive = 0;
                    window = 3;   
                }
                
            }else{
                printf("Signal recek:klihive\n");
                open = 0;
            }
        }
        //Read the buffer
        if(fds[0].revents != 0){
            //printf("Receive data\n");
            if(fds[0].revents & POLLIN){
                data_received++;
                char buf[MAX_BUFFER_SIZE];
                pkt_t *pkt = pkt_new();
                int r = read(sfd,buf,MAX_BUFFER_SIZE);
                if(r==-1){
                    open = 0;
                    break;
                }
                int dec = pkt_decode(buf,MAX_BUFFER_SIZE,pkt);
                //printf("Receive: %s",pkt_get_payload(pkt));
                //printf("Payload nb:%d with seqnum: %d and timestamp: %d and type: %d\n",pkt_get_seqnum(pkt),pkt_get_seqnum(pkt),pkt_get_timestamp(pkt),pkt_get_type(pkt));
                //pkt_t *resp = pkt_new();
                DEBUG("Window val %d and window receive: %d\n",window,window_receive);
                if(dec==PKT_OK){
                    if(pkt_get_tr(pkt)==1){
                        data_truncated_receive++;
                    }
                    if(listPkt[pkt_get_seqnum(pkt)]==NULL && pkt_get_timestamp(pkt) > last_timestamp){
                        listPkt[pkt_get_seqnum(pkt)] = pkt;
                        if(window_receive++ +1 == window){
                            send_ack = 1;
                        }
                        //printf("Win val:%d\n",window_receive);
                    }else{
                        //printf("Already on buffer %s\n",pkt_get_payload(pkt));
                        pkt_del(pkt);
                        pkt = NULL;
                    }

                }else{
                    pkt_set_length(pkt, 0);
                    pkt_del(pkt);
                    
                   //printf("Invalid pkt rcv %d \n",pkt_get_seqnum(pkt));
                   nack = window_receive;
                    //printf("It(s a nack!!\n");
                    if(nack < 0 || nack > window){
                        nack = 0;
                    }
                    packet_ignored++;
                    
                }
                
                
            }
            else{
                printf("Break hrere\n"); //Functional
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
    //free(fds);
    free(listPkt);
    DEBUG("End !");
}