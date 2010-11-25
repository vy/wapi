#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "util.h"
#include "wapi.h"


/*-- Up & Down ---------------------------------------------------------------*/


int
wapi_get_ifup(int sock, const char *ifname, int *is_up)
{
	struct ifreq ifr;
	int ret;

	WAPI_VALIDATE_PTR(is_up);

	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if ((ret = ioctl(sock, SIOCGIFFLAGS, &ifr)) >= 0)
		*is_up = (ifr.ifr_flags & IFF_UP) == IFF_UP;
	else WAPI_IOCTL_STRERROR(SIOCGIFFLAGS);

	return ret;
}


int
wapi_set_ifup(int sock, const char *ifname)
{
	struct ifreq ifr;
	int ret;

	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if ((ret = ioctl(sock, SIOCGIFFLAGS, &ifr)) >= 0)
	{
		ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
		ret = ioctl(sock, SIOCSIFFLAGS, &ifr);
	}
	else WAPI_IOCTL_STRERROR(SIOCGIFFLAGS);

	return ret;
}


int
wapi_set_ifdown(int sock, const char *ifname)
{
	struct ifreq ifr;
	int ret;

	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if ((ret = ioctl(sock, SIOCGIFFLAGS, &ifr)) >= 0)
	{
		ifr.ifr_flags &= ~IFF_UP;
		ret = ioctl(sock, SIOCSIFFLAGS, &ifr);
	}
	else WAPI_IOCTL_STRERROR(SIOCGIFFLAGS);

	return ret;
}


/*-- IP & Netmask ------------------------------------------------------------*/


static int
wapi_get_addr(int sock, const char *ifname, int cmd, struct sockaddr *addr)
{
	struct ifreq ifr;
	int ret;

	WAPI_VALIDATE_PTR(addr);

	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_addr.sa_family = AF_INET;
	if ((ret = ioctl(sock, cmd, &ifr)) >= 0)
		memcpy(addr, &ifr.ifr_addr, sizeof(struct sockaddr_in));
	else WAPI_IOCTL_STRERROR(cmd);

	return ret;
}


static int
wapi_set_addr(int sock, const char *ifname, int cmd, const struct sockaddr *addr)
{
	struct ifreq ifr;
	int ret;

	WAPI_VALIDATE_PTR(addr);

	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	memcpy(&ifr.ifr_addr, addr, sizeof(struct sockaddr));
	if ((ret = ioctl(sock, cmd, &ifr)) < 0)
		WAPI_IOCTL_STRERROR(cmd);

	return ret;
}


int
wapi_get_ip(int sock, const char *ifname, struct sockaddr *addr)
{
	return wapi_get_addr(sock, ifname, SIOCGIFADDR, addr);
}


int
wapi_set_ip(int sock, const char *ifname, const struct sockaddr *addr)
{
	return wapi_set_addr(sock, ifname, SIOCSIFADDR, addr);
}


int
wapi_get_netmask(int sock, const char *ifname, struct sockaddr *addr)
{
	return wapi_get_addr(sock, ifname, SIOCGIFNETMASK, addr);
}


int
wapi_set_netmask(int sock, const char *ifname, const struct sockaddr *addr)
{
	return wapi_set_addr(sock, ifname, SIOCSIFNETMASK, addr);
}
