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


wg_status
trans_unix_new(Transport *trans, wg_char *address)
{
    int sfd = 0;
    wg_status status = WG_FAILURE;

    CHECK_FOR_NULL(trans);

    /* check size of address */
    if (strlen(address) >= UNIX_PATH_MAX){
        WG_LOG("Unix socket address too long\n");
        return WG_FAILURE;
    }

    /* create a new unix socket */
    errno = 0;
    sfd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (-1 == sfd){
        WG_LOG("%s\n", strerror(errno));
        return WG_FAILURE;
    }

    WG_DEBUG("Unix socket created : %d\n", sfd);

    memset(trans, '\0', sizeof (Transport));

    /* save address as part of transaction */
    status = wg_strdup(address, &trans->address);
    if (WG_FAILURE == status){
        close(sfd);
        return WG_FAILURE;
    }

    /* save socket as part of transaction */
    trans->out_fd = sfd;

    return status;
}

wg_status
trans_unix_connect(Transport *trans)
{
    int status = 0;
    struct sockaddr_un address;

    CHECK_FOR_NULL(trans);

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

    return WG_SUCCESS;
}

wg_status
trans_unix_send(Transport *trans, wg_uchar *buffer, wg_size size)
{
    int written = 0;

    CHECK_FOR_NULL(trans);
    CHECK_FOR_NULL(buffer);

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
trans_unix_close(Transport *trans)
{
    CHECK_FOR_NULL(trans);

    close(trans->out_fd);

    WG_FREE(trans->address);

    memset(trans, '\0', sizeof (Transport));

    return WG_SUCCESS;
}

