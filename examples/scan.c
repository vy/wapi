/**
 * Scans available APs in the range using given @a ifname interface.
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
	if (wapi_scan_init(sock, ifname) < 0)
		return;

	/* wait for completion */
	do {
		printf("scan(): sleeptries: %d\n", sleeptries);
		sleep(sleepdur);
		ret = wapi_scan_stat(sock, ifname);
	} while (--sleeptries > 0 && ret > 0);

	/* check wait result */
	if (ret < 0)
	{
		if (!sleeptries)
			fprintf(stderr, "scan(): sleeptries exceeded!\n");
		return;
	}

	/* collect results */
	bzero(&list, sizeof(wapi_list_t));
	if (wapi_scan_coll(sock, ifname, &list) >= 0)
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
