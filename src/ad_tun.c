/*************************************************
**************************************************
**              Name: AD Tun Implementation     **
**              Author: Arkaprava Das           **
**************************************************
**************************************************/

#include "../include/ad_tun.h"
#include "../../../prebuilt/inih/include/ini.h"
#include "../../ad_logger/include/ad_logger.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>

/* Default values for ad_tun_config_t */
#define DEFAULT_MTU 1500
#define DEFAULT_PERSIST 0

/* Internal module state */
static ad_tun_state_t g_state = AD_TUN_STATE_UNINITIALIZED;
static ad_tun_config_t g_cfg;
static pthread_mutex_t g_state_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_tun_fd = -1;
static int g_config_initialized = 0;

/* ---- INI handler callback with logging ---- */
static int ad_tun_ini_handler(void* user, const char* section,
                              const char* name, const char* value)
{
    ad_tun_config_t* cfg = (ad_tun_config_t*)user;

    /* Only handle [ad_tun] section */
    if (strcmp(section, "ad_tun") != 0) {
        AD_LOG_TUN_DEBUG("Ignoring section: %s", section);
        return 1; // ignore other sections
    }

    AD_LOG_TUN_DEBUG("Parsing config key: [%s] %s = %s", section, name, value);

    if (strcmp(name, "ifname") == 0) {
        cfg->ifname = strdup(value);
        if (!cfg->ifname) {
            AD_LOG_TUN_ERROR("Memory allocation failed for 'ifname'");
            return 0; // fail parsing
        }
    } else if (strcmp(name, "ipv4") == 0) {
        cfg->ipv4 = strdup(value);
        if (!cfg->ipv4) {
            AD_LOG_TUN_ERROR("Memory allocation failed for 'ipv4'");
            return 0;
        }
    } else if (strcmp(name, "ipv6") == 0) {
        cfg->ipv6 = strdup(value);
        if (!cfg->ipv6) {
            AD_LOG_TUN_WARN("Memory allocation failed for 'ipv6', IPv6 disabled");
            cfg->ipv6 = NULL;
        }
    } else if (strcmp(name, "mtu") == 0) {
        cfg->mtu = atoi(value);
    } else if (strcmp(name, "persist") == 0) {
        cfg->persist = atoi(value);
    } else {
        AD_LOG_TUN_WARN("Unknown config key ignored: %s", name);
    }

    return 1; // success
}

/* Free heap-allocated strings inside ad_tun_config_t */
void ad_tun_free_config(ad_tun_config_t *cfg)
{
    if (!cfg) return;

    free((char*)cfg->ifname);
    free((char*)cfg->ipv4);
    free((char*)cfg->ipv6);

    memset(cfg, 0, sizeof(*cfg));
}

