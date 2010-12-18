/**
 * @file
 * Wireless network interface accessors.
 */


#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <linux/nl80211.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#include <iwlib.h>

#include "wapi.h"
#include "util.h"


/*-- Misc --------------------------------------------------------------------*/


int
wapi_get_we_version(int sock, const char *ifname, int *we_version)
{
	struct iwreq wrq;
	char buf[sizeof(struct iw_range) * 2];
	int ret;

	WAPI_VALIDATE_PTR(we_version);

	/* Prepare request. */
	bzero(buf, sizeof(buf));
	wrq.u.data.pointer = buf;
	wrq.u.data.length = sizeof(buf);
	wrq.u.data.flags = 0;

	/* Get WE version. */
	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	if ((ret = ioctl(sock, SIOCGIWRANGE, &wrq)) >= 0)
	{
		struct iw_range *range = (struct iw_range *) buf;
		*we_version = (int) range->we_version_compiled;
	}
	else WAPI_IOCTL_STRERROR(SIOCGIWRANGE);

	return ret;
}


/*-- Frequency ---------------------------------------------------------------*/


const char *wapi_freq_flags[] = {
	"WAPI_FREQ_AUTO",
	"WAPI_FREQ_FIXED"
};


/**
 * Converts internal representation of frequencies to a floating point.
 */
static inline double
wapi_freq2float(const struct iw_freq *freq)
{
	return ((double) freq->m) * pow(10, freq->e);
}


/**
 * Converts a floating point the our internal representation of frequencies.
 */
static inline void
wapi_float2freq(double floatfreq, struct iw_freq *freq)
{
	freq->e = (short) floor(log10(floatfreq));
	if (freq->e > 8)
	{
		freq->m = ((long) (floor(floatfreq / pow(10,freq->e - 6)))) * 100;
		freq->e -= 8;
	}
	else
	{
		freq->m = (long) floatfreq;
		freq->e = 0;
	}
}


int
wapi_get_freq(int sock, const char *ifname, double *freq, wapi_freq_flag_t *flag)
{
	struct iwreq wrq;
	int ret;

	WAPI_VALIDATE_PTR(freq);
	WAPI_VALIDATE_PTR(flag);

	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	if ((ret = ioctl(sock, SIOCGIWFREQ, &wrq)) >= 0)
	{
		/* Set flag. */
		if (IW_FREQ_AUTO == (wrq.u.freq.flags & IW_FREQ_AUTO))
			*flag = WAPI_FREQ_AUTO;
		else if (IW_FREQ_FIXED == (wrq.u.freq.flags & IW_FREQ_FIXED))
			*flag = WAPI_FREQ_FIXED;
		else
		{
			WAPI_ERROR("Unknown flag: %d.\n", wrq.u.freq.flags);
			return -1;
		}

		/* Set freq. */
		*freq = wapi_freq2float(&(wrq.u.freq));
	}

	return ret;
}


int
wapi_set_freq(int sock, const char *ifname, double freq, wapi_freq_flag_t flag)
{
	struct iwreq wrq;
	int ret;

	/* Set freq. */
	wapi_float2freq(freq, &(wrq.u.freq));

	/* Set flag. */
	switch (flag)
	{
	case WAPI_FREQ_AUTO:
		wrq.u.freq.flags = IW_FREQ_AUTO;
		break;
	case WAPI_FREQ_FIXED:
		wrq.u.freq.flags = IW_FREQ_FIXED;
		break;
	}

	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	ret = ioctl(sock, SIOCSIWFREQ, &wrq);
	if (ret < 0) WAPI_IOCTL_STRERROR(SIOCSIWFREQ);

	return ret;
}


