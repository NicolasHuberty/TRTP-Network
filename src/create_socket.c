#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
int create_socket(struct sockaddr_in6 *source_addr,int src_port,struct sockaddr_in6 *dest_addr,int dst_port){
    int sockfd = socket(AF_INET6,SOCK_DGRAM,0);
    //int opt = 1; //To remove
    //setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,sizeof(opt)); // To remove
    if(sockfd == -1){
        perror("Socket failure");
        return -1;
    }
    if(source_addr != NULL){
        if(src_port > 0){
            source_addr->sin6_port = htons(src_port);
        }
        int bindfd = bind(sockfd,(struct sockaddr*)source_addr,sizeof(struct sockaddr_in6));
        if(bindfd == -1){
            perror("Bind failure");
            return -1;
        }
    }
    if(dest_addr != NULL){
        if(dst_port > 0){
            dest_addr->sin6_port = htons(dst_port);
        }
        int connectfd = connect(sockfd,(struct sockaddr*)dest_addr,sizeof(struct sockaddr_in6));
        if(connectfd==-1){
            perror("Connect failure");
            return -1;
        }
    }
    printf("Socket send: %d\n",sockfd);
    return sockfd;
}