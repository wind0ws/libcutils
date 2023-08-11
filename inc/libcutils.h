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
	 * 
	 * call at the beginning of your app.
	 */
	int libcutils_init();

	/**
	 * global deinit libcutils
	 * 
	 * call at ending of your app, 
	 * otherwise maybe some resource not released
	 */
	int libcutils_deinit();
	
#ifdef __cplusplus	
}
#endif

#endif // !LIBCUTILS_H
