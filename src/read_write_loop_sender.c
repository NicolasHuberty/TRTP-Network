#include "log.h"
#include "packet_interface.h"
#include <poll.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <zlib.h>
#include <stdbool.h>
#define MAX_BUFFER_SIZE 512
int seqnum = 0; //Contain the seqnum of the insert pkt
int last_valid = 0; //Contain the seqnum of the last ack receive
int pkt_sent = 0;   //Contain the number of valid pkt recv on a buffer
int window = 1; //Contain the window of the buffer => WILL became the size of the receiver window
int free_place_on_buffer = 1; // Init free_place with the size of the window => The size of the buffer
int nfds=2,num_open_fds = 2;
uint32_t last_lastPkt = 0;
uint32_t last_pkt_sent = 0;
uint32_t rtt_delay;
pkt_t *lastAck;
int base = 0;
int nextseqnum = 0;
//STATS
int data_sent = 0;
int data_received = 0;
int data_truncated_received = 0;
int fec_sent = 0;
//int fec_received = 0; SHOULD NEVER RECEIVE FEC
//int ack_sent = 0; SHOULD NEVER SEND ACK
int ack_received = 0;
int nack_sent = 0; 
int nack_received = 0;
int packet_ignored = 0;
int min_rtt = 9999999;
int max_rtt = 0;
int packet_retransmitted = 0;

