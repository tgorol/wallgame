#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/types.h>
#include <unistd.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_string.h>
#include <wg_cm.h>
#include <wg_gpm.h>
#include <wg_trans.h>
#include <wg_linked_list.h>
#include <wg_msg.h>

#include "include/gpm_console.h"
#include "include/gpm_console_parser.h"
#include "include/gpm_ini.h"
#include "include/gpm_game.h"

/*! \defgroup gpm_cb Gameplay Console Callbacks
 * @ingroup gameplay
 */

/*! @{ */

/** 
* @brief Exit callback
* 
* @param argc          number of elements on args
* @param args[]        NULL terinated list of arguments
* @param private_data  user data
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status 
cb_exit(wg_int argc, wg_char *args[], void *private_data)
{
    printf("Bye!\n");

    return WG_SUCCESS;
}

/** 
* @brief Info callback
* 
* @param argc          number of elements on args
* @param args[]        NULL terinated list of arguments
* @param private_data  user data
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
cb_info(wg_int argc, wg_char *args[], void *private_data)
{
    printf("Author  : %s\n"
           "Version : %s\n",
           STR(AUTHOR),
           STR(VERSION)
          );

    return WG_SUCCESS;
}

/** 
* @brief Pause callback
* 
* @param argc          number of elements on args
* @param args[]        NULL terinated list of arguments
* @param private_data  user data
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
cb_pause(wg_int argc, wg_char *args[], void *private_data)
{
    return WG_SUCCESS;
}

/** 
* @brief Connect callback
* 
* @param argc          number of elements on args
* @param args[]        NULL terinated list of arguments
* @param private_data  user data
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
cb_connect(wg_uint argc, wg_char *args[], void *private_data)
{
    wg_status status = WG_FAILURE;

    if (argc != 2){
        WG_LOG("Too few parameters\n");
        return WG_FAILURE;
    }

    status = gpm_game_set_transport(args[1]);
    if (WG_SUCCESS != status){
        WG_LOG("Connection error\n");
        return WG_FAILURE;
    }
  
    WG_LOG("Connected to %s\n", args[1]);

    return status;
}

/** 
* @brief Start callback
* 
* @param argc          number of elements on args
* @param args[]        NULL terinated list of arguments
* @param private_data  user data
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
cb_start(wg_int argc, wg_char *args[], void *private_data)
{
    return WG_FAILURE;
}

/** 
* @brief Stop callback
* 
* @param argc          number of elements on args
* @param args[]        NULL terinated list of arguments
* @param private_data  user data
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
cb_stop(wg_int argc, wg_char *args[], void *private_data)
{
    return WG_FAILURE;
}

/** 
* @brief lsg(list games) callback
* 
* @param argc          number of elements on args
* @param args[]        NULL terinated list of arguments
* @param private_data  user data
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
cb_lsg(wg_int argc, wg_char *argv[], void *private_data)
{
    return WG_FAILURE;
}

/** 
* @brief Send callback
* 
* @param argc          number of elements on args
* @param args[]        NULL terinated list of arguments
* @param private_data  user data
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
cb_send(wg_int argc, wg_char *args[], void *private_data)
{
    return WG_FAILURE;
}

/*! @} */
