#include <gtest/gtest.h>

extern "C" {
#include "ad_tun.h"
}

TEST(StateTest, InitThenCleanup) {
    ad_tun_config_t cfg = {
        .ifname="tun0",
        .ipv4="10.8.0.2",
        .ipv6=NULL,
        .mtu=1500,
        .persist=0
    };

    EXPECT_EQ(AD_TUN_OK, ad_tun_init(&cfg));
    EXPECT_EQ(AD_TUN_STATE_INITIALIZED, ad_tun_get_state());

    EXPECT_EQ(AD_TUN_OK, ad_tun_cleanup());
    EXPECT_EQ(AD_TUN_STATE_UNINITIALIZED, ad_tun_get_state());
}
