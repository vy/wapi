#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "wapi.h"


/**
 * Gets current configuration of the @a ifname using WAPI accessors and prints
 * them in a pretty fashion with their corresponding return values.
 */
static void
get(int sock, const char *ifname)
{
	int ret;
	double freq;
	wapi_freq_flag_t freq_flag;
	char essid[WAPI_ESSID_MAX_SIZE + 1];
	wapi_essid_flag_t essid_flag;
	wapi_mode_t mode;
	struct sockaddr ap;
	struct ether_addr *ap_addr;
	int bitrate;
	wapi_bitrate_flag_t bitrate_flag;
	int txpower;
	wapi_txpower_flag_t txpower_flag;

	/* freq */
	ret = wapi_get_freq(sock, ifname, &freq, &freq_flag);
	printf("wapi_get_freq(): ret: %d", ret);
	if (ret >= 0)
		printf(", freq: %g, freq_flag: %s", freq, wapi_freq_flags[freq_flag]);
	putchar('\n');

	/* essid */
	ret = wapi_get_essid(sock, ifname, essid, &essid_flag);
	printf("wapi_get_essid(): ret: %d", ret);
	if (ret >= 0)
		printf(
			", essid: %s, essid_flag: %s",
			essid, wapi_essid_flags[essid_flag]);
	putchar('\n');

	/* operating mode */
	ret = wapi_get_mode(sock, ifname, &mode);
	printf("wapi_get_mode(): ret: %d", ret);
	if (ret >= 0)
		printf(", mode: %s", wapi_modes[mode]);
	putchar('\n');

	/* ap */
	ret = wapi_get_ap(sock, ifname, &ap);
	ap_addr = (struct ether_addr *) &ap.sa_data;
	printf("wapi_get_ap(): ret: %d", ret);
	if (ret >= 0)
		printf(
			", ap: %02X:%02X:%02X:%02X:%02X:%02X",
			ap_addr->ether_addr_octet[0],
			ap_addr->ether_addr_octet[1],
			ap_addr->ether_addr_octet[2],
			ap_addr->ether_addr_octet[3],
			ap_addr->ether_addr_octet[4],
			ap_addr->ether_addr_octet[5]);
	putchar('\n');

	/* bitrate */
	ret = wapi_get_bitrate(sock, ifname, &bitrate, &bitrate_flag);
	printf("wapi_get_bitrate(): ret: %d", ret);
	if (ret >= 0)
		printf(
			", bitrate: %d, bitrate_flag: %s", bitrate,
			wapi_bitrate_flags[bitrate_flag]);
	putchar('\n');

	/* txpower */
	ret = wapi_get_txpower(sock, ifname, &txpower, &txpower_flag);
	printf("wapi_get_txpower(): ret: %d", ret);
	if (ret >= 0)
		printf(
			", txpower: %d, txpower_flag: %s",
			txpower, wapi_txpower_flags[txpower_flag]);
	putchar('\n');
}


/**
 * Tries to alter configuration parameters of supplied @a ifname interface. Pay
 * attention that, setters require root privileges. Moreover, a majority of the
 * commodity wireless drivers provide access to alter only a particular subset
 * of the available configurations.
 */
static void __attribute__((unused))
set(int sock, const char *ifname)
{
	int ret;
	struct sockaddr ap;

	/* freq */
	ret = wapi_set_freq(sock, ifname, 2.462e09, WAPI_FREQ_FIXED);
	printf("wapi_set_freq(): ret: %d\n", ret);

	/* essid */
	ret = wapi_set_essid(sock, ifname, "moo", WAPI_ESSID_ON);
	printf("wapi_set_essid(): ret: %d\n", ret);

	/* mode */
	ret = wapi_set_mode(sock, ifname, WAPI_MODE_MANAGED);
	printf("wapi_set_mode(): ret: %d\n", ret);

	/* ap */
	wapi_make_broad_ether(&ap);
	ret = wapi_set_ap(sock, ifname, &ap);
	printf("wapi_set_ap(): ret: %d\n", ret);

	/* bitrate */
	ret = wapi_set_bitrate(sock, ifname, -1, WAPI_BITRATE_AUTO);
	printf("wapi_set_bitrate(): ret: %d\n", ret);

	/* txpower */
	ret = wapi_set_txpower(sock, ifname, 20, WAPI_TXPOWER_DBM);
	printf("wapi_set_txpower(): ret: %d\n", ret);
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
		printf(">> @%xl %s\n", (size_t) info, (info->has_essid ? info->essid : ""));
}


int
main(int argc, char *argv[])
{
	const char *ifname;
	wapi_list_t names;
	int ret;
	int sock;

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <IFNAME>\n", argv[0]);
		return EXIT_FAILURE;
	}
	ifname = argv[1];

	bzero(&names, sizeof(wapi_list_t));
	ret = wapi_get_ifnames(&names);
	printf("wapi_get_ifnames(): ret: %d", ret);
	if (ret >= 0)
	{
		wapi_string_t *str;

		printf(", ifnames:");
		for (str = names.head.string; str; str = str->next)
			printf(" %s", str->data);
	}
	putchar('\n');

	sock = wapi_make_socket();
	printf("wapi_make_socket(): sock: %d\n", sock);

	printf("\ninitial conf\n");
	printf("------------\n");
	get(sock, ifname);

#ifdef ENABLE_SET
	printf("\nchanging conf...\n");
	set(sock, ifname);

	printf("\nfinal conf\n");
	printf("----------\n");
	get(sock, ifname);
#endif

	printf("\nscan\n");
	printf("----\n");
	scan(sock, ifname);

	return EXIT_SUCCESS;
}