int
wapi_freq2chan(int sock, const char *ifname, double freq, int *chan)
{
	struct iwreq wrq;
	char buf[sizeof(struct iw_range) * 2];
	int ret;

	WAPI_VALIDATE_PTR(chan);

	/* Prepare request. */
	bzero(buf, sizeof(buf));
	wrq.u.data.pointer = buf;
	wrq.u.data.length = sizeof(buf);
	wrq.u.data.flags = 0;

	/* Get range. */
	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	if ((ret = ioctl(sock, SIOCGIWRANGE, &wrq)) >= 0)
	{
		struct iw_range *range = (struct iw_range *) buf;
		int k;

		/* Compare the frequencies as double to ignore differences in encoding.
		 * Slower, but safer... */
		for (k = 0; k < range->num_frequency; k++)
			if (freq == wapi_freq2float(&(range->freq[k])))
			{
				*chan = range->freq[k].i;
				return 0;
			}

		/* Oops! Nothing found. */
		WAPI_ERROR("No channel matches for the given frequency!\n");
		ret = -2;
	}
	else WAPI_IOCTL_STRERROR(SIOCGIWRANGE);

	return ret;
}


int
wapi_chan2freq(int sock, const char *ifname, int chan, double *freq)
{
	struct iwreq wrq;
	char buf[sizeof(struct iw_range) * 2];
	int ret;

	WAPI_VALIDATE_PTR(freq);

	/* Prepare request. */
	bzero(buf, sizeof(buf));
	wrq.u.data.pointer = buf;
	wrq.u.data.length = sizeof(buf);
	wrq.u.data.flags = 0;

	/* Get range. */
	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	if ((ret = ioctl(sock, SIOCGIWRANGE, &wrq)) >= 0)
	{
		struct iw_range *range = (struct iw_range *) buf;
		int k;

		for (k = 0; k < range->num_frequency; k++)
			if (chan == range->freq[k].i)
			{
				*freq = wapi_freq2float(&(range->freq[k]));
				return 0;
			}

		/* Oops! Nothing found. */
		WAPI_ERROR("No frequency matches for the given channel!\n");
		ret = -2;
	}
	else WAPI_IOCTL_STRERROR(SIOCGIWRANGE);

	return ret;
}


/*-- ESSID -------------------------------------------------------------------*/


const char *wapi_essid_flags[] = {
	"WAPI_ESSID_ON",
	"WAPI_ESSID_OFF"
};


int
wapi_get_essid(
	int sock,
	const char *ifname,
	char *essid,
	wapi_essid_flag_t *flag)
{
	struct iwreq wrq;
	int ret;

	WAPI_VALIDATE_PTR(essid);
	WAPI_VALIDATE_PTR(flag);

	wrq.u.essid.pointer = essid;
	wrq.u.essid.length = WAPI_ESSID_MAX_SIZE + 1;
	wrq.u.essid.flags = 0;

	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	ret = ioctl(sock, SIOCGIWESSID, &wrq);
	if (ret < 0) WAPI_IOCTL_STRERROR(SIOCGIWESSID);
	else *flag = (wrq.u.essid.flags) ? WAPI_ESSID_ON : WAPI_ESSID_OFF;

	return ret;
}


int
wapi_set_essid(
	int sock,
	const char *ifname,
	const char *essid,
	wapi_essid_flag_t flag)
{
	char buf[WAPI_ESSID_MAX_SIZE + 1];
	struct iwreq wrq;
	int ret;

	/* Prepare request. */
	wrq.u.essid.pointer = buf;
	wrq.u.essid.length =
		snprintf(buf, ((WAPI_ESSID_MAX_SIZE + 1) * sizeof(char)), "%s", essid);
	wrq.u.essid.flags = (flag == WAPI_ESSID_ON);

	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	ret = ioctl(sock, SIOCSIWESSID, &wrq);
	if (ret < 0) WAPI_IOCTL_STRERROR(SIOCSIWESSID);

	return ret;
}


/*-- Operating Mode ----------------------------------------------------------*/


const char *wapi_modes[] = {
	"WAPI_MODE_AUTO",
	"WAPI_MODE_ADHOC",
	"WAPI_MODE_MANAGED",
	"WAPI_MODE_MASTER",
	"WAPI_MODE_REPEAT",
	"WAPI_MODE_SECOND",
	"WAPI_MODE_MONITOR"
};


