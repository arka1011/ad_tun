#include <gtest/gtest.h>

extern "C" {
#include "ad_tun.h"
}

TEST(ConfigTest, LoadValidConfig) {
    ad_tun_config_t cfg;

    ASSERT_EQ(AD_TUN_OK, ad_tun_load_config("../../test_configs/good.ini", &cfg));

    EXPECT_STREQ(cfg.ifname, "ad_tun0");
    EXPECT_STREQ(cfg.ipv4, "10.10.1.2/24");
    EXPECT_STREQ(cfg.ipv6, "fea0:1234:5678::9/64");
    EXPECT_EQ(cfg.mtu, 2400);

    ad_tun_free_config(&cfg);
}

TEST(ConfigTest, MissingIfnameFails) {
    ad_tun_config_t cfg;

    EXPECT_EQ(AD_TUN_ERR_CONFIG,
              ad_tun_load_config("../../test_configs/missing_ifname.ini", &cfg));
}

TEST(ConfigTest, MissingIpv4Fails) {
    ad_tun_config_t cfg;

    EXPECT_EQ(AD_TUN_ERR_CONFIG,
              ad_tun_load_config("../../test_configs/missing_ipv4.ini", &cfg));
}

TEST(ConfigTest, MissingIpv6MtuPersistAllowed) {
    ad_tun_config_t cfg;

    EXPECT_EQ(AD_TUN_OK,
              ad_tun_load_config("../../test_configs/missing_ipv6MtuPersist.ini", &cfg));
}