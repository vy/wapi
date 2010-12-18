/**
 * @file
 * Public API declarations.
 */


#ifndef WAPI_H
#define WAPI_H


#include <netinet/in.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <linux/wireless.h>


/* Generic linked list (dummy) decleration. (No definition!) */
typedef struct wapi_list_t wapi_list_t;


/**
 * @defgroup accessors Network Interface Accessors
 *
 * This section is composed of accessors (getters and setters) for various
 * network interface configurations. Each accessor requires a socket (see
 * wapi_make_socket()) to issue kernel commands. Since each accessor is a
 * user-friendly wrapper over various ioctl() calls, unless stated otherwise,
 * return values of the accessors are generally obtained by related ioctl()
 * calls. In other words, on success, zero is returned.
 *
 * Pay attention that, all setters require root privileges.
 *
 * Below is an example usage of some accessor routines.
 *
 * @include conf.c
 *
 * A sample output of the above function is as follows.
 *
@verbatim
ip: 192.168.1.111
netmask: 255.255.255.0
freq: 2.412e+09
freq_flag: WAPI_FREQ_AUTO
chan: 1
freq: 2.412e+09
essid: ozu
essid_flag: WAPI_ESSID_ON
mode: WAPI_MODE_MANAGED
ap: 00:14:C1:34:CE:83
wireless.c:451:wapi_get_bitrate(): Bitrate is disabled.
wireless.c:546:wapi_get_txpower():ioctl(SIOCGIWTXPOW): Invalid argument
@endverbatim
 *
 * As can be seen from the output, some functionalities are not supported by the
 * underlying wireless network card of the test system.
 */


/**
 * @defgroup ifaccessors Generic Network Interface Accessors
 * @ingroup accessors
 *
 * This section consists of generic network interface accessors (IP address,
 * gateway, netmask) without any device specific features.
 */


/**
 * @defgroup misc Miscellaneous Accessors
 * @ingroup ifaccessors
 * @{
 */


/**
 * Gets the interface up status.
 *
 * @param[out] is_up Set to 0, if up; 1, otherwise.
 */
int wapi_get_ifup(int sock, const char *ifname, int *is_up);


/**
 * Activates the interface.
 */
int wapi_set_ifup(int sock, const char *ifname);


/**
 * Shuts down the interface.
 */
int wapi_set_ifdown(int sock, const char *ifname);


/** @} misc/ifaccessors */


/**
 * @defgroup ip IP (Internet Protocol) Accessors
 * @ingroup ifaccessors
 * @{
 */


/**
 * Gets IP address of the given network interface.
 */
int wapi_get_ip(int sock, const char *ifname, struct in_addr *addr);


/**
 * Sets IP adress of the given network interface.
 */
int wapi_set_ip(int sock, const char *ifname, const struct in_addr *addr);


/**
 * Gets netmask of the given network interface.
 */
int wapi_get_netmask(int sock, const char *ifname, struct in_addr *addr);


/**
 * Sets netmask of the given network interface.
 */
int wapi_set_netmask(int sock, const char *ifname, const struct in_addr *addr);


/** @} ip/ifaccessors */


/**
 * @defgroup route Routing Table Accessors
 * @ingroup ifaccessors
 * @{
 */


/**
 * Parses routing table rows from @c WAPI_PROC_NET_ROUTE.
 *
 * @param[out] list Pushes collected @c wapi_route_info_t into this list.
 *
 * Below is example usage of wapi_get_routes().
 *
 * @include routes.c
 *
 * Here is a sample output of the above getroutes().
 *
@verbatim
>> dest: 0.0.0.0, gw: 0.0.0.0, netmask: 0.0.0.0
>> dest: 192.168.1.0, gw: 192.168.1.0, netmask: 192.168.1.0
@endverbatim
 */
int wapi_get_routes(wapi_list_t *list);


/** Route target types. */
typedef enum {
	WAPI_ROUTE_TARGET_NET,	/**< The target is a network. */
	WAPI_ROUTE_TARGET_HOST	/**< The target is a host. */
} wapi_route_target_t;


/**
 * Adds @a gateway for the given @a target network.
 */
int
wapi_add_route_gw(
	int sock,
	wapi_route_target_t targettype,
	const struct in_addr *target,
	const struct in_addr *netmask,
	const struct in_addr *gw);


/**
 * Deletes @a gateway for the given @a target network.
 */
int
wapi_del_route_gw(
	int sock,
	wapi_route_target_t targettype,
	const struct in_addr *target,
	const struct in_addr *netmask,
	const struct in_addr *gw);


/** @} route/ifaccessors */


