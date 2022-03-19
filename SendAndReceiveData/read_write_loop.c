#include <poll.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
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
    while(open > 0){
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
            char buf[10];
            if(fds[j].revents != 0){
                printf("New event !!");
                if(fds[j].revents & POLLIN){
                    ssize_t s = read(fds[j].fd,buf,sizeof(buf));
                    if(s==-1){
                        perror("Read failure");
                        return;
                    }
                    int e = write(sfd,buf,s);
                    e = write(0,buf,s);
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