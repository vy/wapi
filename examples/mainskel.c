#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "wapi.h"


#include "conf.c"
#include "scan.c"
#include "ifnames.c"
#include "routes.c"


static int
mainskel(int argc, char *argv[], int set_ok)
{
	const char *ifname;
	int sock;

	/* check command line args */
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <IFNAME>\n", argv[0]);
		return EXIT_FAILURE;
	}
	ifname = argv[1];

	/* get ifnames */
	getifnames();
	putchar('\n');

	/* get routes */
	printf("\nroutes:\n");
	printf("-------\n");
	getroutes();
	putchar('\n');

	/* make a comm. sock. */
	sock = wapi_make_socket();
	if (sock >= 0)
	{
		/* list conf */
		printf("\nconf\n");
		printf("------------\n");
		conf(sock, ifname, set_ok);

		/* scan aps */
		printf("\nscan\n");
		printf("----\n");
		scan(sock, ifname);

		/* close comm. sock. */
		close(sock);
	}

	return EXIT_SUCCESS;
}
