#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
int wait_for_client(int sfd){
    char buffer[0];
    struct sockaddr_in6 address;
    socklen_t len = sizeof(address);
    int rcvfrom = recvfrom(sfd,buffer,0,MSG_PEEK,(struct sockaddr*)&address,(socklen_t*)&len);
    if(rcvfrom == -1){
        return -1;
    }
    int con = connect(sfd,(struct sockaddr*)&address,len);
    if(con == -1){
        return -1;
    }
    return 0;
}