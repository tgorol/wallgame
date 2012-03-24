#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_linked_list.h>

#include <ut_tools.h>

#include "include/wg_config.h"

#define TEST_FILENAME "test.conf"

#define TEST_FILENAME_FULL "test_full.conf"

UT_DEFINE(test_config_init)
    Wg_config config;
    char value[80];

    UT_PASS_ON(wg_config_init(TEST_FILENAME, &config) == WG_SUCCESS);

    UT_PASS_ON(strcmp(config.filename, TEST_FILENAME) == 0);

    UT_PASS_ON(wg_config_add_value(&config, "key1", "value1") == WG_SUCCESS);

    UT_PASS_ON(wg_config_get_value(&config, "key1", value, ELEMNUM(value))
            == WG_SUCCESS);

    printf("%s\n", value);

    wg_config_cleanup(&config);
UT_END

UT_DEFINE(test_config_init_1)
    Wg_config config;
    char value[80];

    wg_config_init(TEST_FILENAME_FULL, &config);

    UT_PASS_ON(wg_config_get_value(&config, "key1", value, ELEMNUM(value))
            == WG_SUCCESS);

    printf("%s\n", value);

    UT_PASS_ON(wg_config_get_value(&config, "key2", value, ELEMNUM(value))
            == WG_SUCCESS);

    printf("%s\n", value);

    UT_PASS_ON(wg_config_get_value(&config, "key3", value, ELEMNUM(value))
            == WG_SUCCESS);

    printf("%s\n", value);

    wg_config_cleanup(&config);

UT_END

int
main(int argc, char *argv[])
{
    UT_RUN_TEST(test_config_init);

    UT_RUN_TEST(test_config_init_1);

    return EXIT_SUCCESS;
}

