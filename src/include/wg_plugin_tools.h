#ifndef _PLUGINS_TOOLS_H
#define _PLUGINS_TOOLS_H

/** 
* @brief Message Transport
*/
typedef struct Wg_msg_transport{
    Wg_transport transport;        /*!< socket transport instance */
}Wg_msg_transport;

WG_PUBLIC wg_status
wg_msg_transport_init(wg_char *address, Wg_msg_transport *msg);

WG_PUBLIC wg_status
wg_msg_transport_send_hit(Wg_msg_transport *msg, wg_double x, wg_double y);

WG_PUBLIC wg_status
wg_msg_transport_cleanup(Wg_msg_transport *msg);

#endif
