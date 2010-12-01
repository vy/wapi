#include <stdio.h>
#include <stdlib.h>

#include "wapi.h"


int
main(int argc, char *argv[])
{
	int ret;

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <IFNAME>\n", argv[0]);
		return EXIT_FAILURE;
	}

	ret = wapi_if_del(-1, argv[1]);
	fprintf(stderr, "wapi_if_del(): ret: %d\n", ret);

	return ret;
}
