#ifndef _TRANS_H
#define _TRANS_H

/**
 * @brief Unit socket transport structure
 */
typedef struct Transport{
    int out_fd;                 /*!< socket file descriptor */
    wg_char *address;           /*!< address                */
    wg_boolean is_connected;    /*!< connection status      */
}Transport;

WG_PUBLIC wg_status
trans_unix_new(Transport *trans, const wg_char *address);

WG_PUBLIC wg_status
trans_unix_connect(Transport *trans);

WG_PUBLIC wg_status
trans_unix_send(Transport *trans, wg_uchar *buffer, wg_size size);

WG_PUBLIC wg_status
trans_unix_print(Transport *trans, const char *format, ...);

WG_PUBLIC wg_status
trans_unix_close(Transport *trans);

WG_PUBLIC wg_status
trans_unix_disconnect(Transport *trans);

#endif

