#pragma once
#ifndef _LCU_H
#define _LCU_H

#ifdef __cplusplus
extern "C" {
#endif	

	/**
	 * get version of lcu
	 * note: do not free returned string 
	 */
	char* lcu_get_version();

	/**
	 * global init lcu
	 * 
	 * call at the beginning of your app.
	 */
	int lcu_global_init();

	/**
	 * global cleanup lcu
	 * 
	 * call at ending of your app, 
	 * otherwise maybe some resource not released
	 */
	int lcu_global_cleanup();
	
#ifdef __cplusplus	
}
#endif

#endif // !_LCU_H