/* Load configuration from INI file */
ad_tun_error_t ad_tun_load_config(const char *path, ad_tun_config_t *out_cfg)
{
    if (!path || !out_cfg) {
        AD_LOG_TUN_ERROR("Invalid arguments to ad_tun_load_config()");
        return AD_TUN_ERR_CONFIG;
    }

    /* Zero initialize and set defaults */
    memset(out_cfg, 0, sizeof(*out_cfg));
    out_cfg->mtu = DEFAULT_MTU;
    out_cfg->persist = DEFAULT_PERSIST;

    AD_LOG_TUN_INFO("Loading config file: %s", path);

    int rc = ini_parse(path, ad_tun_ini_handler, out_cfg);
    if (rc < 0) {
        AD_LOG_TUN_ERROR("Failed to open config file: %s", path);
        return AD_TUN_ERR_CONFIG;
    } else if (rc > 0) {
        AD_LOG_TUN_ERROR("Parsing error at line %d in config file %s", rc, path);
        return AD_TUN_ERR_CONFIG;
    }

    /* Validate required fields */
    if (!out_cfg->ifname || strlen(out_cfg->ifname) == 0) {
        AD_LOG_TUN_ERROR("Config error: 'ifname' is missing or empty");
        ad_tun_free_config(out_cfg);
        return AD_TUN_ERR_CONFIG;
    }

    if (!out_cfg->ipv4 || strlen(out_cfg->ipv4) == 0) {
        AD_LOG_TUN_ERROR("Config error: 'ipv4' is missing or empty");
        ad_tun_free_config(out_cfg);
        return AD_TUN_ERR_CONFIG;
    }

    if (!out_cfg->ipv6 || strlen(out_cfg->ipv6) == 0) {
        AD_LOG_TUN_WARN("'ipv6' is missing or empty — IPv6 will be disabled");
        out_cfg->ipv6 = NULL;
    }

    if (out_cfg->mtu <= 0 || out_cfg->mtu > 9000) {
        AD_LOG_TUN_WARN("Config warning: 'mtu' is invalid (%d), using default %d", out_cfg->mtu, DEFAULT_MTU);
        out_cfg->mtu = DEFAULT_MTU;
    }

    if (out_cfg->persist != 0 && out_cfg->persist != 1) {
        AD_LOG_TUN_WARN("Config warning: 'persist' should be 0 or 1, using default %d", DEFAULT_PERSIST);
        out_cfg->persist = DEFAULT_PERSIST;
    }

    AD_LOG_TUN_INFO("Config loaded successfully from %s", path);
    AD_LOG_TUN_DEBUG("ifname=%s, ipv4=%s, ipv6=%s, mtu=%d, persist=%d",
               out_cfg->ifname, out_cfg->ipv4, out_cfg->ipv6 ? out_cfg->ipv6 : "none",
               out_cfg->mtu, out_cfg->persist);

    return AD_TUN_OK;
}

/* Initialize the TUN module with a config */
ad_tun_error_t ad_tun_init(const ad_tun_config_t *cfg)
{
    if (!cfg) {
        AD_LOG_TUN_ERROR("ad_tun_init called with NULL config");
        return AD_TUN_ERR_CONFIG;
    }

    pthread_mutex_lock(&g_state_lock);

    if (g_state != AD_TUN_STATE_UNINITIALIZED && g_state != AD_TUN_STATE_STOPPED) {
        AD_LOG_TUN_WARN("ad_tun_init called while module in state %d", g_state);
        pthread_mutex_unlock(&g_state_lock);
        return AD_TUN_ERR_INVALID_STATE;
    }

    /* Clear previous config if any */
    if (g_config_initialized) {
        ad_tun_free_config(&g_cfg);
        g_config_initialized = 0;
    }

    /* Copy strings */
    memset(&g_cfg, 0, sizeof(g_cfg));

    if (cfg->ifname) g_cfg.ifname = strdup(cfg->ifname);
    if (cfg->ipv4)   g_cfg.ipv4   = strdup(cfg->ipv4);
    if (cfg->ipv6)   g_cfg.ipv6   = strdup(cfg->ipv6);

    g_cfg.mtu     = (cfg->mtu > 0) ? cfg->mtu : DEFAULT_MTU;
    g_cfg.persist = (cfg->persist == 1) ? 1 : 0;

    g_config_initialized = 1;
    g_state = AD_TUN_STATE_INITIALIZED;

    AD_LOG_TUN_INFO("ad_tun module initialized: ifname=%s, ipv4=%s, ipv6=%s, mtu=%d, persist=%d",
              g_cfg.ifname, g_cfg.ipv4, g_cfg.ipv6 ? g_cfg.ipv6 : "none",
              g_cfg.mtu, g_cfg.persist);

    pthread_mutex_unlock(&g_state_lock);
    return AD_TUN_OK;
}