static int
wapi_parse_mode(int iw_mode, wapi_mode_t *wapi_mode)
{
	switch (iw_mode)
	{
	case WAPI_MODE_AUTO:
	case WAPI_MODE_ADHOC:
	case WAPI_MODE_MANAGED:
	case WAPI_MODE_MASTER:
	case WAPI_MODE_REPEAT:
	case WAPI_MODE_SECOND:
	case WAPI_MODE_MONITOR:
		*wapi_mode = iw_mode;
		return 0;

	default:
		WAPI_ERROR("Unknown mode: %d.\n", iw_mode);
		return -1;
	}
}


int
wapi_get_mode(int sock, const char *ifname, wapi_mode_t *mode)
{
	struct iwreq wrq;
	int ret;

	WAPI_VALIDATE_PTR(mode);

	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	if ((ret = ioctl(sock, SIOCGIWMODE, &wrq)) >= 0)
		ret = wapi_parse_mode(wrq.u.mode, mode);
	else WAPI_IOCTL_STRERROR(SIOCGIWMODE);

	return ret;
}


int
wapi_set_mode(int sock, const char *ifname, wapi_mode_t mode)
{
	struct iwreq wrq;
	int ret;

	wrq.u.mode = mode;

	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	ret = ioctl(sock, SIOCSIWMODE, &wrq);
	if (ret < 0) WAPI_IOCTL_STRERROR(SIOCSIWMODE);

	return ret;
}


/*-- Access Point ------------------------------------------------------------*/


static int
wapi_make_ether(struct ether_addr *addr, int byte)
{
	WAPI_VALIDATE_PTR(addr);
	memset(addr, byte, sizeof(struct ether_addr));
	return 0;
}


int
wapi_make_broad_ether(struct ether_addr *addr)
{
	return wapi_make_ether(addr, 0xFF);
}


int
wapi_make_null_ether(struct ether_addr *sa)
{
	return wapi_make_ether(sa, 0x00);
}


int
wapi_get_ap(int sock, const char *ifname, struct ether_addr *ap)
{
	struct iwreq wrq;
	int ret;

	WAPI_VALIDATE_PTR(ap);

	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	if ((ret = ioctl(sock, SIOCGIWAP, &wrq)) >= 0)
		memcpy(ap, wrq.u.ap_addr.sa_data, sizeof(struct ether_addr));
	else WAPI_IOCTL_STRERROR(SIOCGIWAP);

	return ret;
}


int
wapi_set_ap(int sock, const char *ifname, const struct ether_addr *ap)
{
	struct iwreq wrq;
	int ret;

	WAPI_VALIDATE_PTR(ap);

	wrq.u.ap_addr.sa_family = ARPHRD_ETHER;
	memcpy(wrq.u.ap_addr.sa_data, ap, sizeof(struct ether_addr));
	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	ret = ioctl(sock, SIOCSIWAP, &wrq);
	if (ret < 0) WAPI_IOCTL_STRERROR(SIOCSIWAP);

	return ret;
}


/*-- Bit Rate ----------------------------------------------------------------*/


const char *wapi_bitrate_flags[] = {
	"WAPI_BITRATE_AUTO",
	"WAPI_BITRATE_FIXED"
};


int
wapi_get_bitrate(
	int sock,
	const char *ifname,
	int *bitrate,
	wapi_bitrate_flag_t *flag)
{
	struct iwreq wrq;
	int ret;

	WAPI_VALIDATE_PTR(bitrate);
	WAPI_VALIDATE_PTR(flag);

	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	if ((ret = ioctl(sock, SIOCGIWRATE, &wrq)) >= 0)
	{
		/* Check if enabled. */
		if (wrq.u.bitrate.disabled)
		{
			WAPI_ERROR("Bitrate is disabled.\n");
			return -1;
		}

		/* Get bitrate. */
		*bitrate = wrq.u.bitrate.value;
		*flag = wrq.u.bitrate.fixed ? WAPI_BITRATE_FIXED : WAPI_BITRATE_AUTO;
	}
	else WAPI_IOCTL_STRERROR(SIOCGIWRATE);

	return ret;
}


