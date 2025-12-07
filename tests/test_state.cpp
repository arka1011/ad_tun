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

// New comprehensive combinations and edge cases

TEST(StateTest, RestartWithoutStartFailsWhenOnlyInitialized) {
    ad_tun_config_t cfg = {
        .ifname="tun_restart0",
        .ipv4="10.20.0.2",
        .ipv6=NULL,
        .mtu=1400,
        .persist=0
    };

    EXPECT_EQ(AD_TUN_OK, ad_tun_init(&cfg));
    EXPECT_EQ(AD_TUN_STATE_INITIALIZED, ad_tun_get_state());

    // restart should fail because stop() will fail (not running)
    EXPECT_EQ(AD_TUN_ERR_INVALID_STATE, ad_tun_restart());
    EXPECT_EQ(AD_TUN_STATE_INITIALIZED, ad_tun_get_state());

    EXPECT_EQ(AD_TUN_OK, ad_tun_cleanup());
}

TEST(StateTest, DoubleStartFails) {
    ad_tun_config_t cfg = {
        .ifname="tun_double_start",
        .ipv4="10.30.0.2",
        .ipv6=NULL,
        .mtu=1500,
        .persist=0
    };

    EXPECT_EQ(AD_TUN_OK, ad_tun_init(&cfg));
    EXPECT_EQ(AD_TUN_OK, ad_tun_start());
    EXPECT_EQ(AD_TUN_STATE_RUNNING, ad_tun_get_state());

    // starting while already running should fail
    EXPECT_EQ(AD_TUN_ERR_INVALID_STATE, ad_tun_start());
    EXPECT_EQ(AD_TUN_STATE_RUNNING, ad_tun_get_state());

    EXPECT_EQ(AD_TUN_OK, ad_tun_stop());
    EXPECT_EQ(AD_TUN_OK, ad_tun_cleanup());
}

TEST(StateTest, DoubleStopFails) {
    ad_tun_config_t cfg = {
        .ifname="tun_double_stop",
        .ipv4="10.40.0.2",
        .ipv6=NULL,
        .mtu=1500,
        .persist=0
    };

    EXPECT_EQ(AD_TUN_OK, ad_tun_init(&cfg));
    EXPECT_EQ(AD_TUN_OK, ad_tun_start());
    EXPECT_EQ(AD_TUN_OK, ad_tun_stop());
    EXPECT_EQ(AD_TUN_STATE_STOPPED, ad_tun_get_state());

    // stopping when already stopped should fail with invalid state
    EXPECT_EQ(AD_TUN_ERR_INVALID_STATE, ad_tun_stop());
    EXPECT_EQ(AD_TUN_STATE_STOPPED, ad_tun_get_state());

    EXPECT_EQ(AD_TUN_OK, ad_tun_cleanup());
}

TEST(StateTest, StartAfterStopSucceeds) {
    ad_tun_config_t cfg = {
        .ifname="tun_start_after_stop",
        .ipv4="10.50.0.2",
        .ipv6=NULL,
        .mtu=1500,
        .persist=0
    };

    EXPECT_EQ(AD_TUN_OK, ad_tun_init(&cfg));
    EXPECT_EQ(AD_TUN_OK, ad_tun_start());
    EXPECT_EQ(AD_TUN_OK, ad_tun_stop());

    // start should work again after stop (state STOPPED is allowed)
    EXPECT_EQ(AD_TUN_OK, ad_tun_start());
    EXPECT_EQ(AD_TUN_STATE_RUNNING, ad_tun_get_state());

    EXPECT_EQ(AD_TUN_OK, ad_tun_stop());
    EXPECT_EQ(AD_TUN_OK, ad_tun_cleanup());
}

TEST(StateTest, RestartCycleWorks) {
    ad_tun_config_t cfg = {
        .ifname="tun_restart_cycle",
        .ipv4="10.60.0.2",
        .ipv6=NULL,
        .mtu=1500,
        .persist=0
    };

    EXPECT_EQ(AD_TUN_OK, ad_tun_init(&cfg));
    EXPECT_EQ(AD_TUN_OK, ad_tun_start());
    EXPECT_EQ(AD_TUN_STATE_RUNNING, ad_tun_get_state());

    // restart should stop then start again
    EXPECT_EQ(AD_TUN_OK, ad_tun_restart());
    EXPECT_EQ(AD_TUN_STATE_RUNNING, ad_tun_get_state());

    EXPECT_EQ(AD_TUN_OK, ad_tun_stop());
    EXPECT_EQ(AD_TUN_OK, ad_tun_cleanup());
}

