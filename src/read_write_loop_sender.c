#include <poll.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int read_write_loop_sender(int sfd,FILE *fd){
    int wait_an_ack = 0;
    int window = 1;
    int seqnum = 0;
    int seqnum_valid = 0;
    int seqnum_send = 0;
    int nfds,num_open_fds;
    int end = 0;
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
                pkt_t *pkt = pkt_new();
                int len = 1024;
                pkt_decode(buf,len,pkt);
                if(pkt_get_type(pkt)==PTYPE_ACK){
                    window = pkt_get_window(pkt);
                    printf("acquittement receive for packet %d\n",pkt_get_seqnum(pkt));
                    if(pkt_get_seqnum(pkt)>seqnum_valid){
                        seqnum_valid = pkt_get_seqnum(pkt);
                    }
                    if(s == -1){
                        perror("Read failure");
                        exit(EXIT_FAILURE);
                    }
                }
                pkt_del(pkt);
                //Deal the entry pkt
            }
        }
        if(pfds[0].revents != 0){
            //Normally ready to write
            if(pfds[0].revents & POLLOUT){
                if(!wait_an_ack){
                    while(seqnum_send - window <= seqnum_valid){
                        char buffer[MAX_BUFFER_SIZE];
                        char *content = malloc(sizeof(MAX_BUFFER_SIZE -sizeof(pkt_t)));
                        //Start read file
                        fread(content,sizeof(char),sizeof(content),fd);
                        put_on_pkt(buffer,content,seqnum_send++);
                        free(content);
                        write(sfd,buffer,sizeof(buffer));
                        if(feof(fd)){
                            fclose(fd);
                            close(pfds[0].fd);
                            close(pfds[1].fd);
                            num_open_fds = 0;

                        }
                        printf("Packet number %d send\n",seqnum_send -1);

                    }
                }
                    
                }
        }
    }
    free(pfds);
    return 0;
}