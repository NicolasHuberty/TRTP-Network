#include <poll.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include "packet_implem.c"

void read_write_loop(int sfd){
    fprintf(stdout,"Start\n");
    int nfds = 2;
    int open = 2;
    struct pollfd *fds;
    fds = calloc(nfds,sizeof(struct pollfd));
    if(!fds){
        perror("Calloc failure");
        return;
    }
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = sfd;
    fds[1].events = POLLIN;
    while(open > 1){
        printf("Before poll\n");
        int ready;
        ready = poll(fds,nfds,-1);
        printf("After poll\n");
        if(ready==-1){
            perror("Poll failure");
            return;
        }
        for (int j = 0; j < nfds; j++)
        {
            
            char buf[1000];
            if(fds[j].revents != 0){
                printf("New event !!");
                if(fds[j].revents & POLLIN){
                    printf("You are here %x\n",fds[j].revents);
                    ssize_t s = read(fds[j].fd,buf,sizeof(buf));
                    if(s==-1){
                        perror("Read failure");
                        return;
                    }
                    int e = write(sfd,buf,s);
                    pkt_t *new_pkt = pkt_new();
                    pkt_decode(buf,1000,new_pkt);
                    printf("TR Val: %d\n",pkt_get_tr(new_pkt));
                    printf("Pkt size: %d\n",pkt_get_length(new_pkt));
                    printf("Pkt payload:%s\n",pkt_get_payload(new_pkt));
                    //e = write(0,buf,s);
                }else{
                    if(fds[j].revents & POLLOUT){
                        printf("Tiens tiens tiens\n");
                    }
                    if(close(fds[j].fd)==-1){
                        perror("Close failure");
                        return;
                    }
                    open--;
                }
            }
        }
        
    }
    printf("End !");
}