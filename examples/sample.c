#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "wapi.h"


/**
 * Gets current configuration of the @a ifname using WAPI accessors and prints
 * them in a pretty fashion with their corresponding return values. If a getter
 * succeeds, we try to set that property with the same value to test the setters
 * as well.
 */
static void
conf(int sock, const char *ifname)
{
	int ret;
	struct in_addr addr;
	double freq;
	wapi_freq_flag_t freq_flag;
	char essid[WAPI_ESSID_MAX_SIZE + 1];
	wapi_essid_flag_t essid_flag;
	wapi_mode_t mode;
	struct ether_addr ap;
	int bitrate;
	wapi_bitrate_flag_t bitrate_flag;
	int txpower;
	wapi_txpower_flag_t txpower_flag;

	/* get ip */
	bzero(&addr, sizeof(struct in_addr));
	ret = wapi_get_ip(sock, ifname, &addr);
	printf("wapi_get_ip(): ret: %d", ret);
	if (ret >= 0)
	{
		printf(", ip: %s", inet_ntoa(addr));

#ifdef ENABLE_SET
		/* set ip (Make sure sin.sin_family is set to AF_INET.) */
		ret = wapi_set_ip(sock, ifname, &addr);
		printf("\nwapi_set_ip(): ret: %d", ret);
#endif
	}
	putchar('\n');

	/* get netmask */
	bzero(&addr, sizeof(struct in_addr));
	ret = wapi_get_netmask(sock, ifname, &addr);
	printf("wapi_get_netmask(): ret: %d", ret);
	if (ret >= 0)
	{
		printf(", netmask: %s", inet_ntoa(addr));

#ifdef ENABLE_SET
		/* set netmask (Make sure sin.sin_family is set to AF_INET.) */
		ret = wapi_set_netmask(sock, ifname, &addr);
		printf("\nwapi_set_netmask(): ret: %d", ret);
#endif
	}
	putchar('\n');

	/* get freq */
	ret = wapi_get_freq(sock, ifname, &freq, &freq_flag);
	printf("wapi_get_freq(): ret: %d", ret);
	if (ret >= 0)
	{
		int chan;
		double tmpfreq;

		printf(", freq: %g, freq_flag: %s", freq, wapi_freq_flags[freq_flag]);

		ret = wapi_freq2chan(sock, ifname, freq, &chan);
		printf("\nwapi_freq2chan(): ret: %d", ret);
		if (ret >= 0) printf(", chan: %d", chan);

		ret = wapi_chan2freq(sock, ifname, chan, &tmpfreq);
		printf("\nwapi_chan2freq(): ret: %d", ret);
		if (ret >= 0) printf(", freq: %g", tmpfreq);

#ifdef ENABLE_SET
		/* set freq */
		ret = wapi_set_freq(sock, ifname, freq, freq_flag);
		printf("\nwapi_set_freq(): ret: %d", ret);
#endif
	}
	putchar('\n');

	/* get essid */
	ret = wapi_get_essid(sock, ifname, essid, &essid_flag);
	printf("wapi_get_essid(): ret: %d", ret);
	if (ret >= 0)
	{
		printf(
			", essid: %s, essid_flag: %s",
			essid, wapi_essid_flags[essid_flag]);

#ifdef ENABLE_SET
		/* set essid */
		ret = wapi_set_essid(sock, ifname, essid, essid_flag);
		printf("\nwapi_set_essid(): ret: %d", ret);
#endif
	}
	putchar('\n');

	/* get operating mode */
	ret = wapi_get_mode(sock, ifname, &mode);
	printf("wapi_get_mode(): ret: %d", ret);
	if (ret >= 0)
	{
		printf(", mode: %s", wapi_modes[mode]);

#ifdef ENABLE_SET
		/* set operating mode */
		ret = wapi_set_mode(sock, ifname, mode);
		printf("\nwapi_set_mode(): ret: %d", ret);
#endif
	}
	putchar('\n');

	/* get ap */
	ret = wapi_get_ap(sock, ifname, &ap);
	printf("wapi_get_ap(): ret: %d", ret);
	if (ret >= 0)
	{
		printf(
			", ap: %02X:%02X:%02X:%02X:%02X:%02X",
			ap.ether_addr_octet[0],
			ap.ether_addr_octet[1],
			ap.ether_addr_octet[2],
			ap.ether_addr_octet[3],
			ap.ether_addr_octet[4],
			ap.ether_addr_octet[5]);

#ifdef ENABLE_SET
		/* set ap */
		ret = wapi_set_ap(sock, ifname, &ap);
		printf("\nwapi_set_ap(): ret: %d", ret);
#endif
	}
	putchar('\n');

	/* get bitrate */
	ret = wapi_get_bitrate(sock, ifname, &bitrate, &bitrate_flag);
	printf("wapi_get_bitrate(): ret: %d", ret);
	if (ret >= 0)
	{
		printf(
			", bitrate: %d, bitrate_flag: %s", bitrate,
			wapi_bitrate_flags[bitrate_flag]);

#ifdef ENABLE_SET
		/* set bitrate */
		ret = wapi_set_bitrate(sock, ifname, bitrate, bitrate_flag);
		printf("\nwapi_set_bitrate(): ret: %d", ret);
#endif
	}
	putchar('\n');

	/* get txpower */
	ret = wapi_get_txpower(sock, ifname, &txpower, &txpower_flag);
	printf("wapi_get_txpower(): ret: %d", ret);
	if (ret >= 0)
	{
		printf(
			", txpower: %d, txpower_flag: %s",
			txpower, wapi_txpower_flags[txpower_flag]);

#ifdef ENABLE_SET
		/* set txpower */
		ret = wapi_set_txpower(sock, ifname, txpower, txpower_flag);
		printf("\nwapi_set_txpower(): ret: %d", ret);
#endif
	}
	putchar('\n');
}