int
wapi_set_bitrate(
	int sock,
	const char *ifname,
	int bitrate,
	wapi_bitrate_flag_t flag)
{
	struct iwreq wrq;
	int ret;

	wrq.u.bitrate.value = bitrate;
	wrq.u.bitrate.fixed = (flag == WAPI_BITRATE_FIXED);

	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	ret = ioctl(sock, SIOCSIWRATE, &wrq);
	if (ret < 0) WAPI_IOCTL_STRERROR(SIOCSIWRATE);

	return ret;
}


/*-- Transmit Power ----------------------------------------------------------*/


const char *wapi_txpower_flags[] = {
	"WAPI_TXPOWER_DBM",
	"WAPI_TXPOWER_MWATT",
	"WAPI_TXPOWER_RELATIVE"
};


int
wapi_dbm2mwatt(int dbm)
{
	return floor(pow(10, (((double) dbm) / 10)));
}


int
wapi_mwatt2dbm(int mwatt)
{
	return ceil(10 * log10(mwatt));
}


int
wapi_get_txpower(
	int sock,
	const char *ifname,
	int *power,
	wapi_txpower_flag_t *flag)
{
	struct iwreq wrq;
	int ret;

	WAPI_VALIDATE_PTR(power);
	WAPI_VALIDATE_PTR(flag);

	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	if ((ret = ioctl(sock, SIOCGIWTXPOW, &wrq)) >= 0)
	{
		/* Check if enabled. */
		if (wrq.u.txpower.disabled)
			return -1;

		/* Get flag. */
		if (IW_TXPOW_DBM == (wrq.u.txpower.flags & IW_TXPOW_DBM))
			*flag = WAPI_TXPOWER_DBM;
		else if (IW_TXPOW_MWATT == (wrq.u.txpower.flags & IW_TXPOW_MWATT))
			*flag = WAPI_TXPOWER_MWATT;
		else if (IW_TXPOW_RELATIVE == (wrq.u.txpower.flags & IW_TXPOW_RELATIVE))
			*flag = WAPI_TXPOWER_RELATIVE;
		else
		{
			WAPI_ERROR("Unknown flag: %d.\n", wrq.u.txpower.flags);
			return -1;
		}

		/* Get power. */
		*power = wrq.u.txpower.value;
	}
	else WAPI_IOCTL_STRERROR(SIOCGIWTXPOW);

	return ret;
}


int
wapi_set_txpower(
	int sock,
	const char *ifname,
	int power,
	wapi_txpower_flag_t flag)
{
	struct iwreq wrq;
	int ret;

	/* Construct the request. */
	wrq.u.txpower.value = power;
	switch (flag)
	{
	case WAPI_TXPOWER_DBM:
		wrq.u.txpower.flags = IW_TXPOW_DBM;
		break;
	case WAPI_TXPOWER_MWATT:
		wrq.u.txpower.flags = IW_TXPOW_MWATT;
		break;
	case WAPI_TXPOWER_RELATIVE:
		wrq.u.txpower.flags = IW_TXPOW_RELATIVE;
		break;
	}

	/* Issue the set command. */
	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	ret = ioctl(sock, SIOCSIWTXPOW, &wrq);
	if (ret < 0) WAPI_IOCTL_STRERROR(SIOCSIWTXPOW);

	return ret;
}


/*-- Event & Stream Routines -------------------------------------------------*/


struct iw_event_stream {
	char *end;		/* end of the stream */
	char *current;	/* current event in stream of events */
	char *value;		/* current value in event */
};


static void
iw_event_stream_init(struct iw_event_stream *stream, char *data, size_t len)
{
	memset(stream, 0, sizeof(struct iw_event_stream));
	stream->current = data;
	stream->end = &data[len];
}


static int
iw_event_stream_pop(
	struct iw_event_stream *stream,
	struct iw_event *iwe,
	int we_version)
{
	return iw_extract_event_stream(
		(struct stream_descr *) stream, iwe, we_version);
}