/* Start the TUN interface */
ad_tun_error_t ad_tun_start(void)
{
    pthread_mutex_lock(&g_state_lock);

    /* Check module state */
    if (!g_config_initialized) {
        pthread_mutex_unlock(&g_state_lock);
        AD_LOG_TUN_ERROR("Cannot start: configuration not initialized");
        return AD_TUN_ERR_CONFIG;
    }

    if (g_state != AD_TUN_STATE_INITIALIZED && g_state != AD_TUN_STATE_STOPPED) {
        pthread_mutex_unlock(&g_state_lock);
        AD_LOG_TUN_ERROR("Cannot start: module is in wrong state (%d)", g_state);
        return AD_TUN_ERR_INVALID_STATE;
    }

    /* Local copy so we release lock early */
    ad_tun_config_t cfg = g_cfg;
    pthread_mutex_unlock(&g_state_lock);

    AD_LOG_TUN_INFO("Starting TUN interface: %s", cfg.ifname);

    /* Open /dev/net/tun */
    int tun_fd = open("/dev/net/tun", O_RDWR | O_NONBLOCK);
    if (tun_fd < 0) {
        AD_LOG_TUN_ERROR("Failed to open /dev/net/tun: %s", strerror(errno));
        return AD_TUN_ERR_NO_DEVICE;
    }

    /* Prepare interface request */
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    /* Use IFF_TUN and avoid packet information. */
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    /* copy name */
    strncpy(ifr.ifr_name, cfg.ifname, IFNAMSIZ - 1);

    /* Issue ioctl */
    if (ioctl(tun_fd, TUNSETIFF, (void *)&ifr) < 0) {
        AD_LOG_TUN_ERROR("ioctl(TUNSETIFF) failed: %s", strerror(errno));
        close(tun_fd);
        return AD_TUN_ERR_SYS;
    }

    AD_LOG_TUN_INFO("TUN interface %s created successfully", ifr.ifr_name);

    /* Configure MTU via netlink/ioctl (we use ioctl SIOCSIFMTU here) */
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock >= 0) {
        struct ifreq if_mtu;
        memset(&if_mtu, 0, sizeof(if_mtu));
        strncpy(if_mtu.ifr_name, cfg.ifname, IFNAMSIZ - 1);
        if_mtu.ifr_mtu = cfg.mtu;

        if (ioctl(sock, SIOCSIFMTU, &if_mtu) < 0) {
            AD_LOG_TUN_WARN("Failed to set MTU=%d on %s: %s", cfg.mtu, cfg.ifname, strerror(errno));
        } else {
            AD_LOG_TUN_INFO("Set MTU=%d on %s", cfg.mtu, cfg.ifname);
        }

        close(sock);
    } else {
        AD_LOG_TUN_WARN("Failed to open socket for MTU configuration: %s", strerror(errno));
    }

    /* Configure IPv4 */
    if (cfg.ipv4) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd),
                 "ip addr add %s dev %s >/dev/null 2>&1",
                 cfg.ipv4, cfg.ifname);
        if (system(cmd) != 0)
            AD_LOG_TUN_WARN("Failed to assign IPv4: %s", cfg.ipv4);
        else
            AD_LOG_TUN_INFO("Assigned IPv4: %s", cfg.ipv4);
    }

    /* Configure IPv6 */
    if (cfg.ipv6) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd),
                 "ip -6 addr add %s dev %s >/dev/null 2>&1",
                 cfg.ipv6, cfg.ifname);
        if (system(cmd) != 0)
            AD_LOG_TUN_WARN("Failed to assign IPv6: %s", cfg.ipv6);
        else
            AD_LOG_TUN_INFO("Assigned IPv6: %s", cfg.ipv6);
    }

    /* Bring interface up */
    {
        char cmd[128];
        snprintf(cmd, sizeof(cmd),
                 "ip link set dev %s up >/dev/null 2>&1", cfg.ifname);
        if (system(cmd) != 0)
            AD_LOG_TUN_WARN("Failed to bring interface up");
        else
            AD_LOG_TUN_INFO("Interface %s is now UP", cfg.ifname);
    }

    /* Update state */
    pthread_mutex_lock(&g_state_lock);
    g_state = AD_TUN_STATE_RUNNING;
    g_tun_fd = tun_fd; /* fixed assignment */
    pthread_mutex_unlock(&g_state_lock);

    AD_LOG_TUN_INFO("ad_tun_start() completed successfully");

    return AD_TUN_OK;
}

