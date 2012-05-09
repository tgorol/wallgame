#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/types.h>
#include <unistd.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>

#include <wg_trans.h>
#include <wg_string.h>

#include <wg_plugin_tools.h>

/*! \defgroup plugin_tools Plugin Tools
 */

/*! \defgroup msg_transport Event message transport
 * \ingroup plugin_tools
 */

/*! @{ */

#define TIME_STR_SIZE  26

WG_PRIVATE wg_status
get_event_time(wg_char **time);

WG_PRIVATE wg_char hit_format[] =  
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<event>\n"
        "<time>%s</time>\n"
        "<hit>\n"
            "<x>%.4lf</x>\n"
            "<y>%.4lf</y>\n"
        "</hit>\n"
    "</event>\n"
    ;

/** 
* @brief Initialize Message Wg_transport
* 
* @param address  address to bind transport to
* @param msg      message transport instance
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
wg_msg_transport_init(wg_char *address, Wg_msg_transport *msg)
{
    wg_status status = WG_FAILURE;

    CHECK_FOR_NULL_PARAM(address);
    CHECK_FOR_NULL_PARAM(msg);

    status = transport_init(&msg->transport, address);
    if (WG_SUCCESS != status){
        WG_ERROR("Could not create message transport\n");
        return WG_FAILURE;
    }

    return WG_SUCCESS;
}

/** 
* @brief Send 'Hit' message
* 
* @param msg  message transport instance
* @param x    x coordinate of the event
* @param y    y coordinate of the event
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
wg_msg_transport_send_hit(Wg_msg_transport *msg, wg_double x, wg_double y)
{
    Wg_transport *trans = NULL;
    wg_status status = WG_FAILURE;
    wg_char *time_str = NULL;

    CHECK_FOR_NULL_PARAM(msg);

    trans = &msg->transport;

    /* connect transport */
    status = transport_connect(trans);
    if (WG_SUCCESS != status){
        return status;
    }

    status = get_event_time(&time_str);
    if (WG_SUCCESS != status){
        return status;
    }

    /* send message */
    status = transport_print(trans, hit_format, time_str, x, y);
    if (WG_SUCCESS != status){
        transport_disconnect(trans);
        return status;
    }

    /* disconnect transport */
    status = transport_disconnect(trans);

    WG_FREE(time_str);

    return status;
}

/** 
* @brief Release resources allocated by wg_msg_transport()
* 
* @param msg message transport instance
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
wg_msg_transport_cleanup(Wg_msg_transport *msg)
{
    CHECK_FOR_NULL_PARAM(msg);

    transport_close(&msg->transport);

    return WG_SUCCESS;
}

WG_PRIVATE void
chomp(wg_char *text)
{
    size_t len = 0;
    char c = '\0';

    len = strlen(text);

    c = text[--len];

    if ((c == '\n') || (c == '\r')){
        text[len] = '\0';
    }

    c = text[--len];

    if ((c == '\n') || (c == '\r')){
        text[len] = '\0';
    }

    return;
}

WG_PRIVATE wg_status
get_event_time(wg_char **time_str)
{
    time_t t;
    wg_char *l_time = NULL;

    time(&t);

    l_time = WG_MALLOC(TIME_STR_SIZE);
    if (NULL == l_time){
        return WG_FAILURE;
    }

    if (ctime_r(&t, l_time) == NULL){
        return WG_FAILURE;
    }

    chomp(l_time);

    *time_str = l_time;

    return WG_SUCCESS;
}

/*! @} */
