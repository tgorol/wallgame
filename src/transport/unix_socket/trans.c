#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <linux/un.h>
#include <linux/types.h>
#include <unistd.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_string.h>
#include <wg_trans.h>

/*! \defgroup  unix_transport Unix socket transport
 */

/*! @{ */

#define TRANS_UNIX_DISCONNECTED  (-1)

/**
 * @brief Create a unix transport
 *
 * @param trans    memory to store a transport
 * @param address  address of the unix socket
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
trans_unix_new(Transport *trans, wg_char *address)
{
    wg_status status = WG_FAILURE;

    CHECK_FOR_NULL(trans);

    /* check size of address */
    if (strlen(address) >= UNIX_PATH_MAX){
        WG_LOG("Unix socket address too long\n");
        return WG_FAILURE;
    }

    memset(trans, '\0', sizeof (Transport));

    /* save address as part of transaction */
    status = wg_strdup(address, &trans->address);
    CHECK_FOR_FAILURE(status);

    return status;
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
trans_unix_connect(Transport *trans)
{
    int status = 0;
    int sfd = TRANS_UNIX_DISCONNECTED;
    struct sockaddr_un address;

    CHECK_FOR_NULL(trans);

    /* create a new unix socket */
    errno = 0;
    sfd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (-1 == sfd){
        WG_LOG("%s\n", strerror(errno));
        return WG_FAILURE;
    }

    trans->out_fd = sfd;

    WG_DEBUG("Unix socket created : %d\n", sfd);

    address.sun_family = AF_UNIX;

    /* we are sure that size of trans->address is correct
     * cause was checked in trand_unix_new()
     */
    strcpy(address.sun_path, trans->address);
    status = connect(trans->out_fd, (struct sockaddr*)&address,
            sizeof (address));
    if (-1 == status){
        WG_LOG("%s:%s\n",trans->address, strerror(errno));
        return WG_FAILURE;
    }

    WG_DEBUG("Unix socket connected to %s\n", trans->address);

    trans->is_connected = WG_TRUE;

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
trans_unix_send(Transport *trans, wg_uchar *buffer, wg_size size)
{
    int written = 0;

    CHECK_FOR_NULL(trans);
    CHECK_FOR_NULL(buffer);

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

/**
 * @brief Disconnect from a transport
 *
 * @param trans transport to disconnect from
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
trans_unix_disconnect(Transport *trans)
{
    CHECK_FOR_NULL(trans);

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
trans_unix_close(Transport *trans)
{
    CHECK_FOR_NULL(trans);

    trans_unix_disconnect(trans);

    close(trans->out_fd);

    WG_FREE(trans->address);

    memset(trans, '\0', sizeof (Transport));

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
trans_unix_get_address(Transport *trans, const wg_char** address)
{
    CHECK_FOR_NULL(trans);
    CHECK_FOR_NULL(address);

    *address = trans->address;

    return WG_SUCCESS;

}

/*! @} */
