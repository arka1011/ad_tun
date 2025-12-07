/*************************************************
 *************************************************
 **             Name: AD Tun Helper Header      **
 **             Author: Arkaprava Das           **
 *************************************************
 *************************************************/

#ifndef AD_TUN_HELPER_H
#define AD_TUN_HELPER_H

#include "../include/ad_tun.h"
#include <pthread.h>

/* Default zlog config file path */
const char *g_zlog_file_cfg = 
    "/home/arka/workspace/ad_vpn/prebuilt/zlog/config/ad_zlog_config.conf";
/* zlog initialization tracking and helpers */
int g_zlog_initialized = 0;

/* Mutex to protect zlog init/fini */
pthread_mutex_t g_zlog_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Convert an error code to a human-readable string.
 *
 * @param err Error code.
 * @return Static string describing the error.
 */
const char* ad_tun_strerror(ad_tun_error_t err);

/**
 * @brief Initialize zlog for the module.
 *
 * @return 0 on success, -1 on failure.
 */
static int ad_tun_zlog_init(void);

/**
 * @brief Finalize zlog for the module.
 */
static void ad_tun_zlog_fini(void);

#endif // AD_TUN_HELPER_H