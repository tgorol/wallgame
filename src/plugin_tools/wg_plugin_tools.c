#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>

#include <wg_trans.h>

#include <wg_plugin_tools.h>

WG_STATIC wg_char hit_format[] =  
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<event>\n"
        "<hit>\n"
            "<x>%.2lf</x>\n"
            "<y>%.2lf</y>\n"
        "</hit>\n"
    "</event>\n"
    ;

wg_status
wg_msg_transport(wg_char *address, Wg_msg_transport *msg)
{
    wg_status status = WG_FAILURE;

    CHECK_FOR_NULL_PARAM(address);
    CHECK_FOR_NULL_PARAM(msg);

    status = trans_unix_new(&msg->transport, address);
    if (WG_SUCCESS != status){
        WG_ERROR("Could not create message transport\n");
        return WG_FAILURE;
    }

    return WG_SUCCESS;
}

wg_status
wg_msg_transport_send_hit(Wg_msg_transport *msg, wg_double x, wg_double y)
{
    Transport *trans = NULL;
    wg_status status = WG_FAILURE;

    CHECK_FOR_NULL_PARAM(msg);
    CHECK_FOR_NULL_PARAM(text);

    trans = &msg->transport;

    status = trans_unix_connect(trans);
    if (WG_SUCCESS != status){
        return status;
    }

    status = trans_unix_print(trans, hit_format, x, y);
    if (WG_SUCCESS != status){
        trans_unix_disconnect(trans);
        return status;
    }

    status = trans_unix_disconnect(trans);

    return status;
}

wg_status
wg_msg_transport_cleanup(Wg_msg_transport *msg)
{
    CHECK_FOR_NULL_PARAM(msg);

    trans_unix_close(&msg->transport);

    return WG_SUCCESS;
}
