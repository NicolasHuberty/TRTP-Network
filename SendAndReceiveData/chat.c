#include <stdlib.h> /* EXIT_X */
#include <stdio.h> /* fprintf */
#include <unistd.h> /* getopt */
#include <errno.h>

#include "real_address.c"
#include "create_socket.c"
#include "read_write_loop.c"
#include "wait_for_client.c"

int main(int argc, char *argv[])
{
	int client = 0;
	int port = 12345;
	int opt;
	char *host = "::1";

	while ((opt = getopt(argc, argv, "scp:h:")) != -1) {
		switch (opt) {
			case 's':
				client = 0;
				break;
			case 'c':
				client = 1;
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'h':
				host = optarg;
				break;
			default:
				fprintf(stderr, "Usage:\n"
								"-s      Act as server\n"
								"-c      Act as client\n"
								"-p PORT UDP port to connect to (client)\n"
								"        or to listen on (server)\n"
								"-h HOST UDP of the server (client)\n"
								"        or on which we listen (server)\n");
				break;
		}
	}
	/* Resolve the hostname */
	struct sockaddr_in6 addr;
	const char *err = real_address(host, &addr);
	if (err) {
		fprintf(stderr, "Could not resolve hostname %s: %s\n", host, err);
		return EXIT_FAILURE;
	}
	/* Get a socket */
	int sfd;
	if (client) {
		sfd = create_socket(NULL, -1, &addr, port); /* Connected */

	} else {
		sfd = create_socket(&addr, port, NULL, -1); /* Bound */
		printf("Sock val :(() %d\n",sfd);
		if (sfd > 0 && wait_for_client(sfd) < 0) { /* Connected */
			printf("Not at end:/\n");
			fprintf(stderr,
					"Could not connect the socket after the first message.\n");
			close(sfd);
			return EXIT_FAILURE;
		}
		printf("Niceee\n");
	}
	if (sfd < 0) {
		printf("Errooor\n");
		perror("Bad sfd");
		return EXIT_FAILURE;
	}
	/* Process I/O */
	printf("Start read/write\n");
	read_write_loop(sfd);
	printf("End read write\n");

	close(sfd);

	return EXIT_SUCCESS;
}