/**
 * Scans available APs in the range using given @a ifname interface. (Requires
 * root privileges to start a scan.)
 */
static void
scan(int sock, const char *ifname)
{
	int sleepdur = 1;
	int sleeptries = 5;
	wapi_list_t list;
	wapi_scan_info_t *info;
	int ret;

	/* start scan */
	ret = wapi_scan_init(sock, ifname);
	printf("wapi_scan_init(): ret: %d\n", ret);

	/* wait for completion */
	do {
		sleep(sleepdur);
		ret = wapi_scan_stat(sock, ifname);
		printf("wapi_scan_stat(): ret: %d, sleeptries: %d\n", ret, sleeptries);
	} while (--sleeptries > 0 && ret > 0);
	if (ret < 0) return;

	/* collect results */
	bzero(&list, sizeof(wapi_list_t));
	ret = wapi_scan_coll(sock, ifname, &list);
	printf("wapi_scan_coll(): ret: %d\n", ret);

	/* print found aps */
	for (info = list.head.scan; info; info = info->next)
		printf(
			">> %02x:%02x:%02x:%02x:%02x:%02x %s\n",
			info->ap.ether_addr_octet[0],
			info->ap.ether_addr_octet[1],
			info->ap.ether_addr_octet[2],
			info->ap.ether_addr_octet[3],
			info->ap.ether_addr_octet[4],
			info->ap.ether_addr_octet[5],
			(info->has_essid ? info->essid : ""));

	/* free ap list */
	info = list.head.scan;
	while (info)
	{
		wapi_scan_info_t *temp;

		temp = info->next;
		free(info);
		info = temp;
	}
}


int
main(int argc, char *argv[])
{
	const char *ifname;
	wapi_list_t list;
	int ret;
	int sock;

	/* check command line args */
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <IFNAME>\n", argv[0]);
		return EXIT_FAILURE;
	}
	ifname = argv[1];

	/* get ifnames */
	bzero(&list, sizeof(wapi_list_t));
	ret = wapi_get_ifnames(&list);
	printf("wapi_get_ifnames(): ret: %d", ret);
	if (ret >= 0)
	{
		wapi_string_t *str;

		/* print ifnames */
		printf(", ifnames:");
		for (str = list.head.string; str; str = str->next)
			printf(" %s", str->data);

		/* free ifnames */
		str = list.head.string;
		while (str)
		{
			wapi_string_t *tmp;

			tmp = str->next;
			free(str->data);
			free(str);
			str = tmp;
		}
	}
	putchar('\n');

	/* get routes */
	bzero(&list, sizeof(wapi_list_t));
	ret = wapi_get_routes(&list);
	printf("wapi_get_routes(): ret: %d\n", ret);
	if (ret >= 0)
	{
		wapi_route_info_t *ri;

		/* print route */
		for (ri = list.head.route; ri; ri = ri->next)
			printf(
				">> dest: %s, gw: %s, netmask: %s\n",
				inet_ntoa(ri->dest), inet_ntoa(ri->gw), inet_ntoa(ri->netmask));

		/* free routes */
		ri = list.head.route;
		while (ri)
		{
			wapi_route_info_t *tmpri;

			tmpri = ri->next;
			free(ri->ifname);
			free(ri);
			ri = tmpri;
		}
	}

	/* make a comm. sock. */
	sock = wapi_make_socket();
	printf("wapi_make_socket(): sock: %d\n", sock);

	/* list conf */
	printf("\nconf\n");
	printf("------------\n");
	conf(sock, ifname);

	/* scan aps */
	printf("\nscan\n");
	printf("----\n");
	scan(sock, ifname);

	/* close comm. sock. */
	close(sock);

	return EXIT_SUCCESS;
}
