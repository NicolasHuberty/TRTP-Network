#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
int wait_for_client(int sfd){
    char buffer[1024];
    struct sockaddr address;
    socklen_t len;
    int rcvfrom = recvfrom(sfd,buffer,0,MSG_PEEK,&address,&len);
    if(rcvfrom == -1){
        return -1;
    }
    int con = connect(sfd,&address,len);
    if(con == -1){
        return -1;
    }
    return 0;
}