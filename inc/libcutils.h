#pragma once
#ifndef LIBCUTILS_H
#define LIBCUTILS_H

#ifdef __cplusplus
extern "C" {
#endif	

	/**
	 * get version of libcutils
	 * note: return string pointer shouldn't free.
	 */
	char *libcutils_get_version();
	
#ifdef __cplusplus	
}
#endif

#endif // !LIBCUTILS_H
