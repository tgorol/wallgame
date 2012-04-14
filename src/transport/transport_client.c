#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <alloca.h>
#include <regex.h>

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
#include <wg_trans.h>

#include "include/transport_inet.h"
#include "include/transport_unix.h"
#include "include/transport_common.h"

/*! \defgroup  transport_client Transport client
 *  \ingroup transport
 */

/*! @{ */


/** 
* @brief Supported client transports
*/
WG_PRIVATE Transport_init transports[] = {
    {"unix", transport_unix_new}    ,
    {"inet", transport_inet_new}
};

/**
 * @brief Create client transport
 *
 * @param trans    memory to store a transport
 * @param address  address of the un socket
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
transport_init(Wg_transport *trans, const wg_char *address)
{
    wg_char *type    = NULL;
    wg_char *addr = NULL;
    wg_status status = WG_FAILURE;

    CHECK_FOR_NULL_PARAM(trans);
    CHECK_FOR_NULL_PARAM(address);

    status = transport_match_address(address, &type, &addr);
    if (WG_SUCCESS != status){
        return WG_FAILURE;
    }

    status = transport_initialize(transports, ELEMNUM(transports), 
            type, addr, trans);
    if (WG_SUCCESS != status){
        WG_FREE(type);
        WG_FREE(addr);
        return WG_FAILURE;
    }

    WG_FREE(type);
    WG_FREE(addr);

    return WG_SUCCESS;
}

/*! @} */
