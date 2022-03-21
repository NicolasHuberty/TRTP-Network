#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include "log.h"
#include "real_adress.c"
#include "create_socket.c"
#include "wait_for_client.c"
#include "read_write_loop.c"
int print_usage(char *prog_name) {
    ERROR("Usage:\n\t%s [-s stats_filename] listen_ip listen_port", prog_name);
    return EXIT_FAILURE;
}


int main(int argc, char **argv) {
    int opt;

    char *stats_filename = NULL;
    char *listen_ip = NULL;
    char *listen_port_err;
    uint16_t listen_port;

    while ((opt = getopt(argc, argv, "s:h")) != -1) {
        switch (opt) {
        case 'h':
            return print_usage(argv[0]);
        case 's':
            stats_filename = optarg;
            break;
        default:
            return print_usage(argv[0]);
        }
    }

    if (optind + 2 != argc) {
        ERROR("Unexpected number of positional arguments");
        return print_usage(argv[0]);
    }

    listen_ip = argv[optind];
    listen_port = (uint16_t) strtol(argv[optind + 1], &listen_port_err, 10);
    if (*listen_port_err != '\0') {
        ERROR("Receiver port parameter is not a number");
        return print_usage(argv[0]);
    }

    ASSERT(1 == 1); // Try to change it to see what happens when it fails
    DEBUG_DUMP("Some bytes", 11); // You can use it with any pointer type

    // This is not an error per-se.
    ERROR("Receiver has following arguments: stats_filename is %s, listen_ip is %s, listen_port is %u",
        stats_filename, listen_ip, listen_port);

    DEBUG("You can only see me if %s", "you built me using `make debug`");
    ERROR("This is not an error, %s", "now let's code!");

    // Now let's code!
    struct sockaddr_in6 addr;
	const char *err = real_address(listen_ip, &addr);
	if (err) {
		perror("Real address failed");
		return EXIT_FAILURE;
	}
    int sfd = create_socket(&addr,listen_port,NULL,-1);
	if(sfd > 0 && wait_for_client(sfd) < 0){
		fprintf(stderr, "Could not connect to client after the fist message\n");
		close(sfd);
		return EXIT_FAILURE;
	}
    if(sfd == -1){
        perror("Socket failure");
        return EXIT_FAILURE;
    }
    read_write_loop(sfd);
    return EXIT_SUCCESS;
}