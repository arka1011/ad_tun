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

TEST(StateTest, InitStartStopCleanup) {
    ad_tun_config_t cfg = {
        .ifname="ad_tun0",
        .ipv4="10.10.0.2",
        .ipv6="fd00:1234:5678::2/64",
        .mtu=1500,
        .persist=0
    };

    EXPECT_EQ(AD_TUN_OK, ad_tun_init(&cfg));
    EXPECT_EQ(AD_TUN_STATE_INITIALIZED, ad_tun_get_state());

    EXPECT_EQ(AD_TUN_OK, ad_tun_start());
    EXPECT_EQ(AD_TUN_STATE_RUNNING, ad_tun_get_state());

    EXPECT_EQ(AD_TUN_OK, ad_tun_stop());
    EXPECT_EQ(AD_TUN_STATE_STOPPED, ad_tun_get_state());

    EXPECT_EQ(AD_TUN_OK, ad_tun_cleanup());
    EXPECT_EQ(AD_TUN_STATE_UNINITIALIZED, ad_tun_get_state());
}

TEST(StateTest, StartWithoutInitFails) {
    EXPECT_EQ(AD_TUN_ERR_CONFIG, ad_tun_start());
    EXPECT_EQ(AD_TUN_STATE_UNINITIALIZED, ad_tun_get_state());
}

TEST(StateTest, StopWithoutStartFails) {
    EXPECT_EQ(AD_TUN_ERR_INVALID_STATE, ad_tun_stop());
    EXPECT_EQ(AD_TUN_STATE_UNINITIALIZED, ad_tun_get_state());
}

TEST(StateTest, CleanupWithoutInitAccepts) {
    EXPECT_EQ(AD_TUN_OK, ad_tun_cleanup());
    EXPECT_EQ(AD_TUN_STATE_UNINITIALIZED, ad_tun_get_state());
}