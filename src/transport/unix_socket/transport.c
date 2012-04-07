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
 */

/*! @{ */

#define TRANS_UNIX_DISCONNECTED  (-1)

#define SOCK_ADDR(trans) ((struct sockaddr*)((&trans->sockaddr)))

#define SOCK_ADDR_SIZE(trans) (trans->sockaddr_size)

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
trans_unix_new(Wg_transport *trans, const wg_char *address)
{
    CHECK_FOR_NULL(trans);
    CHECK_FOR_NULL(address);

    /* check size of address */
    if (strlen(address) >= UNIX_PATH_MAX){
        WG_LOG("Unix socket address too long\n");
        return WG_FAILURE;
    }

    memset(trans, '\0', sizeof (Wg_transport));

    trans->transport.out_fd = TRANS_UNIX_DISCONNECTED;
    trans->transport.domain = PF_UNIX;
    trans->sockaddr.un.sun_family = AF_UNIX;
    trans->sockaddr_size = sizeof (trans->sockaddr.un);

    /* we are sure that size of trans->address is correct
     * cause was checked in trand_un_new()
     */
    strcpy((char*)&trans->sockaddr.un.sun_path, address);

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
trans_unix_connect(Wg_transport *trans)
{
    int status = 0;
    int sfd = TRANS_UNIX_DISCONNECTED;

    CHECK_FOR_NULL(trans);

    /* create a new un socket */
    errno = 0;
    sfd = socket(trans->transport.domain, SOCK_STREAM, 0);
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
 * @param trans  transport
 * @param buffer data fuffer to send
 * @param size   size of the buffer
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
trans_unix_send(Wg_transport *transport, void *buf, wg_size size)
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
    while ((size != 0) && (written = write(trans->out_fd, buffer, size)) != 0){
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

wg_status
trans_unix_print(Wg_transport *transport, const char *format, ...)
{
    wg_status status = WG_FAILURE;
    int len = 0;
    char *text = NULL;
    va_list arg_list;

    CHECK_FOR_NULL_PARAM(transport);
    CHECK_FOR_NULL_PARAM(format);

    va_start(arg_list, format);

    len = vsnprintf(NULL, 0, format,  arg_list);

    text = WG_MALLOC(len);
    if (NULL == text){
        return WG_FAILURE;
    }

    vsprintf(text, format,  arg_list);

    va_end(arg_list);

    status = trans_unix_send(transport, text, len - 1);

    WG_FREE(text);

    return status;
}

/**
 * @brief Disconnect from a transport
 *
 * @param trans transport to disconnect from
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
trans_unix_disconnect(Wg_transport *transport)
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


/**
 * @brief Close transport
 *
 * @param trans transport to close
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
trans_unix_close(Wg_transport *transport)
{
    CHECK_FOR_NULL(transport);

    trans_unix_disconnect(transport);

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
trans_unix_get_address(Wg_transport *trans, const wg_char** address)
{
    CHECK_FOR_NULL(trans);
    CHECK_FOR_NULL(address);

    *address = (wg_char*)&trans->sockaddr.un.sun_path;

    return WG_SUCCESS;

}

/*! @} */