/*-- Scanning ----------------------------------------------------------------*/


int
wapi_scan_init(int sock, const char *ifname)
{
	struct iwreq wrq;
	int ret;

	wrq.u.data.pointer = NULL;
	wrq.u.data.flags = 0;
	wrq.u.data.length = 0;

	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	ret = ioctl(sock, SIOCSIWSCAN, &wrq);
	if (ret < 0) WAPI_IOCTL_STRERROR(SIOCSIWSCAN);

	return ret;
}


int
wapi_scan_stat(int sock, const char *ifname)
{
	struct iwreq wrq;
	int ret;
	char buf;

	wrq.u.data.pointer = &buf;
	wrq.u.data.flags = 0;
	wrq.u.data.length = 0;

	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	if ((ret = ioctl(sock, SIOCGIWSCAN, &wrq)) < 0)
	{
		if (errno == E2BIG)
			/* Data is ready, but not enough space, which is expected. */
			return 0;
		else if (errno == EAGAIN)
			/* Data is not ready. */
			return 1;

		printf("err[%d]: %s\n", errno, strerror(errno));
	}
	else WAPI_IOCTL_STRERROR(SIOCGIWSCAN);

	return ret;
}


static int
wapi_scan_event(struct iw_event *event, wapi_list_t *list)
{
	wapi_scan_info_t *info;

	/* Get current "wapi_info_t". */
	info = list->head.scan;

	/* Decode the event. */
	switch (event->cmd)
	{
	case SIOCGIWAP:
	{
		wapi_scan_info_t *temp;

		/* Allocate a new cell. */
		temp = malloc(sizeof(wapi_scan_info_t));
		if (!temp)
		{
			WAPI_STRERROR("malloc()");
			return -1;
		}

		/* Reset it. */
		bzero(temp, sizeof(wapi_scan_info_t));

		/* Save cell identifier. */
		memcpy(&temp->ap, &event->u.ap_addr.sa_data, sizeof(struct ether_addr));

		/* Push it to the head of the list. */
		temp->next = info;
		list->head.scan = temp;
		
		break;
	}

	case SIOCGIWFREQ:
		info->has_freq = 1;
		info->freq = wapi_freq2float(&(event->u.freq));
		break;

	case SIOCGIWMODE:
	{
		int ret = wapi_parse_mode(event->u.mode, &info->mode);
		if (ret >= 0)
		{
			info->has_mode = 1;
			break;
		}
		else return ret;
	}

	case SIOCGIWESSID:
		info->has_essid = 1;
		info->essid_flag = (event->u.data.flags) ? WAPI_ESSID_ON : WAPI_ESSID_OFF;
		memset(info->essid, 0, (WAPI_ESSID_MAX_SIZE + 1));
		if ((event->u.essid.pointer) && (event->u.essid.length))
			memcpy(info->essid, event->u.essid.pointer, event->u.essid.length);
		break;

	case SIOCGIWRATE:
		/* Scan may return a list of bitrates. As we have space for only a
		 * single bitrate, we only keep the largest one. */
		if (!info->has_bitrate || event->u.bitrate.value > info->bitrate)
		{
			info->has_bitrate = 1;
			info->bitrate = event->u.bitrate.value;
		}
		break;
	}

	return 0;
}


