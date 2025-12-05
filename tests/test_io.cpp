#include <gtest/gtest.h>

extern "C" {
#include "ad_tun.h"
}

TEST(IOTest, ReadWithoutStartFails) {
    char buf[32];
    EXPECT_LT(ad_tun_read(buf, sizeof(buf)), 0);
}
