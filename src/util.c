/**
 * @file
 * Utility routines.
 */


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

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
	size_t tmpsize = WAPI_PROC_LINE_SIZE * sizeof(char);
	char tmp[WAPI_PROC_LINE_SIZE];

	WAPI_VALIDATE_PTR(list);

	/* Open file for reading. */
	fp = fopen(WAPI_PROC_NET_WIRELESS, "r");
	if (!fp)
	{
		WAPI_STRERROR("fopen(\"%s\", \"r\")", WAPI_PROC_NET_WIRELESS);
		return -1;
	}

	/* Skip first two lines. */
	if (!fgets(tmp, tmpsize, fp) || ! fgets(tmp, tmpsize, fp))
	{
		WAPI_ERROR("Invalid \"%s\" content!\n", WAPI_PROC_NET_WIRELESS);
		return -1;
	}

	/* Iterate over available lines. */
	ret = 0;
	while (fgets(tmp, tmpsize, fp))
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


#define wapi_ioctl_command_name_bufsiz 128	/* Is fairly enough to print an integer. */
static char wapi_ioctl_command_name_buf[wapi_ioctl_command_name_bufsiz];


const char *
wapi_ioctl_command_name(int cmd)
{
	switch (cmd)
	{
	case SIOCADDRT:		return "SIOCADDRT";
	case SIOCDELRT:		return "SIOCDELRT";
	case SIOCGIFADDR:	return "SIOCGIFADDR";
	case SIOCGIWAP:		return "SIOCGIWAP";
	case SIOCGIWESSID:	return "SIOCGIWESSID";
	case SIOCGIWFREQ:	return "SIOCGIWFREQ";
	case SIOCGIWMODE:	return "SIOCGIWMODE";
	case SIOCGIWRANGE:	return "SIOCGIWRANGE";
	case SIOCGIWRATE:	return "SIOCGIWRATE";
	case SIOCGIWSCAN:	return "SIOCGIWSCAN";
	case SIOCGIWTXPOW:	return "SIOCGIWTXPOW";
	case SIOCSIFADDR:	return "SIOCSIFADDR";
	case SIOCSIWAP:		return "SIOCSIWAP";
	case SIOCSIWESSID:	return "SIOCSIWESSID";
	case SIOCSIWFREQ:	return "SIOCSIWFREQ";
	case SIOCSIWMODE:	return "SIOCSIWMODE";
	case SIOCSIWRATE:	return "SIOCSIWRATE";
	case SIOCSIWSCAN:	return "SIOCSIWSCAN";
	case SIOCSIWTXPOW:	return "SIOCSIWTXPOW";
	default:
		snprintf(
			wapi_ioctl_command_name_buf, wapi_ioctl_command_name_bufsiz,
			"0x%x", cmd);
		return wapi_ioctl_command_name_buf;
	}
}
