#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_cm.h>

#include "include/cm.h"

/*! \defgroup Ini Ini File Manipulation
 */

/*! @{ */


wg_status
/**
 * @brief Create an instance of the CM
 *
 * @param file_name config file name
 * @param mode      mode to open the config in
 * @param config    data to store a CM instance
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
cm_init(wg_char *file_name, char *mode, Config *config)
{
    ini_fd_t  ini_fd = 0;

    CHECK_FOR_NULL(file_name);
    CHECK_FOR_NULL(config);
    CHECK_FOR_NULL(mode);

    ini_fd = ini_open(file_name, mode, NULL);
    CHECK_FOR_INI_OPEN_ERROR(ini_fd);

    config->ini_fd = ini_fd;

    return WG_SUCCESS;
}

/**
 * @brief Release resources reserved by CM
 *
 * @param config    config file instance
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
cm_cleanup(Config *config)
{
    int status = -1;

    CHECK_FOR_NULL(config);
    status = ini_close(config->ini_fd);
    CHECK_FOR_INI_CLOSE_ERROR(status);

    return WG_SUCCESS;
}

/*! @} */