/* Stop the TUN interface */
ad_tun_error_t ad_tun_stop(void)
{

    pthread_mutex_lock(&g_state_lock);

    if (!g_config_initialized) {
        AD_LOG_TUN_ERROR("ad_tun_stop(): Config not initialized");
        pthread_mutex_unlock(&g_state_lock);
        return AD_TUN_ERR_INVALID_STATE;
    }

    if (g_state != AD_TUN_STATE_RUNNING) {
        AD_LOG_TUN_WARN("ad_tun_stop(): Interface is not running (state=%d)", g_state);
        pthread_mutex_unlock(&g_state_lock);
        return AD_TUN_ERR_INVALID_STATE;
    }

    /* Snapshot fd & ifname */
    int fd = g_tun_fd;
    const char *ifname = g_cfg.ifname;

    pthread_mutex_unlock(&g_state_lock);

    /* Bring interface down */
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "ip link set %s down >/dev/null 2>&1", ifname);
    int rc = system(cmd);
    if (rc != 0) {
        AD_LOG_TUN_WARN("Failed to bring interface %s down (rc=%d)", ifname, rc);
        /* non-fatal — continue stopping */
    }

    /* Close TUN file descriptor */
    if (fd >= 0) {
        close(fd);
    }

    /* Clear global state */
    pthread_mutex_lock(&g_state_lock);
    g_tun_fd = -1;
    g_state = AD_TUN_STATE_STOPPED;
    pthread_mutex_unlock(&g_state_lock);

    AD_LOG_TUN_INFO("TUN interface %s stopped successfully", ifname);

    return AD_TUN_OK;
}

/* Cleanup the TUN module */
ad_tun_error_t ad_tun_cleanup(void)
{

    pthread_mutex_lock(&g_state_lock);

    /* If uninitialized, cleanup is a no-op */
    if (g_state == AD_TUN_STATE_UNINITIALIZED) {
        pthread_mutex_unlock(&g_state_lock);
        AD_LOG_TUN_INFO("Cleanup requested but module already uninitialized");
        return AD_TUN_OK;
    }

    /* If still running, stop it first */
    if (g_state == AD_TUN_STATE_RUNNING) {
        pthread_mutex_unlock(&g_state_lock);  // release before calling stop()
        ad_tun_stop();                         // safe: stop() locks internally
        pthread_mutex_lock(&g_state_lock);     // re-acquire lock
    }

    /*
     * Now we are guaranteed the module is not RUNNING.
     * Free configuration if it was allocated.
     */
    if (g_config_initialized) {
        ad_tun_free_config(&g_cfg);  // frees strings if dynamically allocated
        memset(&g_cfg, 0, sizeof(g_cfg));
        g_config_initialized = 0;
    }

    /* Reset global state */
    g_state = AD_TUN_STATE_UNINITIALIZED;

    pthread_mutex_unlock(&g_state_lock);

    AD_LOG_TUN_INFO("Cleanup completed successfully");

    return AD_TUN_OK;
}

/* Restart the TUN interface */
ad_tun_error_t ad_tun_restart(void)
{
    ad_tun_error_t err;

    /* First stop the interface */
    err = ad_tun_stop();
    if (err != AD_TUN_OK) {
        return err;
    }

    /* Then start it again */
    err = ad_tun_start();
    if (err != AD_TUN_OK) {
        return err;
    }

    return AD_TUN_OK;
}

/* Read data from the TUN interface */
ssize_t ad_tun_read(char *buf, size_t buf_len)
{

    if (!buf || buf_len == 0) {
        AD_LOG_TUN_ERROR("ad_tun_read: invalid buffer");
        return -EINVAL;
    }

    /* Ensure module is running */
    pthread_mutex_lock(&g_state_lock);
    int running = (g_state == AD_TUN_STATE_RUNNING && g_tun_fd > 0);
    int fd = g_tun_fd;
    pthread_mutex_unlock(&g_state_lock);

    if (!running) {
        AD_LOG_TUN_ERROR("ad_tun_read: called while module not running");
        return -EIO;
    }

    ssize_t n = read(fd, buf, buf_len);

    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            /* No data available */
            return -EAGAIN;
        }
        AD_LOG_TUN_ERROR("ad_tun_read: read() failed: %s", strerror(errno));
        return -EIO;
    }

    AD_LOG_TUN_DEBUG("ad_tun_read: read %zd bytes from TUN", n);
    return n;
}