TEST(StateTest, RestartWithoutInitFails) {
    // calling restart while completely uninitialized should fail
    EXPECT_EQ(AD_TUN_ERR_INVALID_STATE, ad_tun_restart());
    EXPECT_EQ(AD_TUN_STATE_UNINITIALIZED, ad_tun_get_state());
}

TEST(StateTest, InitStopWithoutStartFails) {
    ad_tun_config_t cfg = {
        .ifname="tun_init_stop",
        .ipv4="10.70.0.2",
        .ipv6=NULL,
        .mtu=1500,
        .persist=0
    };

    EXPECT_EQ(AD_TUN_OK, ad_tun_init(&cfg));
    // stopping without starting should fail
    EXPECT_EQ(AD_TUN_ERR_INVALID_STATE, ad_tun_stop());
    EXPECT_EQ(AD_TUN_STATE_INITIALIZED, ad_tun_get_state());

    EXPECT_EQ(AD_TUN_OK, ad_tun_cleanup());
}

TEST(StateTest, StartAfterCleanupFails) {
    ad_tun_config_t cfg = {
        .ifname="tun_start_after_cleanup",
        .ipv4="10.80.0.2",
        .ipv6=NULL,
        .mtu=1500,
        .persist=0
    };

    EXPECT_EQ(AD_TUN_OK, ad_tun_init(&cfg));
    EXPECT_EQ(AD_TUN_OK, ad_tun_cleanup());

    // after cleanup, starting should fail because config not initialized
    EXPECT_EQ(AD_TUN_ERR_CONFIG, ad_tun_start());
    EXPECT_EQ(AD_TUN_STATE_UNINITIALIZED, ad_tun_get_state());
}

TEST(StateTest, CleanupWhileRunningStopsAndCleans) {
    ad_tun_config_t cfg = {
        .ifname="tun_cleanup_running",
        .ipv4="10.90.0.2",
        .ipv6=NULL,
        .mtu=1500,
        .persist=0
    };

    EXPECT_EQ(AD_TUN_OK, ad_tun_init(&cfg));
    EXPECT_EQ(AD_TUN_OK, ad_tun_start());
    EXPECT_EQ(AD_TUN_STATE_RUNNING, ad_tun_get_state());

    // cleanup should internally stop the running interface and then uninitialize
    EXPECT_EQ(AD_TUN_OK, ad_tun_cleanup());
    EXPECT_EQ(AD_TUN_STATE_UNINITIALIZED, ad_tun_get_state());
}

TEST(StateTest, StartThenInitSequence) {
    // start without init fails, then init allows start
    EXPECT_EQ(AD_TUN_ERR_CONFIG, ad_tun_start());

    ad_tun_config_t cfg = {
        .ifname="tun_late_init",
        .ipv4="10.100.0.2",
        .ipv6=NULL,
        .mtu=1500,
        .persist=0
    };

    EXPECT_EQ(AD_TUN_OK, ad_tun_init(&cfg));
    EXPECT_EQ(AD_TUN_OK, ad_tun_start());
    EXPECT_EQ(AD_TUN_STATE_RUNNING, ad_tun_get_state());

    EXPECT_EQ(AD_TUN_OK, ad_tun_stop());
    EXPECT_EQ(AD_TUN_OK, ad_tun_cleanup());
}

TEST(StateTest, DoubleCleanupIsIdempotent) {
    ad_tun_config_t cfg = {
        .ifname="tun_double_cleanup",
        .ipv4="10.110.0.2",
        .ipv6=NULL,
        .mtu=1500,
        .persist=0
    };

    EXPECT_EQ(AD_TUN_OK, ad_tun_init(&cfg));
    EXPECT_EQ(AD_TUN_OK, ad_tun_cleanup());
    // second cleanup should still succeed and keep state uninitialized
    EXPECT_EQ(AD_TUN_OK, ad_tun_cleanup());
    EXPECT_EQ(AD_TUN_STATE_UNINITIALIZED, ad_tun_get_state());
}