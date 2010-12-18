/**
 * @file
 * Utility macros & declarations.
 */


#ifndef UTIL_H
#define UTIL_H


#include <string.h>
#include <errno.h>
#include <libgen.h>


#define WAPI_IOCTL_STRERROR(cmd)						\
	fprintf(											\
		stderr, "%s:%d:%s():ioctl(%s): %s\n",			\
		basename(__FILE__), __LINE__, __func__,			\
		wapi_ioctl_command_name(cmd), strerror(errno))


#define WAPI_STRERROR(fmt, ...)					\
	fprintf(									\
		stderr, "%s:%d:%s():" fmt ": %s\n",		\
		basename(__FILE__), __LINE__, __func__,	\
		## __VA_ARGS__, strerror(errno))


#define WAPI_ERROR(fmt, ...)									\
	fprintf(													\
		stderr, "%s:%d:%s(): " fmt ,							\
		basename(__FILE__), __LINE__, __func__, ## __VA_ARGS__)


#define WAPI_VALIDATE_PTR(ptr)						\
	if (!ptr)										\
	{												\
		WAPI_ERROR("Null pointer: %s.\n", #ptr);	\
		return -1;									\
	}


const char *wapi_ioctl_command_name(int cmd);


#endif /* UTIL_H */