bool buffer_update = false;
int find_buffer(pkt_t **listPkt,int seq,int win){
    for(int i = 0; i < win ; i++){
        if(listPkt[i] && pkt_get_seqnum(listPkt[i])==seq){
            return i;
        }
    
    }
    return -1;
}
int clear_and_fill_buffer(pkt_t **listPkt,int ack,int win,FILE *fd){
    if(ack != -10){
        //int place_of_ack = find_buffer(listPkt, ack, 50);
        int s = 0;
        if(listPkt[0]==NULL){
            buffer_update = true;
        }
        while(listPkt[s]== NULL){
            s++;
        }//DEBUG("SHIFT THE BUFFER OF %d PLACE START:%d STOP:%d",s,s,50-s);
        for(int i =s; i <= 50-s;i++){
            listPkt[i-s] = listPkt[i];
            //DEBUG("UPDATE PLACE%d WITH PLACE%d",i,i-s);
        }

        for(int i = 50-s ; i < 50;i++){
            size_t len = MAX_BUFFER_SIZE;
            char content[MAX_BUFFER_SIZE - sizeof(pkt_t)];
            char buffer[MAX_BUFFER_SIZE];
            int fr = fread(content,sizeof(char),MAX_BUFFER_SIZE - sizeof(pkt_t),fd);
            pkt_t *toInsert = put_on_pkt(buffer,content,seqnum++,win,fr,0,&len,clock());
            if(fr==0 && last_lastPkt == 0){
                last_lastPkt = clock();
            }
            //DEBUG("INSERT PKT%d AT PLACE%d",pkt_get_seqnum(toInsert),i);
            listPkt[i] = toInsert;
        }//DEBUG("STOP TO INSERT");
    }else{
        for(int i = 0; i < 50;i++){
            size_t len = MAX_BUFFER_SIZE;
            char content[MAX_BUFFER_SIZE - sizeof(pkt_t)];
            char buffer[MAX_BUFFER_SIZE];
            int fr = fread(content,sizeof(char),MAX_BUFFER_SIZE - sizeof(pkt_t),fd);
            pkt_t *toInsert = put_on_pkt(buffer,content,seqnum++,win,fr,0,&len,clock());
            //DEBUG("INSERT PKT%d AT PLACE%d",pkt_get_seqnum(toInsert),i);
            listPkt[i] = toInsert; 
        }
    }
    for(int i = 0;i<window;i++){
        if(listPkt[i]==NULL)fprintf(stderr, "NULL ");
        else fprintf(stderr,"%d ",pkt_get_seqnum(listPkt[i]));
    }
    
    return 0;
}
int read_write_loop_sender(int sfd,FILE *fd,FILE *stat){
    //Initiation of all variables
    size_t len = MAX_BUFFER_SIZE;
    char buffer[MAX_BUFFER_SIZE];
    int nack = -1;  
    lastAck = put_on_pkt(buffer, NULL, 0, 1, 0, 1, &len,clock());
    pkt_t **listPkt = malloc(50*sizeof(pkt_t)); //Create a buffer of size 1
    //uint32_t last_timestamp = 0;
    rtt_delay = clock();
    struct pollfd *pfds;
    pfds = calloc(2,sizeof(struct pollfd));
    if(!pfds){
        perror("Calloc failure");
        exit(EXIT_FAILURE);
    }
    pfds[0].fd = sfd;
    pfds[0].events = POLLOUT; //Write on the socket
    pfds[1].fd = sfd;
    pfds[1].events = POLLIN; //Read the socket

    clear_and_fill_buffer(listPkt, -10, window, fd);
    buffer_update = true;

    while(num_open_fds > 0){ //While sender and receiver don't close the socket
        int ready = poll(pfds,nfds,-1); // Change -1 for timeout
        if(ready == -1){
            perror("Poll failure");
            exit(EXIT_FAILURE);
        }
        //Receive data from the socket
        if(pfds[1].revents != 0){
            if(pfds[1].revents & POLLIN){
                int r = read(sfd,buffer,MAX_BUFFER_SIZE);
                if(r==-1){
                    num_open_fds = 0;
                    break;
                    ERROR("Read socket failure");
                }
                pkt_t *decode = pkt_new();
                int dec = pkt_decode(buffer, MAX_BUFFER_SIZE, decode);
                if(dec != PKT_OK){ //IMPOSSIBLE TO DECODE => DROP IT
                    data_truncated_received++;
                    packet_ignored++;
                    ERROR("CORRUPTED PKT DROP IT");
                    pkt_del(decode);
                
                }else if(pkt_get_type(decode)==PTYPE_NACK){ //nack
                    //last_timestamp = pkt_get_timestamp(decode);
                    //If the packet is a NACK
                    //DEBUG("RECEIVE NACK%d LAST ACK VAL%d WiN VAL:%d LAST VALID:%d",pkt_get_seqnum(decode),pkt_get_seqnum(lastAck),window,last_valid);
                    if(pkt_get_seqnum(decode) < last_valid || pkt_get_seqnum(decode) > last_valid+window){
                        nack = 256;
                    }else{
                        nack_received++;
                        nack = pkt_get_seqnum(decode);
                    }
                    pkt_del(decode);
                    
                }else if(pkt_get_type(decode)==PTYPE_ACK){ //Case of ACK
                    //last_timestamp = pkt_get_timestamp(decode);
                    //DEBUG("RECEIVE AN ACK%d last ack:%d!!!",pkt_get_seqnum(decode),pkt_get_seqnum(lastAck));
                    if(pkt_get_seqnum(decode)>pkt_get_seqnum(lastAck) || pkt_get_seqnum(decode)==0){
                        base = pkt_get_seqnum(decode);
                        //ERROR("BASE UPDATE TO %d",base);
                    }
                    rtt_delay = clock();
                    ack_received++;
                    ERROR("RECEIVE AN ACK%d WITH WINDOW:%d BASE VALUE:%d",pkt_get_seqnum(decode),pkt_get_window(decode),base);
                    int place;
                    if(pkt_get_seqnum(decode)==0)place =255;
                    else place = pkt_get_seqnum(decode)-1;
                    //DEBUG("WANT TO DEL PKT%d",place);//BUG IF IT'S 0
                    if(find_buffer(listPkt, place, window)!= -1 && listPkt[find_buffer(listPkt, place, window)]!=NULL){
                        //DEBUG("DELETE PKT%d AT PLACE: %d",pkt_get_seqnum(listPkt[find_buffer(listPkt, place,50)]),find_buffer(listPkt, place,50));
                        int place_to_del = find_buffer(listPkt,place,50);
                        pkt_del(listPkt[place_to_del]);
                        //DEBUG("PUT %d AT NULL",place_to_del);
                        listPkt[place_to_del] = NULL;
                    }
                    clear_and_fill_buffer(listPkt, pkt_get_seqnum(decode), pkt_get_window(decode), fd);
                    if(pkt_get_window(decode)>window)window = pkt_get_window(decode);
                    last_valid = pkt_get_seqnum(decode);
                    pkt_sent = 0;
                    free_place_on_buffer = window;
                    pkt_del(lastAck);
                    lastAck = decode; 
                    //DEBUG("UPDATE LAST ACK TO%d",pkt_get_seqnum(lastAck)); 

                }else{
                    pkt_del(decode);
                    data_received++;
                    packet_ignored++;
                }
              
            }else{
                DEBUG("DROP THE PACKET");
                num_open_fds = 0;
            }
        }


        if(pfds[0].revents != 0){
            //Try to send data
            if(pfds[0].revents & POLLOUT){  
                
                //if(clock() > last_lastPkt + (max_rtt*5000)+10000 && last_lastPkt != 0){
                //    int i = 0;
                //    while(listPkt[i]){
                //        pkt_del(listPkt[i]);
                //        listPkt[i++] = NULL;
                //    }
                //    close(sfd);
                //    num_open_fds =  0;
                //}
                
                if(nack != -1 && nack != 256){
                    ERROR("send nack nb%d AT PLACE:%d",nack,find_buffer(listPkt,nack,50));
                    len = MAX_BUFFER_SIZE;
                    pkt_encode(listPkt[find_buffer(listPkt, nack, 50)],buffer,&len);
                    write(sfd,buffer,len);
                    nack_sent++;
                    data_sent++;
                    nack = -1;
                }
                //DEBUG("PKT SEQ: %d AND WIN:%d FREE PLACE:%d",pkt_get_seqnum(lastAck),pkt_get_window(lastAck),free_place_on_buffer);
                if (buffer_update){
                    //DEBUG("SEND PKT%d TO%d",pkt_get_seqnum(listPkt[0]),pkt_get_seqnum(listPkt[0])+pkt_get_window(lastAck));
                    //DEBUG("while nextseqnum%d < base+window:%d && next!=0 or seqnum%d==0",nextseqnum,((base))+window,pkt_get_seqnum(lastAck));
                    while((nextseqnum < ((base)+window) && (nextseqnum != 0 || pkt_get_seqnum(lastAck) == 0))){
                        //DEBUG("PKT%d HAS A PLACE OF %d",nextseqnum,find_buffer(listPkt,(nextseqnum)%256,50));
                        len = MAX_BUFFER_SIZE;
                        pkt_encode(listPkt[find_buffer(listPkt,(nextseqnum)%256,50)],buffer,&len);
                        write(sfd,buffer,len);
                        data_sent++;
                        pkt_set_timestamp(listPkt[find_buffer(listPkt,(nextseqnum)%256,50)],clock());
                        DEBUG("SEND PKT%d",pkt_get_seqnum(listPkt[find_buffer(listPkt,(nextseqnum)%256,50)]));
                        
                        last_pkt_sent = clock();
                        buffer_update = false;
                        nextseqnum++;
                    }
                    nextseqnum= (nextseqnum)%256;
                }
                if(listPkt[0] != NULL && pkt_get_timestamp(listPkt[0]) + 500000 < clock()){
                    DEBUG("RESEND PKT%d TIMEOUT%d AND CLOCK:%d ",pkt_get_seqnum(listPkt[0]),pkt_get_timestamp(listPkt[0]),(int)clock());
                    pkt_set_timestamp(listPkt[0], clock());
                    len = MAX_BUFFER_SIZE;
                    pkt_encode(listPkt[0], buffer, &len);
                    write(sfd,buffer,len);
                }                    

                
            }
        }
    }
    DEBUG("END FILE");
    free(listPkt);
    DEBUG("FREE LIST");
    free(pfds);
    DEBUG("FREE PFDS");
    fprintf(stat,"data_sent:%d\n",data_sent);
    fprintf(stat,"data_received:%d\n",data_received);
    fprintf(stat,"data_truncated_receive:%d\n",data_truncated_received);
    fprintf(stat,"fec_sent:%d\n",fec_sent);
    fprintf(stat,"fec_received:%d\n",0);
    fprintf(stat,"ack_sent:%d\n",0);
    fprintf(stat,"ack_received:%d\n",ack_received);
    fprintf(stat,"nack_sent:%d\n",nack_sent);
    fprintf(stat,"nack_received:%d\n",nack_received);
    fprintf(stat,"packet_ignored:%d\n",packet_ignored);
    fprintf(stat,"min_rtt:%d\n",min_rtt);
    fprintf(stat,"max_rttt:%d\n",max_rtt);
    fprintf(stat,"packet_retransmitted:%d\n",packet_retransmitted);
    fclose(stat);
    return 0;
}