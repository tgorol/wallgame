#ifndef _TRANS_H
#define _TRANS_H

typedef struct Transport{
    int out_fd;
    wg_char *address;
}Transport;

WG_PUBLIC wg_status
trans_unix_new(Transport *trans, wg_char *address);

wg_status
trans_unix_connect(Transport *trans);

wg_status
trans_unix_send(Transport *trans, wg_uchar *buffer, wg_size size);

wg_status
trans_unix_close(Transport *trans);

wg_status
trans_unix_disconnect(Transport *trans);

#endif