/**
 * @defgroup wifaccessors Wireless Interface Accessors
 * @ingroup accessors
 *
 * This section consists of accessor functions dedicated to wireless network
 * interfaces.
 */


/**
 * @defgroup misc Miscellaneous Accessors
 * @ingroup wifaccessors
 * @{
 */


/**
 * Gets kernel WE (Wireless Extensions) version.
 *
 * @param[out] we_version Set to @c we_version_compiled of range information.
 *
 * @return zero on success.
 */
int wapi_get_we_version(int sock, const char *ifname, int *we_version);


/** @} misc/wifaccessors */


/**
 * @defgroup freq Frequency Accessors
 * @ingroup wifaccessors
 * @{
 */


/** Frequency flags. */
typedef enum {
	WAPI_FREQ_AUTO	= IW_FREQ_AUTO,
	WAPI_FREQ_FIXED	= IW_FREQ_FIXED
} wapi_freq_flag_t;


/** Frequency flag names. */
extern const char *wapi_freq_flags[];


/**
 * Gets the operating frequency of the device.
 */
int wapi_get_freq(
	int sock,
	const char *ifname,
	double *freq,
	wapi_freq_flag_t *flag);


/**
 * Sets the operating frequency of the device.
 */
int
wapi_set_freq(
	int sock,
	const char *ifname,
	double freq,
	wapi_freq_flag_t flag);


/**
 * Finds corresponding channel for the supplied @a freq.
 *
 * @return 0, on success; -2, if not found; otherwise, ioctl() return value.
 */
int wapi_freq2chan(int sock, const char *ifname, double freq, int *chan);


/**
 * Finds corresponding frequency for the supplied @a chan.
 *
 * @return 0, on success; -2, if not found; otherwise, ioctl() return value.
 */
int wapi_chan2freq(int sock, const char *ifname, int chan, double *freq);


/** @} freq/wifaccessors */


/**
 * @defgroup essid ESSID (Extended Service Set Identifier) Accessors
 * @ingroup wifaccessors
 * @{
 */


/** Maximum allowed ESSID size. */
#define WAPI_ESSID_MAX_SIZE IW_ESSID_MAX_SIZE


/** ESSID flags.  */
typedef enum {
	WAPI_ESSID_ON,
	WAPI_ESSID_OFF
} wapi_essid_flag_t;


/** ESSID flag names. */
extern const char *wapi_essid_flags[];


/**
 * Gets ESSID of the device.
 *
 * @param[out] essid Used to store the ESSID of the device. Buffer must have
 *     enough space to store @c WAPI_ESSID_MAX_SIZE+1 characters.
 */
int
wapi_get_essid(
	int sock,
	const char *ifname,
	char *essid,
	wapi_essid_flag_t *flag);


/**
 * Sets ESSID of the device.
 *
 * @a essid At most @c WAPI_ESSID_MAX_SIZE characters are read.
 */
int
wapi_set_essid(
	int sock,
	const char *ifname,
	const char *essid,
	wapi_essid_flag_t flag);


/** @} essid/wifaccessors */


/**
 * @defgroup mode Operating Mode
 * @ingroup wifaccessors
 * @{
 */


/** Supported operation modes. */
typedef enum {
	WAPI_MODE_AUTO		= IW_MODE_AUTO,		/**< Driver decides. */
	WAPI_MODE_ADHOC		= IW_MODE_ADHOC,	/**< Single cell network. */
	WAPI_MODE_MANAGED	= IW_MODE_INFRA,	/**< Multi cell network, roaming, ... */
	WAPI_MODE_MASTER	= IW_MODE_MASTER,	/**< Synchronisation master or access point. */
	WAPI_MODE_REPEAT	= IW_MODE_REPEAT,	/**< Wireless repeater, forwarder. */
	WAPI_MODE_SECOND	= IW_MODE_SECOND,	/**< Secondary master/repeater, backup. */
	WAPI_MODE_MONITOR	= IW_MODE_MONITOR	/**< Passive monitor, listen only. */
} wapi_mode_t;


/** Supported operation mode names. */
extern const char *wapi_modes[];


/**
 * Gets the operating mode of the device.
 */
int wapi_get_mode(int sock, const char *ifname, wapi_mode_t *mode);


/**
 * Sets the operating mode of the device.
 */
int wapi_set_mode(int sock, const char *ifname, wapi_mode_t mode);


/** @} mode/wifaccessors */


/**
 * @defgroup ap Access Point
 * @ingroup wifaccessors
 * @{
 */


/**
 * Creates an ethernet broadcast address.
 */
int wapi_make_broad_ether(struct ether_addr *sa);


