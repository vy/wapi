#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "util.h"
#include "wapi.h"


/*-- IP Routines -------------------------------------------------------------*/


int
wapi_get_ip(int sock, const char *ifname, struct sockaddr *addr)
{
	struct ifreq ifr;
	int ret;

	WAPI_VALIDATE_PTR(addr);

	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_addr.sa_family = AF_INET;
	if ((ret = ioctl(sock, SIOCGIFADDR, &ifr)) >= 0)
		memcpy(addr, &ifr.ifr_addr, sizeof(struct sockaddr_in));
	else WAPI_IOCTL_STRERROR(SIOCGIFADDR);

	return ret;
}


int
wapi_set_ip(int sock, const char *ifname, const struct sockaddr *addr)
{
	struct ifreq ifr;
	int ret;

	WAPI_VALIDATE_PTR(addr);

	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	memcpy(&ifr.ifr_addr, addr, sizeof(struct sockaddr));
	if ((ret = ioctl(sock, SIOCSIFADDR, &ifr)) < 0)
		WAPI_IOCTL_STRERROR(SIOCSIFADDR);

	return ret;
}
