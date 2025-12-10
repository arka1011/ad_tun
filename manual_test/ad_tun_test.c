#include "../include/ad_tun.h"
#include "../include/ad_tun_helper.h"
#include <stdio.h>

int main (void) {
    ad_tun_config_t config;
    ad_tun_error_t err = ad_tun_load_config("manual_test_config.ini", &config);

    if (err != AD_TUN_OK) {
        fprintf(stderr, "Failed to load config: %s\n", ad_tun_strerror(err));
        return 1;
    }

    while (1) {
        printf("Enter choice:\n1. Init\n2. Start\n3. Restart\n4. Stop\n5. Cleanup\n6. Exit\n");
        int input = 0;
        scanf("%d", &input);
        switch (input)
        {
        case 1:
            ad_tun_init(&config);
            break;
        case 2:
            ad_tun_start();
            break;
        case 3:
            ad_tun_restart();
            break;
        case 4:
            ad_tun_stop();
            break;
        case 5:
            ad_tun_cleanup();
            break;
        case 6:
            ad_tun_stop();
            ad_tun_cleanup();
            return 0;
        default:
            break;
        }
    }

    return 0;
}