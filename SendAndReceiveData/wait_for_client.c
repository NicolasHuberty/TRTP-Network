#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
int wait_for_client(int sfd){
    fprintf(stdout,"Start wait for client\n");
    char buffer[1024];
    struct sockaddr address;
    socklen_t len;
    fprintf(stdout,"Just before rcvfrom\n");
    int rcvfrom = recvfrom(sfd,buffer,0,MSG_PEEK,&address,&len);
    fprintf(stdout,"Just after rcvfrom\n");
    if(rcvfrom == -1){
        perror("Receive from failure");
        return -1;
    }
    int con = connect(sfd,&address,len);
    if(con == -1){
        perror("Connect failure");
        return -1;
    }
    return 0;
}