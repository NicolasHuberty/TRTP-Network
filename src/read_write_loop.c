#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include "log.h"
#include "packet_implem.c"
#include "packet_interface.h"
#include "put_on_pkt.c"
#define MAX_BUFFER_SIZE 512
#define realWindow 4
int lastFullAck = 0;
int free_place = realWindow;

int full_ack = -1;
int partial_ack = -1;

int base_r = 0;
int nack = -1;
int fill = 0;
uint32_t last_timestamp = 0;
uint32_t last_time_pkt = 0;

//STATS
int data_sent_receiver = 0;
int data_received_receiver = 0;
int data_truncated_received_receiver = 0;
int fec_sent_receiver = 0;
int fec_received_receiver = 0;
int ack_sent_receiver = 0;
int ack_received_receiver = 0;
int nack_sent_receiver = 0; 
int nack_received_receiver = 0;
int packet_ignored_receiver = 0;
int packet_duplicated_receiver = 0;
int packet_recovered_receiver = 0;
int first_empty_place(pkt_t ** listPkt){
    int  i = 0;
    while(listPkt[i]){
        i++;
    }
    return i;
}
int find_buffer_r(pkt_t **listPkt,int seq,int win){
    for(int i = 0; i < win ; i++){
        if(listPkt[i] && pkt_get_seqnum(listPkt[i])==seq){
            return i;
        }
    
    }
    return -1;
}
int clear_and_fill_buffer_r(pkt_t **listPkt){
    //int place_of_ack = find_buffer_r(listPkt, ack, realWindow);
    int s = 0;
    while(listPkt[s]!=NULL){
        s++;
    }
    base_r = (base_r+s)%(realWindow);
    int first = first_empty_place(listPkt);
    //DEBUG("FIRST EMPTY:%d",first);
    //DEBUG("PLACE OF PKT TO DEL:%d and ACK%d FIRST EMPTY PLACE:%d",s,ack,first);
    for(int i = 0; i < first ; i++){
        //DEBUG("PRINT AND REMOVE%d",i);
        ERROR("TRY TO REMOVE PKT%dAT PLACE:%d",pkt_get_seqnum(listPkt[i]),i);
        fwrite(pkt_get_payload(listPkt[i]),sizeof(char),pkt_get_length(listPkt[i]),stdout);
        free_place++;
        pkt_del(listPkt[i]);
        listPkt[i] = NULL;
    }
    //ERROR("SHIFT ALL BUFFER OF %d PLACES",first);
    for(int i = first; i < realWindow; i++){
        listPkt[i-first] = listPkt[i];
        //DEBUG("SHIFT %d IN %d",i,i-first);
    }
    //DEBUG("BUFFER VAL:");
    for(int i = 0;i<realWindow;i++){
        if(listPkt[i]==NULL)fprintf(stderr, "NULL ");
        else fprintf(stderr,"%d ",pkt_get_seqnum(listPkt[i]));
    }
    return 0;
}
void read_write_loop(int sfd,FILE* sf){
    int err;
    pkt_t **listPkt = malloc(realWindow * sizeof(pkt_t));
    char buffer[MAX_BUFFER_SIZE];
    //Init all variables
    int open = 2,nfds = 2;
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
        }
        //Read the buffer
        if(fds[0].revents != 0){
            if(fds[0].revents & POLLIN){
                err = read(sfd,buffer,MAX_BUFFER_SIZE);
                if(err < 0){ERROR("CAN'T READ BUFFER");}
                pkt_t *pktRcv = pkt_new();
                err = pkt_decode(buffer,MAX_BUFFER_SIZE,pktRcv);
                data_received_receiver++;
                if(err != PKT_OK || pkt_get_type(pktRcv)==PTYPE_NACK){ //Bad packet
                    DEBUG("DROP PKT%d",pkt_get_seqnum(pktRcv));
                    if(pkt_get_tr(pktRcv)==1)data_truncated_received_receiver++;
                    if(pkt_get_seqnum(pktRcv) < lastFullAck + realWindow && pkt_get_seqnum(pktRcv) > lastFullAck && listPkt[pkt_get_seqnum(pktRcv)]==NULL){
                        nack = pkt_get_seqnum(pktRcv);
                    }
                    pkt_set_length(pktRcv,0);
                    pkt_del(pktRcv);
                    packet_ignored_receiver++;
                }else{ //Good packet
                    //Update last TIMESTAMP
                    
                    if(pkt_get_timestamp(pktRcv) > last_time_pkt)last_time_pkt = pkt_get_timestamp(pktRcv);
                    if(pkt_get_length(pktRcv)==0 && pkt_get_type(pktRcv) == PTYPE_DATA){
                        DEBUG("SHOULD CLOSE THE ZLGO");
                        int i = 0;
                        while(listPkt[i] && pkt_get_length(listPkt[i])>0){
                            fwrite(pkt_get_payload(listPkt[i]),pkt_get_length(listPkt[i]),sizeof(char),stdout);
                            pkt_del(listPkt[i++]);
                        }
                        ERROR("CLOSE THE SOCKET");
                        pkt_del(pktRcv);
                        close(sfd);
                        break;
                        open = 0;
                    }
                    if(pkt_get_seqnum(pktRcv)==0)base_r=0;
                    //DEBUG("SHOULD PLACEC AT%d SEQ:%d AND BASE:%d",pkt_get_seqnum(pktRcv)-base_r,pkt_get_seqnum(pktRcv),base_r);
                    if(listPkt[(pkt_get_seqnum(pktRcv)-base_r)%realWindow] == NULL){
                        listPkt[(pkt_get_seqnum(pktRcv)-base_r)%realWindow] = pktRcv;
                        DEBUG("PLACE PKT%d AT: %d",pkt_get_seqnum(pktRcv),(pkt_get_seqnum(pktRcv)-base_r)%realWindow);
                        free_place--;
                        partial_ack = (pkt_get_seqnum(pktRcv)+1)%256;
                        
                        //DEBUG("PARTIAL ACK = 1");
                    }else{
                        DEBUG("DROP PKT%d",pkt_get_seqnum(pktRcv));
                        packet_ignored_receiver++;
                        packet_duplicated_receiver++;
                        if(pkt_get_length(pktRcv)==0)pkt_del(pktRcv);
                    }
                }
                //ERROR("RECEIVE PKT%d",pkt_get_seqnum(pktRcv));
            }else{
                DEBUG("GOOD NEW");
                open = 0;
            }
        }
         
        //Write on  the buffer ACK/NACK
        if(fds[1].revents != 0){
            if(fds[1].revents & POLLOUT){
                if(nack != -1){
                    ERROR("NACK SEND %d",nack);
                    size_t size = MAX_BUFFER_SIZE;
                    pkt_t *nackPkt = put_on_pkt(buffer,NULL,nack,free_place,0,2,&size,last_time_pkt);
                    write(sfd,buffer,size);
                    nack_sent_receiver++;
                    data_sent_receiver++;
                    pkt_del(nackPkt);
                    nack = -1;
                }
                if(partial_ack != -1 && nack == -1 && full_ack == -1){
                    clear_and_fill_buffer_r(listPkt);
                    size_t size = MAX_BUFFER_SIZE;
                    pkt_t *partialAck = put_on_pkt(buffer,NULL,partial_ack,free_place,0,1,&size,last_time_pkt);
                    err = write(sfd,buffer,size);
                    ack_sent_receiver++;
                    if(err < 0){ERROR("CAN'T WRITE ON THE SOCKET");}
                    pkt_del(partialAck);
                    data_sent_receiver++;
                    if(partial_ack==0)base_r=0;
                    //ERROR("SEND PARTIAL ACK%d WIN: %d BASE_R:%d",partial_ack,free_place,base_r);
                    partial_ack = -1;
                    last_timestamp = clock();
               }
            }else{
                DEBUG("GOOD SIGNAL END2!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
                open = 0;
            }
        }
    }
    free(fds);
    free(listPkt);
    DEBUG("End !");
    fprintf(sf,"data_sent:%d\n",data_sent_receiver);
    fprintf(sf,"data_received:%d\n",data_received_receiver);
    fprintf(sf,"data_truncated_received:%d\n",data_truncated_received_receiver);
    fprintf(sf,"fec_sent:%d\n",fec_sent_receiver);
    fprintf(sf,"fec_received_receiver:%d\n",fec_received_receiver);
    fprintf(sf,"ack_sent_receiver:%d\n",ack_sent_receiver);
    fprintf(sf,"ack_received_receiver:%d\n",ack_received_receiver);
    fprintf(sf,"nack_sent:%d\n",nack_sent_receiver);
    fprintf(sf,"nack_received:%d\n",nack_received_receiver);
    fprintf(sf,"packet_ignored_receiver:%d\n",packet_ignored_receiver);
    fprintf(sf,"packet_duplicated_receiver:%d\n",packet_duplicated_receiver);
    fprintf(sf,"packet_recovered_receiver:%d\n",packet_recovered_receiver);
    DEBUG("TOTAL TIME:%f",(float)clock()/CLOCKS_PER_SEC);
    fclose(sf);
}