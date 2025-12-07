/*************************************************
**************************************************
**              Name: AD Tun Interface          **
**              Author: Arkaprava Das           **
**************************************************
**************************************************/

#ifndef AD_TUN_SRC_AD_TUN_H_
#define AD_TUN_SRC_AD_TUN_H_

#include <stddef.h>
#include <sys/types.h>

/**
 * @brief Error codes returned by AD Tun operations.
 */
typedef enum {
    AD_TUN_OK = 0,
    AD_TUN_ERR_INVALID_STATE,  /**< Operation not allowed in current state */
    AD_TUN_ERR_NO_DEVICE,      /**< TUN device cannot be created/opened   */
    AD_TUN_ERR_SYS,            /**< Underlying system call failure        */
    AD_TUN_ERR_CONFIG,         /**< Invalid or unsupported configuration  */
    AD_TUN_ERR_INTERNAL        /**< Unexpected internal error             */
} ad_tun_error_t;

/**
 * @brief States of the TUN interface lifecycle.
 */
typedef enum {
    AD_TUN_STATE_UNINITIALIZED = 0,
    AD_TUN_STATE_INITIALIZED,
    AD_TUN_STATE_RUNNING,
    AD_TUN_STATE_STOPPED,
    AD_TUN_STATE_ERROR
} ad_tun_state_t;

/**
 * @brief Configuration parameters for initializing a TUN interface.
 *
 * All string fields must remain valid for the lifetime of the interface
 * unless otherwise documented.
 */
typedef struct {
    const char *ifname;  /**< Interface name (e.g., "tun0") */
    const char *ipv4;    /**< IPv4 address (e.g., "10.8.0.1/24") */
    const char *ipv6;    /**< IPv6 address (e.g., "fd00::1/64") */
    int mtu;             /**< MTU value */
    int persist;         /**< Whether the TUN device should persist after close */
} ad_tun_config_t;

/**
 * @brief Load AD-TUN configuration from an INI file.
 *
 * @param path Path to the INI configuration file.
 * @param out_cfg Pointer to a caller-allocated config struct to fill.
 *
 * @return AD_TUN_OK on success, or appropriate error code.
 *
 * @note All strings in out_cfg will be heap-allocated using strdup().
 *       Caller is responsible for freeing them using ad_tun_free_config().
 */
ad_tun_error_t ad_tun_load_config(const char *path, ad_tun_config_t *out_cfg);

/**
 * @brief Free all heap-allocated fields inside ad_tun_config_t.
 */
void ad_tun_free_config(ad_tun_config_t *cfg);

/**
 * @brief Initialize the TUN interface with the given configuration.
 *
 * Must be called before any other operation.
 *
 * @param cfg Pointer to configuration structure.
 * @return AD_TUN_OK on success, error code on failure.
 */
ad_tun_error_t ad_tun_init(const ad_tun_config_t *cfg);

/**
 * @brief Create the TUN device, configure it, and bring it up.
 *
 * @return AD_TUN_OK on success, error code on failure.
 */
ad_tun_error_t ad_tun_start(void);

/**
 * @brief Stop the TUN interface and bring it down.
 *
 * @return AD_TUN_OK on success, error code on failure.
 */
ad_tun_error_t ad_tun_stop(void);

/**
 * @brief Convenience function: stop + start.
 *
 * @return AD_TUN_OK on success, error code on failure.
 */
ad_tun_error_t ad_tun_restart(void);

/**
 * @brief Cleanup resources and close the TUN device.
 *
 * After this call, the interface returns to UNINITIALIZED state.
 *
 * @return AD_TUN_OK on success, error code on failure.
 */
ad_tun_error_t ad_tun_cleanup(void);

/**
 * @brief Read raw IP packets from the TUN interface.
 *
 * @param buf Buffer to write into.
 * @param buf_len Size of the buffer.
 * @return Number of bytes read, or negative on error.
 */
ssize_t ad_tun_read(char *buf, size_t buf_len);

/**
 * @brief Write raw IP packets to the TUN interface.
 *
 * @param buf Packet buffer.
 * @param buf_len Packet length.
 * @return Number of bytes written, or negative on error.
 */
ssize_t ad_tun_write(const char *buf, size_t buf_len);

/**
 * @brief Get the TUN file descriptor for event loops or polling.
 *
 * @return File descriptor, or -1 if not running.
 */
int ad_tun_get_fd(void);

/**
 * @brief Get a copy of the configuration used to initialize the interface.
 *
 * @return A copy of the internal config.
 */
ad_tun_config_t ad_tun_get_config_copy(void);

/**
 * @brief Get the interface name.
 *
 * @return String pointer, or NULL if unavailable.
 */
const char* ad_tun_get_name(void);

/**
 * @brief Get the configured MTU.
 *
 * @return MTU value, or -1 if unavailable.
 */
int ad_tun_get_mtu(void);

/**
 * @brief Get the configured IPv4 address.
 *
 * @return IPv4 string, or NULL if unset.
 */
const char* ad_tun_get_ipv4(void);

/**
 * @brief Get the configured IPv6 address.
 *
 * @return IPv6 string, or NULL if unset.
 */
const char* ad_tun_get_ipv6(void);

/**
 * @brief Retrieve the current interface state.
 *
 * @return One of ad_tun_state_t.
 */
ad_tun_state_t ad_tun_get_state(void);

#endif
