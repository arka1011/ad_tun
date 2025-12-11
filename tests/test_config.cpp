#include "../../../prebuilt/googletest/googletest/include/gtest/gtest.h"

extern "C" {
#include "../include/ad_tun.h"
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

    ASSERT_EQ(AD_TUN_OK,
              ad_tun_load_config("../../test_configs/missing_ipv6MtuPersist.ini", &cfg));

    EXPECT_STREQ(cfg.ifname, "ad_tun0");
    EXPECT_STREQ(cfg.ipv4, "10.10.1.2/24");
    EXPECT_EQ(cfg.mtu, 1500);    // default
    EXPECT_EQ(cfg.persist, 0);   // default
    EXPECT_EQ(cfg.ipv6, (const char*)NULL);

    ad_tun_free_config(&cfg);
}

TEST(ConfigTest, FileNotFound) {
    ad_tun_config_t cfg;

    EXPECT_EQ(AD_TUN_ERR_CONFIG,
              ad_tun_load_config("../../test_configs/this_file_does_not_exist.ini", &cfg));
}

TEST(ConfigTest, InvalidMtuFallsBackToDefault) {
    ad_tun_config_t cfg;

    ASSERT_EQ(AD_TUN_OK,
              ad_tun_load_config("../../test_configs/invalid_mtu.ini", &cfg));

    EXPECT_EQ(cfg.mtu, 1500); // DEFAULT_MTU

    ad_tun_free_config(&cfg);
}

TEST(ConfigTest, InvalidPersistFallsBackToDefault) {
    ad_tun_config_t cfg;

    ASSERT_EQ(AD_TUN_OK,
              ad_tun_load_config("../../test_configs/invalid_persist.ini", &cfg));

    EXPECT_EQ(cfg.persist, 0); // DEFAULT_PERSIST

    ad_tun_free_config(&cfg);
}

TEST(ConfigTest, MalformedIniFails) {
    ad_tun_config_t cfg;

    EXPECT_EQ(AD_TUN_ERR_CONFIG,
              ad_tun_load_config("../../test_configs/malformed.ini", &cfg));
}

TEST(ConfigTest, UnknownKeyIsIgnored) {
    ad_tun_config_t cfg;

    ASSERT_EQ(AD_TUN_OK,
              ad_tun_load_config("../../test_configs/unknown_key.ini", &cfg));

    EXPECT_STREQ(cfg.ifname, "ad_tun0");
    EXPECT_STREQ(cfg.ipv4, "10.10.1.2/24");

    ad_tun_free_config(&cfg);
}

// Additional tests covering edge cases
TEST(ConfigTest, EmptyIfnameValueFails) {
    ad_tun_config_t cfg;

    // ifname is present but empty
    EXPECT_EQ(AD_TUN_ERR_CONFIG,
              ad_tun_load_config("../../test_configs/empty_ifname.ini", &cfg));
}

TEST(ConfigTest, Ipv6EmptyIsAllowed) {
    ad_tun_config_t cfg;

    ASSERT_EQ(AD_TUN_OK,
              ad_tun_load_config("../../test_configs/ipv6_empty.ini", &cfg));

    EXPECT_STREQ(cfg.ifname, "ad_tun0");
    EXPECT_STREQ(cfg.ipv4, "10.10.1.2/24");
    EXPECT_EQ(cfg.ipv6, (const char*)NULL);

    ad_tun_free_config(&cfg);
}

TEST(ConfigTest, MtuTooLargeFallsBackToDefault) {
    ad_tun_config_t cfg;

    ASSERT_EQ(AD_TUN_OK,
              ad_tun_load_config("../../test_configs/mtu_toolarge.ini", &cfg));

    EXPECT_EQ(cfg.mtu, 1500); // DEFAULT_MTU used for overly large MTU

    ad_tun_free_config(&cfg);
}

TEST(ConfigTest, KeysInWrongSectionFails) {
    ad_tun_config_t cfg;

    EXPECT_EQ(AD_TUN_ERR_CONFIG,
              ad_tun_load_config("../../test_configs/wrong_section.ini", &cfg));
}

TEST(ConfigTest, DuplicateIfnameUsesLast) {
    ad_tun_config_t cfg;

    ASSERT_EQ(AD_TUN_OK,
              ad_tun_load_config("../../test_configs/duplicate_keys.ini", &cfg));

    EXPECT_STREQ(cfg.ifname, "ad_tun1");
    EXPECT_STREQ(cfg.ipv4, "10.10.1.2/24");

    ad_tun_free_config(&cfg);
}