/* Write data to the TUN interface */
ssize_t ad_tun_write(const char *buf, size_t buf_len)
{

    if (!buf || buf_len == 0) {
        AD_LOG_TUN_ERROR("ad_tun_write: invalid buffer");
        return -EINVAL;
    }

    /* Ensure module is running */
    pthread_mutex_lock(&g_state_lock);
    int running = (g_state == AD_TUN_STATE_RUNNING && g_tun_fd > 0);
    int fd = g_tun_fd;
    pthread_mutex_unlock(&g_state_lock);

    if (!running) {
        AD_LOG_TUN_ERROR("ad_tun_write: called while module not running");
        return -EIO;
    }

    ssize_t n = write(fd, buf, buf_len);

    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            /* Write would block */
            return -EAGAIN;
        }
        AD_LOG_TUN_ERROR("ad_tun_write: write() failed: %s", strerror(errno));
        return -EIO;
    }

    AD_LOG_TUN_DEBUG("ad_tun_write: wrote %zd bytes to TUN", n);
    return n;
}

/* Return the TUN file descriptor */
int ad_tun_get_fd(void)
{
    pthread_mutex_lock(&g_state_lock);
    int fd = g_tun_fd;
    pthread_mutex_unlock(&g_state_lock);

    return fd;
}

/* Helper to get internal config pointer */
ad_tun_config_t ad_tun_get_config_copy(void)
{
    ad_tun_config_t copy;

    pthread_mutex_lock(&g_state_lock);
    copy = g_cfg;  // safe, atomic copy of struct
    pthread_mutex_unlock(&g_state_lock);

    return copy;
}

/* Return interface name (e.g., "tun0") */
const char* ad_tun_get_name(void)
{
    pthread_mutex_lock(&g_state_lock);
    const char *name = g_cfg.ifname;
    pthread_mutex_unlock(&g_state_lock);

    return name;
}

/* Return configured MTU */
int ad_tun_get_mtu(void)
{
    pthread_mutex_lock(&g_state_lock);
    int mtu = g_cfg.mtu;
    pthread_mutex_unlock(&g_state_lock);

    return mtu;
}

/* Return configured IPv4 address */
const char* ad_tun_get_ipv4(void)
{
    pthread_mutex_lock(&g_state_lock);
    const char *ip = g_cfg.ipv4;
    pthread_mutex_unlock(&g_state_lock);

    return ip;
}

/* Return configured IPv6 address */
const char* ad_tun_get_ipv6(void)
{
    pthread_mutex_lock(&g_state_lock);
    const char *ip6 = g_cfg.ipv6;
    pthread_mutex_unlock(&g_state_lock);

    return ip6;
}

/* Return current module state */
ad_tun_state_t ad_tun_get_state(void)
{
    pthread_mutex_lock(&g_state_lock);
    ad_tun_state_t s = g_state;
    pthread_mutex_unlock(&g_state_lock);

    return s;
}

/* Convert error code to string */
const char* ad_tun_strerror(ad_tun_error_t err)
{
    switch (err) {
    case AD_TUN_OK:
        return "Operation successful";

    case AD_TUN_ERR_INVALID_STATE:
        return "Invalid state for requested operation";

    case AD_TUN_ERR_NO_DEVICE:
        return "TUN device not available or could not be opened";

    case AD_TUN_ERR_SYS:
        return "System-level error (check errno)";

    case AD_TUN_ERR_CONFIG:
        return "Invalid or missing configuration";

    case AD_TUN_ERR_INTERNAL:
        return "Internal error";

    default:
        return "Unknown error";
    }
}
