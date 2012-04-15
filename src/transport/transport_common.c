#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <stdarg.h>

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
#include <wg_trans.h>

#include <wg_string.h>
#include "include/transport_common.h"

/**
*  \defgroup transport Transport 
*/

/** @{ */
/** @brief Transport disconnected value */
#define TRANS_UNIX_DISCONNECTED  (-1)


/*! @brief Address regexp expression */
#define ADDR_EXPR  "^\\(unix\\|inet\\)[: ]\\([^$]\\+\\)$"

/** @brief Get pointer to sockaddr from transport */
#define SOCK_ADDR(trans) ((struct sockaddr*)((&trans->sockaddr)))

/** @brief Get size of sockaddr */
#define SOCK_ADDR_SIZE(trans) (trans->sockaddr_size)

/** 
* @brief Groups in ADDR_EXPR
*/
enum {
    GROUP_ALL     = 0 ,   /*!< full string      */
    GROUP_TYPE        ,   /*!< type group       */
    GROUP_ADDRESS     ,   /*!< addrss group     */
    GROUP_NUM             /*!< number of groups */
};


/** 
* @brief Match address string
* 
* @param[in]  address  input address
* @param[out] type    type of transport 
* @param[in]  addr         transport address
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
transport_match_address(const wg_char *address, wg_char **type, wg_char **addr)
{
    regex_t addr_expr;
    int reg_status = 1;
    wg_status status = WG_FAILURE;
    regmatch_t match_list[GROUP_NUM];
    regmatch_t *match_tmp = NULL;
    wg_char *tmp_1 = NULL;
    wg_char *tmp_2 = NULL;
    wg_int len = 0;

    CHECK_FOR_NULL_PARAM(type);
    CHECK_FOR_NULL_PARAM(addr);

    reg_status = regcomp(&addr_expr, ADDR_EXPR, REG_ICASE);
    if (0 != reg_status){
        return WG_FAILURE;
    }

    reg_status = regexec(&addr_expr, address, ELEMNUM(match_list),
            match_list, 0);
    if (0 != reg_status){
        regfree(&addr_expr);
        return WG_FAILURE;
    }
    regfree(&addr_expr);

    match_tmp = &match_list[GROUP_TYPE];
    len = match_tmp->rm_eo - match_tmp->rm_so;
    status = wg_strndup(&tmp_1, address + match_tmp->rm_so, len);
    if (WG_SUCCESS != status){
        return WG_FAILURE;
    }

    match_tmp = &match_list[GROUP_ADDRESS];
    len = match_tmp->rm_eo - match_tmp->rm_so;
    status = wg_strndup(&tmp_2, address + match_tmp->rm_so, len);
    if (WG_SUCCESS != status){
        WG_FREE(tmp_1);
        return WG_FAILURE;
    }

    *type = tmp_1;
    *addr = tmp_2;

    return WG_SUCCESS;
}

/** 
* @brief Initialize transport
* 
* @param transports  Array of supported transports
* @param num         number of elements in transports
* @param type        transport type
* @param address     transport address
* @param trans       memory to store transport
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
transport_initialize(const Transport_init *transports, wg_size num, 
        const wg_char *type, const wg_char *address, Wg_transport *trans)
{
    wg_int i = 0;
    wg_status status = WG_FAILURE;
    const Transport_init *info = NULL;

    for (i = 0; i < num; ++i){
        info = &transports[i];
        if (strcmp(type, info->name) == 0){
            status = info->init(trans, address);
            if (WG_SUCCESS != status){
                WG_LOG("Could not initialize \"%s\" transport\n", info->name);
            }
            wg_strdup(address, &trans->transport.address);
            break;
        }
    }

    return status;
}

/**
 * @brief Close transport
 *
 * @param transport transport to close
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
transport_close(Wg_transport *transport)
{
    CHECK_FOR_NULL(transport);

    transport_disconnect(transport);

    WG_FREE(transport->transport.address);

    memset(transport, '\0', sizeof (Wg_transport));

    return WG_SUCCESS;
}

/**
 * @brief Get transport address
 *
 * @param trans   transport
 * @param address memory to store a pointer to the address
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
transport_get_address(Wg_transport *trans, const wg_char** address)
{
    CHECK_FOR_NULL(trans);
    CHECK_FOR_NULL(address);

    *address = trans->transport.address;

    return WG_SUCCESS;

}

/**
 * @brief Connect
 *
 * @param trans transport 
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
transport_connect(Wg_transport *trans)
{
    int status = 0;
    int sfd = TRANS_UNIX_DISCONNECTED;

    CHECK_FOR_NULL(trans);

    /* create a new un socket */
    errno = 0;
    sfd = socket(trans->transport.domain, trans->transport.type, 
        trans->transport.protocol);
    if (-1 == sfd){
        WG_LOG("%s\n", strerror(errno));
        return WG_FAILURE;
    }

    trans->transport.out_fd = sfd;

    WG_DEBUG("Unix socket created : %d\n", sfd);

    status = connect(trans->transport.out_fd, SOCK_ADDR(trans),
            SOCK_ADDR_SIZE(trans));
    if (-1 == status){
        WG_LOG("%s\n",strerror(errno));
        return WG_FAILURE;
    }

    WG_DEBUG("Socket connected.\n");

    trans->transport.is_connected = WG_TRUE;

    return WG_SUCCESS;
}

