#ifndef _GPM_HOOKS_H
#define _GPM_HOOKS_H


WG_PUBLIC wg_status 
cb_exit(wg_uint argc, wg_char *args[], void *private_data);

WG_PUBLIC wg_status 
cb_info(wg_uint argc, wg_char *args[], void *private_data);

WG_PUBLIC wg_status 
cb_start(wg_uint argc, wg_char *args[], void *private_data);

WG_PUBLIC wg_status 
cb_stop(wg_uint argc, wg_char *args[], void *private_data);

WG_PUBLIC wg_status 
cb_pause(wg_uint argc, wg_char *args[], void *private_data);

WG_PUBLIC wg_status 
cb_lsg(wg_uint argc, wg_char *args[], void *private_data);

WG_PUBLIC wg_status 
cb_send(wg_uint argc, wg_char *args[], void *private_data);

WG_PUBLIC wg_status
cb_connect(wg_uint argc, wg_char *args[], void *private_data);

#endif
