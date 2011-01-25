#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timeb.h>

#include "wapi.h"


static inline unsigned int
epoch_millitm(void)
{
	struct timeb t;
	ftime(&t);
	return (t.time * 1000) + t.millitm;
}


static int
recover_for_essid(
	int sock, const char *ifname, const char *essid, unsigned int maxdur)
{
	const unsigned int maxtm = epoch_millitm() + maxdur;
	const unsigned int sleepdur = 100;
	wapi_list_t list;
	wapi_scan_info_t *info;
	double freq;
	int ret;

scan:
	/* Initiate scan. */
	if ((ret = wapi_scan_init(sock, ifname)) < 0)
		return ret;

	/* Wait for scan to complete. */
	for (;epoch_millitm() <= maxtm; usleep(sleepdur * 1000))
		switch ((ret = wapi_scan_stat(sock, ifname)))
		{
		case 0: break;
		case 1: continue;
		default: return ret;
		}

	/* Data is not ready. */
	if (ret == 1) return 1;

	/* Collect results. */
	if ((ret = wapi_scan_coll(sock, ifname, &list)) < 0)
		return ret;

	/* See if we have an AP with the given ESSID. */
	freq = -1;
	for (info = list.head.scan; info; info = info->next)
		if (info->has_essid && info->has_freq && !strcmp(info->essid, essid))
		{
			freq = info->freq;
			break;
		}

	/* If no such AP, try again. */
	if (freq < 0)
	{
		ret = 1;
		goto scan;
	}

	/* Switch to AP freq and ESSID. */
	if ((ret = wapi_set_freq(sock, ifname, freq, WAPI_FREQ_FIXED)) > 0)
		ret = wapi_set_essid(sock, ifname, essid, WAPI_ESSID_ON);

	return ret;
}


int
main(int argc, char *argv[])
{
	const char *ifname;
	const char *essid;
	unsigned int maxdur;
	int sock;
	int ret;

	/* Parse command line arguments. */
	if (argc != 4)
	{
		fprintf(stderr, "Usage: %s <IFNAME> <ESSID> <MAXDUR>\n", argv[0]);
		return EXIT_FAILURE;
	}
	ifname = argv[1];
	essid = argv[2];
	maxdur = atoi(argv[3]);

	if ((sock = wapi_make_socket()) < 0) return EXIT_FAILURE;
	ret = recover_for_essid(sock, ifname, essid, maxdur);
	close(sock);

	return ret;
}
