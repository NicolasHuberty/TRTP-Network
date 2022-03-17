#include "create_socket.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#define BACKLOG 3
int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port){
    int fdsocket;
    if ((fdsocket = socket(AF_INET6, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr,"Error while socket creation");
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    if (setsockopt(fdsocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,	sizeof(opt)) != 0) {
        fprintf(stderr,"Error while settings the socket");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in6 adresse;

    adresse.sin6_family = AF_INET6;
    // Ecoute sur toutes les adresses (INADDR_ANY <=> 0.0.0.0)
    adresse.sin6_addr = source_addr->sin6_addr;
    // Conversion du port en valeur réseaux (Host TO Network Short)
    adresse.sin6_port = src_port;

    if (bind(fdsocket,(struct sockaddr*)&adresse, sizeof(adresse)) != 0) {
        fprintf(stderr,"Error while binding the socket");
        exit(EXIT_FAILURE);
    }
    if (listen(fdsocket, BACKLOG) != 0) {
        fprintf(stderr,"Error while listening the socket");
        exit(EXIT_FAILURE);
    }
    int clientSocket;
    // Structure contenant l'adresse du client
    struct sockaddr_in6 clientAdresse;
    clientAdresse.sin6_addr = dest_addr->sin6_addr;
    clientAdresse.sin6_port = dst_port;
    unsigned int addrLen = sizeof(clientAdresse);
    if ((clientSocket = accept(fdsocket,(struct sockaddr*)&clientAdresse, &addrLen)) == -1) {
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);

}