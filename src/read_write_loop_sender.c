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
int read_write_loop_sender(int sfd,FILE *fd,FILE *stat){
    //Initiation of all variables
    int wait_an_ack = 0;
    int window = 1;
    int pkt_send = 0;
    int nack = 256;
    int pkt_receive = 0;
    int ack_received = 0;
    int nack_received = 0;
    int seqnum_send = 0;
    //int wait_last = 0;
    int nfds=2,num_open_fds = 2;
    size_t len = MAX_BUFFER_SIZE;
    uint32_t last_timestamp = clock();
    struct pollfd *pfds;
    pkt_t **listPkt = malloc(window*sizeof(pkt_t*)); //Bufffer of size window and type pkt
    pfds = calloc(2,sizeof(struct pollfd));
    if(!pfds){
        perror("Calloc failure");
        exit(EXIT_FAILURE);
    }
    pfds[0].fd = sfd;
    pfds[0].events = POLLOUT; //Write on the socket
    pfds[1].fd = sfd;
    pfds[1].events = POLLIN; //Read the socket
    while(num_open_fds > 0){ //While sender and receiver don't close the socket
        int ready = poll(pfds,nfds,-1); // Change -1 for timeout
        if(ready == -1){
            perror("Poll failure");
            exit(EXIT_FAILURE);
        }
        //Receive data from the socket
        if(pfds[1].revents != 0){
            if(pfds[1].revents & POLLIN){
                char buf[MAX_BUFFER_SIZE];
                ssize_t s = read(pfds[1].fd,buf,MAX_BUFFER_SIZE);
                if(s == -1){
                    perror("Read failure");
                    exit(EXIT_FAILURE);
                }
                pkt_receive++;
                pkt_t *pkt = pkt_new();
                len = MAX_BUFFER_SIZE;
                pkt_decode(buf,len,pkt);
                if(pkt_get_type(pkt)==PTYPE_DATA && pkt_get_length(pkt)== 0){
                    num_open_fds = 0;
                    break;
                }
                if(pkt_get_type(pkt)==PTYPE_ACK){ //Case of ack
                    last_timestamp = pkt_get_timestamp(pkt);
                    ack_received++;
                    seqnum_send = wait_an_ack = 0;
                    for(int i =0;i<window;i++){ //Clear the buffer
                        pkt_t*rem = listPkt[i];
                        pkt_del(rem);
                        listPkt[i] = NULL;
                    }
                    if(window != pkt_get_window(pkt)){ //Reset the buffer with the new buffer
                        free(listPkt);
                        listPkt = malloc(window*sizeof(pkt_t*));
                    }
                    window = pkt_get_window(pkt);                   
                } 
                else{ //Case of nack
                    //Avoid to run a nack with a seqnum if seqnum ∉ window
                    nack_received++;
                    if(pkt_get_seqnum(pkt)>window){
                        nack = -1;
                    }
                    if(nack != -1){
                        nack = pkt_get_seqnum(pkt);
                    }
                    
                }
                //Free the packet receive
                pkt_set_payload(pkt, NULL, 0);
                pkt_del(pkt); 
            }
        }

        if(pfds[0].revents != 0){
            //Try to send data
            if(pfds[0].revents & POLLOUT){
                if(nack != 256 && nack != -1){//Seqnum of nack known
                    pkt_t *resend = listPkt[nack];
                    char buffer[MAX_BUFFER_SIZE];
                    len = MAX_BUFFER_SIZE;
                    pkt_encode(resend, buffer, &len);
                     pkt_t *test = pkt_new();
                    pkt_decode(buffer, MAX_BUFFER_SIZE, test);
                    //printf("Normally receive: %s",pkt_get_payload(test));
                    pkt_del(test);
                    int s = write(sfd,buffer,len);
                    if(s==-1){ 
                        perror("Known nack sent failure");
                    }
                    nack = 256; //No more nack to check
                }
                if(nack == -1 ||last_timestamp +2000 < clock()){ //Case of timestamp or nack unknown
                    for(int i = 0;i < seqnum_send; i++){
                        pkt_t *resend = listPkt[i];
                        char buffer[MAX_BUFFER_SIZE];
                        len = MAX_BUFFER_SIZE;
                        pkt_encode(resend, buffer, &len);
                        int s = write(sfd, buffer,len);
                        //printf("Nack resent: %s",pkt_get_payload(resend));
                        if(s==-1){
                            perror("Write timeout failure");
                        }
                        last_timestamp = clock();
                        nack = 256;
                    }
                }
                if(!wait_an_ack){ // Is able to send data (a window)
                    while(num_open_fds > 0 && seqnum_send < window){
                        size_t size_content = MAX_BUFFER_SIZE - sizeof(pkt_t);
                        char buffer[MAX_BUFFER_SIZE];
                        char content[size_content];
                        //Start read file
                        int r = fread((void *)content,sizeof(char),size_content,fd);
                        if(r==-1){
                            perror("Read failure");
                            exit(EXIT_FAILURE);
                        }
                        pkt_t *toSend = pkt_new();
                        pkt_set_type(toSend, PTYPE_DATA);
                        pkt_set_tr(toSend,0);
                        pkt_set_window(toSend, window);
                        pkt_set_length(toSend, size_content);
                        pkt_set_seqnum(toSend, seqnum_send++);
                        pkt_set_timestamp(toSend, clock());
                        pkt_set_payload(toSend, content, size_content);
                        len = MAX_BUFFER_SIZE;
                        //printf("Pkt%d time:%d sent payload: %s\n",pkt_get_seqnum(toSend),pkt_get_timestamp(toSend),pkt_get_payload(toSend));
                        pkt_encode(toSend, buffer, &len);
                        int w = write(sfd,buffer,len);
                        if(w==-1){
                            perror("Read failure");
                            exit(EXIT_FAILURE);
                        }
                        listPkt[seqnum_send -1] = toSend;
                        pkt_send++;
                        if(feof(fd)){//End to read the file
                            for(int i =seqnum_send;i < window;i++){
                                printf("Push one end pkt%d\n",i);
                                char buffer[MAX_BUFFER_SIZE];
                                pkt_t *toSend = pkt_new();
                                pkt_set_type(toSend, PTYPE_DATA);
                                pkt_set_tr(toSend,0);
                                pkt_set_window(toSend, window);
                                pkt_set_length(toSend, 0);
                                pkt_set_seqnum(toSend, seqnum_send++);
                                pkt_set_timestamp(toSend, clock());
                                pkt_set_payload(toSend, NULL,0);
                                int wr = write(sfd,buffer,MAX_BUFFER_SIZE);
                                listPkt[i] = toSend;
                                //wait_last = 1;
                                if(wr == -1){
                                    DEBUG("Signal end not write\n");
                                }
                                
                                seqnum_send = window;
                            }
                            printf("Buffer full with end signal\n");
                            //free(pkt);
                            //fclose(fd);
                            //close(pfds[0].fd);
                            //close(pfds[1].fd);
                            //num_open_fds=0;
                        }
                        DEBUG("Packet number %d send\n",seqnum_send -1);

                    }
                    wait_an_ack = 1;
                }
                    
            }else{
                printf("Other signal good sign\n");
                num_open_fds = 0;
            }
        }
    }
    //free(pfds);
    free(listPkt);
    fprintf(stat,"data_sent: %d\n",pkt_send);
    fprintf(stat,"data_received: %d\n",pkt_receive);
    fprintf(stat,"ack_received: %d\n",ack_received);
    return 0;
}