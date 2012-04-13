#ifndef _TRANS_H
#define _TRANS_H

/**
 * @brief Unit socket transport structure
 */
typedef struct Transport{
    int out_fd;                 /*!< socket file descriptor */
    wg_boolean is_connected;    /*!< connection status      */
    int domain;                 /*!< transport domain       */
    int type;                   /*!< type of socket         */
    int protocol;               /*!< protocol to use        */
    wg_char *address;
}Transport;


typedef struct  Wg_transport{
    Transport transport;
    size_t sockaddr_size;
    union{
        struct sockaddr_un un;
        struct sockaddr_in in;
    }sockaddr;
}Wg_transport;

WG_PUBLIC wg_status
transport_init(Wg_transport *trans, const wg_char *address);

WG_PUBLIC wg_status
transport_connect(Wg_transport *trans);

WG_PUBLIC wg_status
transport_send(Wg_transport *trans, void *buffer, wg_size size);

WG_PUBLIC wg_size
transport_receive(Wg_transport *transport, void *buf, wg_size size);

WG_PUBLIC wg_status
transport_print(Wg_transport *trans, const char *format, ...);

WG_PUBLIC wg_status
transport_close(Wg_transport *trans);

WG_PUBLIC wg_status
transport_disconnect(Wg_transport *trans);

WG_PUBLIC wg_status
transport_get_address(Wg_transport *trans, const wg_char** address);

WG_PUBLIC wg_status
transport_server_init(Wg_transport *transport, const wg_char *address);

WG_PUBLIC wg_status
transport_server_listen(Wg_transport *server, wg_size num);

WG_PUBLIC wg_status
transport_server_accept(Wg_transport *server,
        Wg_transport *transport);

#endif