int
wapi_scan_coll(int sock, const char *ifname, wapi_list_t *aps)
{
	char *buf;
	int buflen;
	struct iwreq wrq;
	int we_version;
	int ret;

	WAPI_VALIDATE_PTR(aps);

	/* Get WE version. (Required for event extraction via libiw.) */
	if ((ret = wapi_get_we_version(sock, ifname, &we_version)) < 0)
		return ret;

	buflen = IW_SCAN_MAX_DATA;
	buf = malloc(buflen * sizeof(char));
	if (!buf)
	{
		WAPI_STRERROR("malloc()");
		return -1;
	}

alloc:
	/* Collect results. */
	wrq.u.data.pointer = buf;
	wrq.u.data.length = buflen;
	wrq.u.data.flags = 0;
	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	if ((ret = ioctl(sock, SIOCGIWSCAN, &wrq)) < 0 && errno == E2BIG)
	{
		char *tmp;

		buflen *= 2;
		tmp = realloc(buf, buflen);
		if (!tmp)
		{
			WAPI_STRERROR("realloc()");
			free(buf);
			return -1;
		}

		buf = tmp;
		goto alloc;
	}

	/* There is still something wrong. It's either EAGAIN or some other ioctl()
	 * failure. We don't bother, let the user deal with it. */
	if (ret < 0)
	{
		WAPI_IOCTL_STRERROR(SIOCGIWSCAN);
		free(buf);
		return ret;
	}

	/* We have the results, process them. */
	if (wrq.u.data.length)
	{
		struct iw_event iwe;
		struct iw_event_stream stream;

		iw_event_stream_init(&stream, buf, wrq.u.data.length);
		do {
			if ((ret = iw_event_stream_pop(&stream, &iwe, we_version)) >= 0)
			{
				int eventret = wapi_scan_event(&iwe, aps);
				if (eventret < 0)
					ret = eventret;
			}
			else WAPI_ERROR("iw_event_stream_pop() failed!\n");
		} while (ret > 0);
	}

	/* Free request buffer. */
	free(buf);

	return ret;
}


/*-- Add/Del Interface -------------------------------------------------------*/


typedef enum {
	WAPI_NL80211_CMD_IFADD,
	WAPI_NL80211_CMD_IFDEL
} wapi_nl80211_cmd_t;


typedef struct wapi_nl80211_ifadd_ctx_t {
	const char *name;
	wapi_mode_t mode;
} wapi_nl80211_ifadd_ctx_t;


typedef struct wapi_nl80211_ifdel_ctx_t {
} wapi_nl80211_ifdel_ctx_t;


typedef struct wapi_nl80211_ctx_t {
	const char *ifname;
	wapi_nl80211_cmd_t cmd;
	union {
		wapi_nl80211_ifadd_ctx_t ifadd;
		wapi_nl80211_ifdel_ctx_t ifdel;
	} u;
} wapi_nl80211_ctx_t;


#ifdef LIBNL1
#define nl_sock nl_handle


static struct nl_handle *nl_socket_alloc(void)
{
	return nl_handle_alloc();
}


static void nl_socket_free(struct nl_sock *h)
{
	nl_handle_destroy(h);
}
#endif


static int
wapi_mode_to_iftype(wapi_mode_t mode, enum nl80211_iftype *type)
{
	int ret = 0;

	switch (mode)
	{
	case WAPI_MODE_AUTO:	*type = NL80211_IFTYPE_UNSPECIFIED;	break;
	case WAPI_MODE_ADHOC:	*type = NL80211_IFTYPE_ADHOC;		break;
	case WAPI_MODE_MANAGED:	*type = NL80211_IFTYPE_STATION;		break;
	case WAPI_MODE_MASTER:	*type = NL80211_IFTYPE_AP;			break;
	case WAPI_MODE_MONITOR:	*type = NL80211_IFTYPE_MONITOR;		break;
	default:
		WAPI_ERROR(
			"No supported nl80211 iftype for mode: %s!\n",
			wapi_modes[mode]);
		ret = -1;
	}

	return ret;
}


static int
nl80211_err_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
	int *ret = arg;
	*ret = err->error;
	return NL_STOP;
}


static int
nl80211_fin_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_SKIP;
}


static int
nl80211_ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}


