#include "../include/ad_tun.h"
#include "../../prebuilt/inih/include/ini.h"
#include "../../prebuilt/zlog/include/zlog.h"
#include <stdlib.h>
#include <string.h>

/* Default values for ad_tun_config_t */
#define DEFAULT_MTU 1500
#define DEFAULT_PERSIST 0

/* INI handler callback with logging */
static int ad_tun_ini_handler(void* user, const char* section,
                              const char* name, const char* value)
{
    ad_tun_config_t* cfg = (ad_tun_config_t*)user;
    zlog_category_t *zc = zlog_get_category("ad_tun");

    /* Only handle [ad_tun] section */
    if (strcmp(section, "ad_tun") != 0) {
        zlog_debug(zc, "Ignoring section: %s", section);
        return 1; // ignore other sections
    }

    zlog_debug(zc, "Parsing config key: [%s] %s = %s", section, name, value);

    if (strcmp(name, "ifname") == 0) {
        cfg->ifname = strdup(value);
        if (!cfg->ifname) {
            zlog_error(zc, "Memory allocation failed for 'ifname'");
            return 0; // fail parsing
        }
    } else if (strcmp(name, "ipv4") == 0) {
        cfg->ipv4 = strdup(value);
        if (!cfg->ipv4) {
            zlog_error(zc, "Memory allocation failed for 'ipv4'");
            return 0;
        }
    } else if (strcmp(name, "ipv6") == 0) {
        cfg->ipv6 = strdup(value);
        if (!cfg->ipv6) {
            zlog_warn(zc, "Memory allocation failed for 'ipv6', IPv6 disabled");
            cfg->ipv6 = NULL;
        }
    } else if (strcmp(name, "mtu") == 0) {
        cfg->mtu = atoi(value);
    } else if (strcmp(name, "persist") == 0) {
        cfg->persist = atoi(value);
    } else {
        zlog_warn(zc, "Unknown config key ignored: %s", name);
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
        zlog_error(zlog_get_category("ad_tun"), "Invalid arguments to ad_tun_load_config()");
        return AD_TUN_ERR_CONFIG;
    }

    /* Zero initialize and set defaults */
    memset(out_cfg, 0, sizeof(*out_cfg));
    out_cfg->mtu = DEFAULT_MTU;
    out_cfg->persist = DEFAULT_PERSIST;

    zlog_category_t *zc = zlog_get_category("ad_tun");
    zlog_info(zc, "Loading config file: %s", path);

    int rc = ini_parse(path, ad_tun_ini_handler, out_cfg);
    if (rc < 0) {
        zlog_error(zc, "Failed to parse config file: %s", path);
        return AD_TUN_ERR_CONFIG;
    }

    /* Validate required fields */
    if (!out_cfg->ifname || strlen(out_cfg->ifname) == 0) {
        zlog_error(zc, "Config error: 'ifname' is missing or empty");
        ad_tun_free_config(out_cfg);
        return AD_TUN_ERR_CONFIG;
    }

    if (!out_cfg->ipv4 || strlen(out_cfg->ipv4) == 0) {
        zlog_error(zc, "Config error: 'ipv4' is missing or empty");
        ad_tun_free_config(out_cfg);
        return AD_TUN_ERR_CONFIG;
    }

    if (!out_cfg->ipv6 || strlen(out_cfg->ipv6) == 0) {
        zlog_warn(zc, "'ipv6' is missing or empty — IPv6 will be disabled");
        out_cfg->ipv6 = NULL;
    }

    if (out_cfg->mtu <= 0 || out_cfg->mtu > 9000) {
        zlog_warn(zc, "Config warning: 'mtu' is invalid (%d), using default %d", out_cfg->mtu, DEFAULT_MTU);
        out_cfg->mtu = DEFAULT_MTU;
    }

    if (out_cfg->persist != 0 && out_cfg->persist != 1) {
        zlog_warn(zc, "Config warning: 'persist' should be 0 or 1, using default %d", DEFAULT_PERSIST);
        out_cfg->persist = DEFAULT_PERSIST;
    }

    zlog_info(zc, "Config loaded successfully from %s", path);
    zlog_debug(zc, "ifname=%s, ipv4=%s, ipv6=%s, mtu=%d, persist=%d",
               out_cfg->ifname, out_cfg->ipv4, out_cfg->ipv6 ? out_cfg->ipv6 : "none",
               out_cfg->mtu, out_cfg->persist);

    return AD_TUN_OK;
}
