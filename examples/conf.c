/**
 * Gets current configuration of the @a ifname using WAPI accessors and prints
 * them in a pretty fashion. If a getter succeeds (and if @a set_ok is set), we
 * try to set that property with the same value to test the setters as well.
 */
static void
conf(int sock, const char *ifname, int set_ok)
{
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
	if (wapi_get_ip(sock, ifname, &addr) >= 0)
	{
		printf("ip: %s\n", inet_ntoa(addr));

		if (set_ok)
			/* set ip (Make sure sin.sin_family is set to AF_INET.) */
			wapi_set_ip(sock, ifname, &addr);
	}

	/* get netmask */
	bzero(&addr, sizeof(struct in_addr));
	if (wapi_get_netmask(sock, ifname, &addr) >= 0)
	{
		printf("netmask: %s\n", inet_ntoa(addr));

		if (set_ok)
			/* set netmask (Make sure sin.sin_family is set to AF_INET.) */
			wapi_set_netmask(sock, ifname, &addr);
	}

	/* get freq */
	if (wapi_get_freq(sock, ifname, &freq, &freq_flag) >= 0)
	{
		int chan;
		double tmpfreq;

		printf("freq: %g\n", freq);
		printf("freq_flag: %s\n", wapi_freq_flags[freq_flag]);

		if (wapi_freq2chan(sock, ifname, freq, &chan) >= 0)
			printf("chan: %d\n", chan);

		if (wapi_chan2freq(sock, ifname, chan, &tmpfreq) >= 0)
			printf("freq: %g\n", tmpfreq);

		if (set_ok)
			/* set freq */
			wapi_set_freq(sock, ifname, freq, freq_flag);
	}

	/* get essid */
	if (wapi_get_essid(sock, ifname, essid, &essid_flag) >= 0)
	{
		printf("essid: %s\n", essid);
		printf("essid_flag: %s\n", wapi_essid_flags[essid_flag]);

		if (set_ok)
			/* set essid */
			wapi_set_essid(sock, ifname, essid, essid_flag);
	}

	/* get operating mode */
	if (wapi_get_mode(sock, ifname, &mode) >= 0)
	{
		printf("mode: %s\n", wapi_modes[mode]);

		if (set_ok)
			/* set operating mode */
			wapi_set_mode(sock, ifname, mode);
	}

	/* get ap */
	if (wapi_get_ap(sock, ifname, &ap) >= 0)
	{
		printf(
			"ap: %02X:%02X:%02X:%02X:%02X:%02X\n",
			ap.ether_addr_octet[0],
			ap.ether_addr_octet[1],
			ap.ether_addr_octet[2],
			ap.ether_addr_octet[3],
			ap.ether_addr_octet[4],
			ap.ether_addr_octet[5]);

		if (set_ok)
			/* set ap */
			wapi_set_ap(sock, ifname, &ap);
	}

	/* get bitrate */
	if (wapi_get_bitrate(sock, ifname, &bitrate, &bitrate_flag) >= 0)
	{
		printf("bitrate: %d\n", bitrate);
		printf("bitrate_flag: %s\n", wapi_bitrate_flags[bitrate_flag]);

		if (set_ok)
			/* set bitrate */
			wapi_set_bitrate(sock, ifname, bitrate, bitrate_flag);
	}

	/* get txpower */
	if (wapi_get_txpower(sock, ifname, &txpower, &txpower_flag) >= 0)
	{
		printf("txpower: %d\n", txpower);
		printf("txpower_flag: %s\n", wapi_txpower_flags[txpower_flag]);

		if (set_ok)
			/* set txpower */
			wapi_set_txpower(sock, ifname, txpower, txpower_flag);
	}
}
