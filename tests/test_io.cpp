#include <gtest/gtest.h>

extern "C" {
#include "ad_tun.h"
}

TEST(IOTest, ReadWithoutStartFails) {
    char buf[32];
    EXPECT_LT(ad_tun_read(buf, sizeof(buf)), 0);
}

TEST(IOTest, WriteWithoutStartFails) {
    char buf[32];
    EXPECT_LT(ad_tun_write(buf, sizeof(buf)), 0);
}

TEST(IOTest, ReadWriteWhileRunning) {
    ad_tun_config_t cfg = {
        .ifname = "test_io0",
        .ipv4 = "10.200.0.2",
        .ipv6 = NULL,
        .mtu = 1500,
        .persist = 0
    };

    ad_tun_error_t rc = ad_tun_init(&cfg);
    if (rc != AD_TUN_OK) {
        ad_tun_cleanup();
        GTEST_SKIP() << "Skipping: ad_tun_init failed";
    }

    ad_tun_error_t start_rc = ad_tun_start();
    if (start_rc != AD_TUN_OK) {
        ad_tun_cleanup();
        GTEST_SKIP() << "Skipping: ad_tun_start failed (device may be unavailable)";
    }

    EXPECT_EQ(AD_TUN_STATE_RUNNING, ad_tun_get_state());

    char wbuf[64] = {0};
    ssize_t wn = ad_tun_write(wbuf, sizeof(wbuf));
    EXPECT_TRUE(wn >= 0 || wn == -EAGAIN || wn == -EIO);

    char rbuf[64];
    ssize_t rn = ad_tun_read(rbuf, sizeof(rbuf));
    EXPECT_TRUE(rn >= 0 || rn == -EAGAIN || rn == -EIO);

    EXPECT_EQ(AD_TUN_OK, ad_tun_stop());
    EXPECT_EQ(AD_TUN_OK, ad_tun_cleanup());
}
