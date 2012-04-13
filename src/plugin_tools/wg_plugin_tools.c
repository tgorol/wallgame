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

#include <wg_plugin_tools.h>

/*! \defgroup plugin_tools Plugin Tools
 */

/*! \defgroup msg_transport Message Wg_transport
 * \ingroup plugin_tools
 */

/*! @{ */

WG_PRIVATE wg_char hit_format[] =  
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<event>\n"
        "<hit>\n"
            "<x>%.2lf</x>\n"
            "<y>%.2lf</y>\n"
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

    CHECK_FOR_NULL_PARAM(msg);

    trans = &msg->transport;

    /* connect transport */
    status = transport_connect(trans);
    if (WG_SUCCESS != status){
        return status;
    }

    /* send message */
    status = transport_print(trans, hit_format, x, y);
    if (WG_SUCCESS != status){
        transport_disconnect(trans);
        return status;
    }

    /* disconnect transport */
    status = transport_disconnect(trans);

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

/*! @} */
