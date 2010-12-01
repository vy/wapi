#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wapi.h"


static int
parse_wapi_mode(const char *s, wapi_mode_t *mode)
{
	int ret = 0;

	if 		(!strcmp(s, "auto"))	*mode = WAPI_MODE_AUTO;
	else if (!strcmp(s, "adhoc"))	*mode = WAPI_MODE_ADHOC;
	else if (!strcmp(s, "managed"))	*mode = WAPI_MODE_MANAGED;
	else if (!strcmp(s, "master"))	*mode = WAPI_MODE_MASTER;
	else if (!strcmp(s, "repeat"))	*mode = WAPI_MODE_REPEAT;
	else if (!strcmp(s, "second"))	*mode = WAPI_MODE_SECOND;
	else if (!strcmp(s, "monitor"))	*mode = WAPI_MODE_MONITOR;
	else ret = 1;

	return ret;
}


int
main(int argc, char *argv[])
{
	wapi_mode_t mode;
	const char *ifname;
	const char *name;
	int ret;

	if (argc != 4)
	{
		fprintf(stderr, "Usage: %s <IFNAME> <NAME> <MODE>\n", argv[0]);
		return EXIT_FAILURE;
	}
	ifname = argv[1];
	name = argv[2];

	if (parse_wapi_mode(argv[3], &mode))
	{
		fprintf(stderr, "Unknown mode: %s!\n", argv[3]);
		return EXIT_FAILURE;
	}

	ret = wapi_if_add(-1, ifname, name, mode);
	fprintf(stderr, "wapi_if_add(): ret: %d\n", ret);

	return ret;
}