/**
 * @brief Send data 
 *
 * @param transport  transport instance
 * @param buf data fuffer to send
 * @param size   size of the buffer
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
transport_send(Wg_transport *transport, void *buf, wg_size size)
{
    int written = 0;
    Transport *trans = NULL;
    wg_uchar *buffer = NULL;

    CHECK_FOR_NULL(transport);
    CHECK_FOR_NULL(buf);

    buffer = buf;
    trans = &transport->transport;

    /* TODO Add input parameter to store an error code */
    if (trans->out_fd == TRANS_UNIX_DISCONNECTED){
        return WG_FAILURE;
    }

    errno = 0;
    while ((size != 0) && 
            (written = sendto(trans->out_fd, buffer, size, 0, NULL, 0)) != 0){
        if (written == -1){
            if (errno == EINTR){
                continue;
            }
            WG_ERROR("%s\n", strerror(errno));
            return WG_FAILURE;
        }
        size   -= written;
        buffer += written;

        WG_DEBUG("Unix socket written %d bytes\n", written);
    }

    return WG_SUCCESS;
}

/** 
* @brief Read data
* 
* @param transport transport instance
* @param buf       buffer for read data
* @param size      size of buffer
* 
* @return number of read bytes or -1 for failure and errno set
*/
wg_size
transport_receive(Wg_transport *transport, void *buf, wg_size size)
{
    int readed = 0;
    Transport *trans = NULL;
    wg_uchar *buffer = NULL;

    CHECK_FOR_NULL(transport);
    CHECK_FOR_NULL(buf);

    buffer = buf;
    trans = &transport->transport;

    /* TODO Add input parameter to store an error code */
    if (trans->out_fd == TRANS_UNIX_DISCONNECTED){
        return WG_FAILURE;
    }

    errno = 0;
    readed = recv(trans->out_fd, buffer, size, 0);
    if (readed != -1){
        WG_DEBUG("Unix socket read %d bytes\n", readed);
    }

    return readed;
}

/** 
* @brief Send formated data through transport
* 
* @param transport  transport instance
* @param format     format string
* @param ...        parameters
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
transport_print(Wg_transport *transport, const char *format, ...)
{
    wg_status status = WG_FAILURE;
    int len = 0;
    char *text = NULL;
    va_list arg_list;

    CHECK_FOR_NULL_PARAM(transport);
    CHECK_FOR_NULL_PARAM(format);

    va_start(arg_list, format);

    len = vsnprintf(NULL, 0, format, arg_list);

    text = WG_MALLOC(len + 1);
    if (NULL == text){
        return WG_FAILURE;
    }

    vsprintf(text, format, arg_list);

    va_end(arg_list);

    status = transport_send(transport, text, len - 1);

    WG_FREE(text);

    return status;
}

/**
 * @brief Disconnect from a transport
 *
 * @param transport transport to disconnect from
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
transport_disconnect(Wg_transport *transport)
{
    Transport *trans = NULL;

    CHECK_FOR_NULL(transport);

    trans = &transport->transport;

    if (trans->out_fd != TRANS_UNIX_DISCONNECTED){
        close(trans->out_fd);
        trans->out_fd = TRANS_UNIX_DISCONNECTED;
    }

    return WG_SUCCESS;
}


/** @} */
