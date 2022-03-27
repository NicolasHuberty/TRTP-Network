#include <poll.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int read_write_loop_sender(int sfd,FILE *fd,FILE *stat){
    int wait_an_ack = 0;
    int window = 1;
    //int seqnum_valid = 0;
    int pkt_send = 0;
    int pkt_receive = 0;
    //int fec_send = 0;
    //int data_tr = 0;
    int ack_received = 0;
    int nack_received = 0;
    //int packets_ignored = 0;
    int seqnum_send = 0;
    int nfds,num_open_fds;
    //int nack = 0;
    struct pollfd *pfds;
    pfds = calloc(2,sizeof(struct pollfd));
    if(!pfds){
        perror("Calloc failure");
        exit(EXIT_FAILURE);
    }
    num_open_fds = nfds = 2;
    // Write the file 
    pfds[0].fd = sfd;
    pfds[0].events = POLLOUT;
    // Read the socket
    pfds[1].fd = sfd;
    pfds[1].events = POLLIN;
    while(num_open_fds > 0){
        int ready = poll(pfds,nfds,-1); // Change -1 for timeout
        if(ready == -1){
            perror("Poll failure");
            exit(EXIT_FAILURE);
        }
        //Receive data from the socket
        if(pfds[1].revents != 0){
            if(pfds[1].revents & POLLIN){
                char buf[MAX_BUFFER_SIZE];
                ssize_t s = read(pfds[1].fd,buf,sizeof(buf));
                pkt_receive++;
                pkt_t *pkt = pkt_new();
                int len = 1024;
                pkt_decode(buf,len,pkt);
                if(pkt_get_type(pkt)==PTYPE_ACK){
                    ack_received++;
                    window = pkt_get_window(pkt);
                    DEBUG("acquittement receive for packet %d\n",pkt_get_seqnum(pkt));
                    seqnum_send = pkt_get_seqnum(pkt);
                    wait_an_ack = 0;
                    if(s == -1){
                        perror("Read failure");
                        exit(EXIT_FAILURE);
                    }
                }else{ //Case of nack
                    nack_received++;
                }
                pkt_del(pkt);
                //Deal the entry pkt
            }
        }
        if(pfds[0].revents != 0){
            //Try to send data
            if(pfds[0].revents & POLLOUT){
                if(!wait_an_ack){
                    while(num_open_fds > 0 && seqnum_send < window){
                        DEBUG("Seqnum send %d and window: %d\n",seqnum_send,window);
                        char buffer[MAX_BUFFER_SIZE];
                        char *content = malloc(sizeof(MAX_BUFFER_SIZE -sizeof(pkt_t)));
                        //Start read file
                        int r = fread(content,sizeof(char),sizeof(content),fd);
                        if(r==-1){
                            perror("Read failure");
                            exit(EXIT_FAILURE);
                        }
                        put_on_pkt(buffer,content,seqnum_send++,window,0);
                        free(content);
                        int w = write(sfd,buffer,sizeof(buffer));
                        printf("Payload nb %d sent\n",seqnum_send);
                        pkt_send++;
                        if(w==-1){
                            perror("Read failure");
                            exit(EXIT_FAILURE);
                        }
                        if(feof(fd)){
                            DEBUG("Try to close the file because end\n");
                            
                            char buffer[MAX_BUFFER_SIZE];
                            pkt_t *pkt = pkt_new();
                            put_on_pkt(buffer,pkt,0,0,1);
                            int wr = write(sfd,buffer,sizeof(buffer));
                            if(wr == -1){
                                DEBUG("Signal end not write\n");
                            }
                            fclose(fd);
                            close(pfds[0].fd);
                            close(pfds[1].fd);
                            num_open_fds = 0;

                        }
                        DEBUG("Packet number %d send\n",seqnum_send -1);

                    }
                    wait_an_ack = 1;
                }
                    
                }
        }
    }
    free(pfds);
    fprintf(stat,"data_sent: %d\n",pkt_send);
    fprintf(stat,"data_received: %d\n",pkt_receive);
    fprintf(stat,"ack_received: %d\n",ack_received);
    return 0;
}