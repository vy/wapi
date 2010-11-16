//#include <iwlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <stdlib.h>

/* kernel includes in userspace */
#ifndef __user
#define __user
#endif


#include "iwlib.h"
#include "wapi.h"


/*-- Utility Routines --------------------------------------------------------*/


#define WAPI_IOCTL_STRERROR(cmd)				\
	fprintf(									\
		stderr, "%s:%d:%s():ioctl(%s): %s\n",	\
		__FILE__, __LINE__, __func__,			\
		#cmd, strerror(errno))


#define WAPI_STRERROR(fmt, ...)					\
	fprintf(									\
		stderr, "%s:%d:%s():" fmt ": %s\n",		\
		__FILE__, __LINE__, __func__,			\
		## __VA_ARGS__, strerror(errno))


#define WAPI_ERROR(fmt, ...)							\
	fprintf(											\
		stderr, "%s:%d:%s(): " fmt ,					\
		__FILE__, __LINE__, __func__, ## __VA_ARGS__)


int
wapi_make_socket(void)
{
	return socket(AF_INET, SOCK_DGRAM, 0);
}


/**
 * Shortcut to make WE @a ioctl() calls.
 */
static inline int
wapi_ioctl(int sock, const char *ifname, int cmd, struct iwreq *wrq)
{
	strncpy(wrq->ifr_name, ifname, IFNAMSIZ);
	return ioctl(sock, cmd, wrq);
}


int
wapi_get_we_version(int sock, const char *ifname, int *we_version)
{
	struct iwreq wrq;
	char buf[sizeof(struct iw_range) * 2];
	int ret;

	/* Prepare request. */
	bzero(buf, sizeof(buf));
	wrq.u.data.pointer = buf;
	wrq.u.data.length = sizeof(buf);
	wrq.u.data.flags = 0;

	/* Get WE version. */
	if ((ret = wapi_ioctl(sock, ifname, SIOCGIWRANGE, &wrq)) >= 0)
	{
		struct iw_range *range = (struct iw_range *) buf;
		*we_version = (int) range->we_version_compiled;
	}
	else WAPI_IOCTL_STRERROR(SIOCGIWRANGE);

	return ret;
}


int
wapi_get_ifnames(wapi_list_t *list)
{
	FILE *fp;
	int ret;
	char tmp[1024]; /* Can a line be longer than 1024 bytes? */

	/* Validate "list" first. */
	if (!list) return -1;

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

		/* Do a single allocation for both wapi_string_t and char vector. */
		string = malloc(sizeof(wapi_string_t) + (end - beg + sizeof(char)));
		if (!string)
		{
			WAPI_STRERROR("malloc()");
			ret = -1;
			break;
		}
		string->data = (char *) (string + sizeof(wapi_string_t));

		/* Copy region into the buffer. */
		memcpy(string->data, beg, (end - beg));
		*(string->data + (end - beg)) = '\0';

		/* Push string into the list. */
		string->next = list->head.string;
		list->head.string = string;
	}

	fclose(fp);
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

	if ((ret = wapi_ioctl(sock, ifname, SIOCGIWFREQ, &wrq)) >= 0)
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

	ret = wapi_ioctl(sock, ifname, SIOCSIWFREQ, &wrq);
	if (ret < 0) WAPI_IOCTL_STRERROR(SIOCSIWFREQ);

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

	wrq.u.essid.pointer = essid;
	wrq.u.essid.length = WAPI_ESSID_MAX_SIZE + 1;
	wrq.u.essid.flags = 0;

	ret = wapi_ioctl(sock, ifname, SIOCGIWESSID, &wrq);
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

	ret = wapi_ioctl(sock, ifname, SIOCSIWESSID, &wrq);
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

	if ((ret = wapi_ioctl(sock, ifname, SIOCGIWMODE, &wrq)) >= 0)
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

	ret = wapi_ioctl(sock, ifname, SIOCSIWMODE, &wrq);
	if (ret < 0) WAPI_IOCTL_STRERROR(SIOCSIWMODE);

	return ret;
}


/*-- Access Point ------------------------------------------------------------*/


void
wapi_make_broad_ether(struct sockaddr *sa)
{
	sa->sa_family = ARPHRD_ETHER;
	memset(sa->sa_data, 0xFF, ETH_ALEN);
}


void
wapi_make_null_ether(struct sockaddr *sa)
{
	sa->sa_family = ARPHRD_ETHER;
	memset(sa->sa_data, 0x00, ETH_ALEN);
}


int
wapi_get_ap(int sock, const char *ifname, struct sockaddr *ap)
{
	struct iwreq wrq;
	int ret;

	if ((ret = wapi_ioctl(sock, ifname, SIOCGIWAP, &wrq)) >= 0)
		memcpy(ap, &(wrq.u.ap_addr), sizeof(struct sockaddr));
	else WAPI_IOCTL_STRERROR(SIOCGIWAP);

	return ret;
}


int
wapi_set_ap(int sock, const char *ifname, const struct sockaddr *ap)
{
	struct iwreq wrq;
	int ret;

	/* Socket address must be of ARPHRD_ETHER family. */
	if (ap->sa_family != ARPHRD_ETHER)
		return -1;

	memcpy(&wrq.u.ap_addr, ap, sizeof(struct sockaddr));
	ret = wapi_ioctl(sock, ifname, SIOCSIWAP, &wrq);
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

	if ((ret = wapi_ioctl(sock, ifname, SIOCGIWRATE, &wrq)) >= 0)
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

	ret = wapi_ioctl(sock, ifname, SIOCSIWRATE, &wrq);
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

	if ((ret = wapi_ioctl(sock, ifname, SIOCGIWTXPOW, &wrq)) >= 0)
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
	ret = wapi_ioctl(sock, ifname, SIOCSIWTXPOW, &wrq);
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


/*-- Scanning Routines -------------------------------------------------------*/


int
wapi_scan_init(int sock, const char *ifname)
{
	struct iwreq wrq;
	int ret;

	wrq.u.data.pointer = NULL;
	wrq.u.data.flags = 0;
	wrq.u.data.length = 0;

	ret = wapi_ioctl(sock, ifname, SIOCSIWSCAN, &wrq);
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

	if ((ret = wapi_ioctl(sock, ifname, SIOCGIWSCAN, &wrq)) < 0)
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

	/* Validate "list" first.*/
	if (!list)
		return -1;

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

		/* Save cell identifier */
		memcpy(&(temp->ap), &(event->u.ap_addr), sizeof(struct sockaddr));

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
	if ((ret = wapi_ioctl(sock, ifname, SIOCGIWSCAN, &wrq)) < 0 && errno == E2BIG)
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