/**
 * Creates an ethernet NULL address.
 */
int wapi_make_null_ether(struct ether_addr *sa);


/**
 * Gets access point address of the device.
 *
 * @param[out] ap Set the to MAC address of the device. (For "any", a broadcast
 *     ethernet address; for "off", a null ethernet address is used.)
 */
int wapi_get_ap(int sock, const char *ifname, struct ether_addr *ap);


/**
 * Sets access point address of the device.
 */
int wapi_set_ap(int sock, const char *ifname, const struct ether_addr *ap);


/** @} ap/wifaccessors */


/**
 * @defgroup bitrate Bit Rate
 * @ingroup wifaccessors
 * @{
 */


/**
 * Bitrate flags.
 *
 * At the moment, unicast (@c IW_BITRATE_UNICAST) and broadcast (@c
 * IW_BITRATE_BROADCAST) bitrate flags are not supported.
 */
typedef enum {
	WAPI_BITRATE_AUTO,
	WAPI_BITRATE_FIXED
} wapi_bitrate_flag_t;


/** Bitrate flag names. */
extern const char *wapi_bitrate_flags[];


/**
 * Gets bitrate of the device.
 */
int wapi_get_bitrate(
	int sock,
	const char *ifname,
	int *bitrate,
	wapi_bitrate_flag_t *flag);


/**
 * Sets bitrate of the device.
 */
int
wapi_set_bitrate(
	int sock,
	const char *ifname,
	int bitrate,
	wapi_bitrate_flag_t flag);


/** @} bitrate/wifaccessors */


/**
 * @defgroup txpower Transmit Power
 * @ingroup wifaccessors
 * @{
 */


/** Transmit power (txpower) flags. */
typedef enum {
	WAPI_TXPOWER_DBM,		/**< Value is in dBm. */
	WAPI_TXPOWER_MWATT,		/**< Value is in mW. */
	WAPI_TXPOWER_RELATIVE	/**< Value is in arbitrary units. */
} wapi_txpower_flag_t;


/** Transmit power flag names. */
extern const char *wapi_txpower_flags[];


/**
 * Converts a value in dBm to a value in milliWatt.
 */
int wapi_dbm2mwatt(int dbm);


/**
 * Converts a value in milliWatt to a value in dBm.
 */
int wapi_mwatt2dbm(int mwatt);


/**
 * Gets txpower of the device.
 */
int
wapi_get_txpower(
	int sock,
	const char *ifname,
	int *power,
	wapi_txpower_flag_t *flag);


/**
 * Sets txpower of the device.
 */
int
wapi_set_txpower(
	int sock,
	const char *ifname,
	int power,
	wapi_txpower_flag_t flag);


/** @} txpower/wifaccessors */


/**
 * @defgroup ifadddel Add/Delete Virtual Interfaces
 * @ingroup wifaccessors
 *
 * nl80211 routines to add/delete virtual interfaces. Some of these features
 * require dedicated hardware to work properly. Since these features utilize
 * nl80211 interface, @c sock arguments are there just for the function
 * footprint uniformness purposes, hence are ignored totally.
 *
 * @{
 */


/**
 * Creates a virtual interface with @a name for interface @a ifname.
 */
int
wapi_if_add(int sock, const char *ifname, const char *name, wapi_mode_t mode);


/**
 * Deletes a virtual interface with @a name.
 */
int
wapi_if_del(int sock, const char *ifname);


/** @} ifadddel/wifaccessors */


/**
 * @defgroup utils Utility Routines
 * @{
 */


/**
 * Creates an AF_INET socket to be used in ioctl() calls.
 *
 * @return non-negative on success.
 */
int wapi_make_socket(void);


/**
 * Parses @c WAPI_PROC_NET_WIRELESS according to hardcoded mechanisms in @c
 * linux/net/wireless/wext-proc.c sources.
 *
 * @param[out] list Pushes collected @c wapi_string_t into this list.
 *
 * Here is an example usage of the wapi_get_ifnames().
 *
 * @include ifnames.c
 */
int wapi_get_ifnames(wapi_list_t *list);


/** @} utils */


