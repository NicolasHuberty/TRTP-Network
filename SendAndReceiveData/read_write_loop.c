#include <poll.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
void read_write_loop(int sfd){
    printf("Start");
    int nfds = 2;
    int open = 2;
    struct pollfd *fds;
    fds = calloc(nfds,sizeof(struct pollfd));
    if(!fds){
        perror("Calloc failure");
        return;
    }
    fds[0].fd = sfd;
    fds[0].events = POLLIN;
    fds[1].fd = sfd;
    fds[1].events = POLLIN;
    while(open > 0){
        int ready;
        ready = poll(fds,nfds,-1);
        if(ready==-1){
            perror("Poll failure");
            return;
        }
        for (int j = 0; j < nfds; j++)
        {
            char buf[10];
            if(fds[j].revents != 0){
                printf("New event !!");
                if(fds[j].revents & POLLIN){
                    ssize_t s = read(fds[j].fd,buf,sizeof(buf));
                    if(s==-1){
                        perror("Read failure");
                        return;
                    }
                }else{
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