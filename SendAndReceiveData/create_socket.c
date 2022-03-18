#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

int create_socket(struct sockaddr_in6 *source_addr,int src_port,struct sockaddr_in6 *dest_addr,int dst_port){

    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    /*int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,sizeof(opt)) != 0) {
        perror("SetSocket error");
        exit(EXIT_FAILURE);
    }*/
    if(sock == -1) perror("Socket error");
    source_addr->sin6_port = htons(src_port);
    if(source_addr != NULL){
        int bindf = bind(sock, (struct sockaddr *) source_addr, sizeof(struct sockaddr_in6));
        if(bindf == -1) perror("Bind error: ");return -1;
    }
    dest_addr->sin6_family = AF_INET6;
    dest_addr->sin6_port = htons(dst_port);
    if(dest_addr != NULL){
        int connectfd = connect(sock, (struct sockaddr *) dest_addr, sizeof(dest_addr));
        if(connectfd == 1) perror("Connect error: ");
    }
    return  0;
}