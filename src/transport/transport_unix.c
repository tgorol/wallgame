#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <alloca.h>

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

/*! \defgroup  un_transport Unix Socket Transport
 *  \ingroup transport
 */

/*! @{ */

#define TRANS_UNIX_DISCONNECTED  (-1)

/**
 * @brief Create a un transport
 *
 * @param trans    memory to store a transport
 * @param address  address of the un socket
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
transport_unix_new(Wg_transport *trans, const wg_char *address)
{
    CHECK_FOR_NULL(trans);
    CHECK_FOR_NULL(address);

    /* check size of address */
    if (strlen(address) >= UNIX_PATH_MAX){
        WG_LOG("Unix socket address too long\n");
        return WG_FAILURE;
    }

    memset(trans, '\0', sizeof (Wg_transport));

    trans->transport.out_fd   = TRANS_UNIX_DISCONNECTED;
    trans->transport.domain   = AF_UNIX;
    trans->transport.type     = SOCK_STREAM;
    trans->transport.protocol = 0;

    trans->sockaddr.un.sun_family = AF_UNIX;
    trans->sockaddr_size = sizeof (trans->sockaddr.un);

    /* we are sure that size of trans->address is correct
     * cause was checked in trand_un_new()
     */
    strcpy((char*)&trans->sockaddr.un.sun_path, address);

    return WG_SUCCESS;
}

/*! @} */