/**
 * @defgroup scan Scanning
 *
 * This group consists of functions for scanning accessible access points (APs)
 * in the range.
 *
 * Unfortunately provided scanning API by wireless-tools libraries (libiw) is
 * quite limited, and doesn't list all APs in the range. (See iw_process_scan()
 * of libiw. For iwlist case, it has its own hardcoded magic for this stuff and
 * it is not provided by libiw.) For this purpose, we needed to implement our
 * own scanning routines. Furthermore, scanning requires extracting binary
 * results returned from kernel over a @c char buffer, hence it causes dozens of
 * hairy binary compatibility issues. Luckily, libiw provides an API method to
 * cope with this: iw_extract_event_stream(). That's the only place in this
 * project relying on libiw.
 *
 * The scanning operation disable normal network traffic, and therefore you
 * should not abuse of scan. The scan need to check the presence of network on
 * other frequencies. While you are checking those other frequencies, you can
 * not be on your normal frequency to listen to normal traffic in the cell. You
 * need typically in the order of one second to actively probe all 802.11b
 * channels (do the maths). Some cards may do that in background, to reply to
 * scan commands faster, but they still have to do it. Leaving the cell for such
 * an extended period of time is pretty bad. Any kind of streaming/low latency
 * traffic will be impacted, and the user will perceive it (easily checked with
 * telnet). People trying to send traffic to you will retry packets and waste
 * bandwidth. Some applications may be sensitive to those packet losses in weird
 * ways, and tracing those weird behavior back to scanning may take time. If you
 * are in ad-hoc mode, if two nodes scan approx at the same time, they won't see
 * each other, which may create associations issues. For those reasons, the
 * scanning activity should be limited to what's really needed, and continuous
 * scanning is a bad idea. --- Jean Tourrilhes
 *
 * Here is an example demonstrating the usage of the scanning API.
 *
 * @include scan.c
 *
 * A sample output of the above function is as follows.
 *
@verbatim
scan(): sleeptries: 5
scan(): sleeptries: 4
>> 00:23:f8:d2:90:3f ednz
>> 00:1c:a8:14:6d:16 meddev
>> 00:1a:2a:c0:47:84 suat
>> 00:1c:a8:fe:93:8c home
>> 00:25:86:cf:e6:e5 TP-LINK
>> 00:02:cf:af:d9:96 ZyXEL
>> 00:14:c1:34:ce:83 ozu
@endverbatim
 *
 * Considering @c scan() function, pay attention that scanning requires root
 * privileges. (See wapi_scan_init() for details.)
 *
 * @{
 */


/**
 * Starts a scan on the given interface. Root privileges are required to start a
 * scan.
 */
int wapi_scan_init(int sock, const char *ifname);


/**
 * Checks the status of the scan process.
 *
 * @return zero, if data is ready; 1, if data is not ready; negative on failure.
 */
int wapi_scan_stat(int sock, const char *ifname);


/**
 * Collects the results of a scan process.
 *
 * @param[out] aps Pushes collected @c wapi_scan_info_t into this list.
 */
int wapi_scan_coll(int sock, const char *ifname, wapi_list_t *aps);


/** @} scan */


/**
 * @defgroup commons Common Data Structures & Definitions
 * @{
 */


/** Path to @c /proc/net/wireless. (Requires procfs mounted.) */
#define WAPI_PROC_NET_WIRELESS "/proc/net/wireless"

/** Path to @c /proc/net/route. (Requires procfs mounted.) */
#define WAPI_PROC_NET_ROUTE "/proc/net/route"

/** Buffer size while reading lines from PROC_NET_ files. */
#define WAPI_PROC_LINE_SIZE	1024


/** Linked list container for strings. */
typedef struct wapi_string_t {
	struct wapi_string_t *next;
	char *data;
} wapi_string_t;


/** Linked list container for scan results. */
typedef struct wapi_scan_info_t {
	struct wapi_scan_info_t *next;
	struct ether_addr ap;
	int has_essid;
	char essid[WAPI_ESSID_MAX_SIZE+1];
	wapi_essid_flag_t essid_flag;
	int has_freq;
	double freq;
	int has_mode;
	wapi_mode_t mode;
	int has_bitrate;
	int bitrate;
} wapi_scan_info_t;


/** Linked list container for routing table rows. */
typedef struct wapi_route_info_t {
	struct wapi_route_info_t *next;
	char *ifname;
	struct in_addr dest;
	struct in_addr gw;
	unsigned int flags;	/**< See @c RTF_* in @c net/route.h for available values. */
	unsigned int refcnt;
	unsigned int use;
	unsigned int metric;
	struct in_addr netmask;
	unsigned int mtu;
	unsigned int window;
	unsigned int irtt;
} wapi_route_info_t;


/**
 * A generic linked list container. For functions taking @c wapi_list_t type of
 * argument, caller is resposible for releasing allocated memory.
 */
struct wapi_list_t {
	union wapi_list_head_t {
		wapi_string_t *string;
		wapi_scan_info_t *scan;
		wapi_route_info_t *route;
	} head;
};


/** @} commons */


#endif /* WAPI_H */
