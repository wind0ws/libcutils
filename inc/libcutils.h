#pragma once
#ifndef LIBCUTILS_H
#define LIBCUTILS_H

#ifdef __cplusplus
extern "C" {
#endif	

	/**
	 * get version of libcutils
	 * note: do not free returned string 
	 */
	char* libcutils_get_version();

	/**
	 * global init libcutils
	 */
	int libcutils_init();

	/**
	 * global deinit libcutils
	 * 
	 * call it at end of your app, 
	 * otherwise maybe some resource not released
	 */
	int libcutils_deinit();
	
#ifdef __cplusplus	
}
#endif

#endif // !LIBCUTILS_H
