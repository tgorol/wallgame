#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
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

#include "include/transport_common.h"
#include "include/transport_unix.h"
#include "include/transport_inet.h"

/*! \defgroup  transport_server Transport server
 *  \ingroup transport
 */

/** @{ */

/** 
* @brief Supported transport servers
*/
WG_PRIVATE Transport_init transports[] = {
    {"inet", transport_inet_new}
};

/** 
* @brief Initialize server
* 
* @param transport memory to store transport instance
* @param address   address
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
transport_server_init(Wg_transport *transport, const wg_char *address)
{
    wg_char *serv_address;
    wg_char *type;
    wg_status status = WG_FAILURE;
    Transport *t = NULL;
    int sock_status = 0;

    CHECK_FOR_NULL_PARAM(transport);
    CHECK_FOR_NULL_PARAM(address);

    t = &transport->transport;

    status = transport_match_address(address, &type, &serv_address);
    if (WG_SUCCESS != status){
        return WG_FAILURE;
    }

    status = transport_initialize(transports, ELEMNUM(transports),
        type, serv_address, transport);
    if (status != WG_SUCCESS){
        WG_FREE(type);
        WG_FREE(serv_address);
        return WG_FAILURE;
    }

    WG_FREE(type);
    WG_FREE(serv_address);

    sock_status = socket(t->domain, t->type, t->protocol);
    if (-1 == sock_status){
        WG_LOG("%s\n", strerror(errno));
        return WG_FAILURE;
    }
    t->out_fd = sock_status;

    sock_status = bind(t->out_fd, (struct sockaddr*)&transport->sockaddr,
            transport->sockaddr_size);
    if (0 != sock_status){
        close(t->out_fd);
        WG_LOG("%s:%s\n", address, strerror(errno));
        return WG_FAILURE;
    }

    return WG_SUCCESS;
}

/** 
* @brief Mark transport as server
* 
* @param server  server instance
* @param num     size of queue
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
transport_server_listen(Wg_transport *server, wg_size num)
{
    int sock_status = -1;
    CHECK_FOR_NULL_PARAM(server);

    sock_status = listen(server->transport.out_fd, num);
    if (-1 == sock_status){
        WG_LOG("%s\n", strerror(errno));
        return WG_FAILURE;
    }

    return WG_SUCCESS;
}

/** 
* @brief Accept packets
* 
* @param server      server connection
* @param transport   new connected transport
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
transport_server_accept(Wg_transport *server,
        Wg_transport *transport)
{
    int sock_status = -1;

    CHECK_FOR_NULL_PARAM(server);
    CHECK_FOR_NULL_PARAM(transport);

    sock_status = accept(server->transport.out_fd, 
            (struct sockaddr*)&transport->sockaddr, &server->sockaddr_size);
    if (-1 == sock_status){
        WG_LOG("%s\n", strerror(errno));
        return WG_FAILURE;
    }

    memset(transport, '\0', sizeof (Wg_transport));

    transport->sockaddr_size      = server->sockaddr_size;

    transport->transport.out_fd   = sock_status;
    transport->transport.domain   = server->transport.domain;
    transport->transport.type     = server->transport.type;
    transport->transport.protocol = server->transport.protocol;
    transport->transport.is_connected = WG_TRUE;

    wg_strdup(server->transport.address, &transport->transport.address);

    return WG_SUCCESS;
}

/** @} */
