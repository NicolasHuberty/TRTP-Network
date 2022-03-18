#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
const char * real_address(const char *address, struct sockaddr_in6 *rval){
    struct addrinfo hints;
    struct addrinfo *result;
    int s;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;    
    hints.ai_socktype = SOCK_DGRAM;   
    s = getaddrinfo(address, NULL, &hints, &result);
    if (s != 0) {
        perror("getaddrinfo failure")
        exit(EXIT_FAILURE);
    }
    memcpy(rval,result->ai_addr,sizeof(struct sockaddr_in6));
    freeaddrinfo(result);        
    return 0;
}