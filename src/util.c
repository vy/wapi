#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "wapi.h"
#include "util.h"


int
wapi_make_socket(void)
{
	return socket(AF_INET, SOCK_DGRAM, 0);
}


int
wapi_get_ifnames(wapi_list_t *list)
{
	FILE *fp;
	int ret;
	char tmp[1024]; /* Can a line be longer than 1024 bytes? */

	WAPI_VALIDATE_PTR(list);

	/* Open file for reading. */
	fp = fopen(WAPI_PROC_NET_WIRELESS, "r");
	if (!fp)
	{
		WAPI_STRERROR("fopen(\"%s\", \"r\")", WAPI_PROC_NET_WIRELESS);
		return -1;
	}

	/* Skip first two lines. */
	fgets(tmp, sizeof(tmp), fp);
	fgets(tmp, sizeof(tmp), fp);

	/* Iterate over available lines. */
	ret = 0;
	while (fgets(tmp, sizeof(tmp), fp))
	{
		char *beg;
		char *end;
		wapi_string_t *string;

		/* Locate the interface name region. */
		for (beg = tmp; *beg && isspace(*beg); beg++);
		for (end = beg; *end && *end != ':'; end++);

		/* Allocate both wapi_string_t and char vector. */
		string = malloc(sizeof(wapi_string_t));
		if (string) string->data = malloc(end - beg + sizeof(char));
		if (!string || !string->data)
		{
			WAPI_STRERROR("malloc()");
			ret = -1;
			break;
		}

		/* Copy region into the buffer. */
		snprintf(string->data, (end - beg + sizeof(char)), "%s", beg);

		/* Push string into the list. */
		string->next = list->head.string;
		list->head.string = string;
	}

	fclose(fp);
	return ret;
}
