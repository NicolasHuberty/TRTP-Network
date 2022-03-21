#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include "log.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "real_adress.c"
#include "create_socket.c"
#include "wait_for_client.c"
#include "read_write_loop.c"
#include <sys/stat.h>
#include <fcntl.h>
#include "read_write_loop_sender.c"
//#include "packet_implem.c"
int print_usage(char *prog_name) {
    ERROR("Usage:\n\t%s [-f filename] [-s stats_filename] [-c] receiver_ip receiver_port", prog_name);
    return EXIT_FAILURE;
}

char buf[1024];
int main(int argc, char **argv) {
    int opt;

    char *filename = NULL;
    char *stats_filename = NULL;
    char *receiver_ip = NULL;
    char *receiver_port_err;
    bool fec_enabled = false;
    uint16_t receiver_port;

    while ((opt = getopt(argc, argv, "f:s:hc")) != -1) {
        switch (opt) {
        case 'f':
            filename = optarg;
            break;
        case 'h':
            return print_usage(argv[0]);
        case 's':
            stats_filename = optarg;
            break;
	case 'c':
	    fec_enabled = true;
	    break;
        default:
            return print_usage(argv[0]);
        }
    }

    if (optind + 2 != argc) {
        ERROR("Unexpected number of positional arguments");
        return print_usage(argv[0]);
    }
    FILE *fd;
    fd = fopen(filename,"r");

    receiver_ip = argv[optind];
    receiver_port = (uint16_t) strtol(argv[optind + 1], &receiver_port_err, 10);
    if (*receiver_port_err != '\0') { 
        ERROR("Receiver port parameter is not a number");
        return print_usage(argv[0]);
    }

    ASSERT(1 == 1); // Try to change it to see what happens when it fails
    DEBUG_DUMP("Some bytes", 11); // You can use it with any pointer type

    // This is not an error per-se.
    //ERROR("Sender has following arguments: filename is %s, stats_filename is %s, fec_enabled is %d, receiver_ip is %s, receiver_port is %u",
        //filename, stats_filename, fec_enabled, receiver_ip, receiver_port);

    DEBUG("You can only see me if %s", "you built me using `make debug`");
    //ERROR("This is not an error, %s", "now let's code!");

    // Now let's code!
    struct sockaddr_in6 addr;
	const char *err = real_address(receiver_ip, &addr);
	if (err) {
		perror("Real address failed");
		return EXIT_FAILURE;
	}
    int sfd = create_socket(NULL, -1, &addr, receiver_port);
    if(sfd == -1){
        perror("Socket failure");
        return EXIT_FAILURE;
    }
    //put_on_pkt(fd);
    //send(sfd,buf,sizeof(buf),0);
    read_write_loop_sender(sfd,fd);
    return EXIT_SUCCESS;
}

