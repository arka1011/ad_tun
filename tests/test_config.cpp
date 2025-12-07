#include <gtest/gtest.h>

extern "C" {
#include "ad_tun.h"
}

TEST(ConfigTest, LoadValidConfig) {
    ad_tun_config_t cfg;

    ASSERT_EQ(AD_TUN_OK, ad_tun_load_config("../../test_configs/good.ini", &cfg));

    EXPECT_STREQ(cfg.ifname, "tun0");
    EXPECT_STREQ(cfg.ipv4, "10.8.0.2/24");
    EXPECT_EQ(cfg.mtu, 1500);

    ad_tun_free_config(&cfg);
}

TEST(ConfigTest, MissingIfnameFails) {
    ad_tun_config_t cfg;

    EXPECT_EQ(AD_TUN_ERR_CONFIG,
              ad_tun_load_config("../../test_configs/missing_ifname.ini", &cfg));
}