static int
nl80211_cmd_handler(const wapi_nl80211_ctx_t *ctx)
{
	struct nl_sock *sock;
	struct nl_msg *msg;
	struct nl_cb *cb;
	int family;
	int ifidx;
	int ret;

	/* Allocate netlink socket. */
	sock = nl_socket_alloc();
	if (!sock)
	{
		WAPI_ERROR("Failed to allocate netlink socket!\n");
		return -ENOMEM;
	}

	/* Reset "msg" and "cb". */
	msg = NULL;
	cb = NULL;

	/* Connect to generic netlink socket on kernel side. */
	if (genl_connect(sock))
	{
		WAPI_ERROR("Failed to connect to generic netlink!\n");
		ret = -ENOLINK;
		goto exit;
	}

	/* Ask kernel to resolve family name to family id. */
	ret = family = genl_ctrl_resolve(sock, "nl80211");
	if (ret < 0)
	{
		WAPI_ERROR("genl_ctrl_resolve() failed!\n");
		goto exit;
	}

	/* Map given network interface name (ifname) to its corresponding index. */
	ifidx = if_nametoindex(ctx->ifname);
	if (!ifidx)
	{
		WAPI_STRERROR("if_nametoindex(\"%s\")", ctx->ifname);
		ret = -errno;
		goto exit;
	}

	/* Construct a generic netlink by allocating a new message. */
	msg = nlmsg_alloc();
	if (!msg)
	{
		WAPI_ERROR("nlmsg_alloc() failed!\n");
		ret = -ENOMEM;
		goto exit;
	}

	/* Append the requested command to the message. */
	switch (ctx->cmd)
	{
	case WAPI_NL80211_CMD_IFADD:
	{
		enum nl80211_iftype iftype;

		genlmsg_put(
			msg, NL_AUTO_PID, NL_AUTO_SEQ, family, 0, 0,
			NL80211_CMD_NEW_INTERFACE, 0);
		NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, ifidx);

		/* Get NL80211_IFTYPE_* for the given WAPI mode. */
		ret = wapi_mode_to_iftype(ctx->u.ifadd.mode, &iftype);
		if (ret < 0) goto exit;

		NLA_PUT_STRING(msg, NL80211_ATTR_IFNAME, ctx->u.ifadd.name);
		NLA_PUT_U32(msg, NL80211_ATTR_IFTYPE, iftype);

		break;
	}

	case WAPI_NL80211_CMD_IFDEL:
		genlmsg_put(
			msg, NL_AUTO_PID, NL_AUTO_SEQ, family, 0, 0,
			NL80211_CMD_DEL_INTERFACE, 0);
		NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, ifidx);
		break;
	}

	/* Finalize (send) the message. */
	ret = nl_send_auto_complete(sock, msg);
	if (ret < 0)
	{
		WAPI_ERROR("nl_send_auto_complete() failed!\n");
		goto exit;
	}

	/* Allocate a new callback handle. */
	cb = nl_cb_alloc(NL_CB_VERBOSE);
	if (!cb)
	{
		WAPI_ERROR("nl_cb_alloc() failed\n");
		ret = -1;
		goto exit;
	}

	/* Configure callback handlers. */
	nl_cb_err(cb, NL_CB_CUSTOM, nl80211_err_handler, &ret);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, nl80211_fin_handler, &ret);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, nl80211_ack_handler, &ret);

	/* Consume netlink replies. */
	for (ret = 1; ret > 0; ) nl_recvmsgs(sock, cb);
	if (ret) WAPI_ERROR("nl_recvmsgs() failed!\n");

exit:
	/* Release resources and exit with "ret". */
	nl_socket_free(sock);
	if (msg) nlmsg_free(msg);
	if (cb) free(cb);
	return ret;

nla_put_failure:
	WAPI_ERROR("nla_put_failure!\n");
	ret = -1;
	goto exit;
}


int
wapi_if_add(int sock, const char *ifname, const char *name, wapi_mode_t mode)
{
	wapi_nl80211_ctx_t ctx;
	ctx.ifname = ifname;
	ctx.cmd = WAPI_NL80211_CMD_IFADD;
	ctx.u.ifadd.name = name;
	ctx.u.ifadd.mode = mode;
	return nl80211_cmd_handler(&ctx);
}


int
wapi_if_del(int sock, const char *ifname)
{
	wapi_nl80211_ctx_t ctx;
	ctx.ifname = ifname;
	ctx.cmd = WAPI_NL80211_CMD_IFDEL;
	return nl80211_cmd_handler(&ctx);
}
