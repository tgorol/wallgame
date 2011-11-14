#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>

#include <wg_msg.h>
#include <wgp.h>


wg_int
run(void *gh, Msg_handler handler) 
{
    Wg_message msg;
    int i = 0;

    msg.type = MSG_XY;

    srand(time(NULL));


    for (i = 0; i < 10000; ++i){
        msg.value.point.x = ((float)rand() / (float)RAND_MAX) * 100.0f;
        msg.value.point.y = ((float)rand() / (float)RAND_MAX) * 100.0f;
        handler(gh, &msg);
    }

    for (;;){sleep(1);}

    return 0;
}

wg_status
init(Wgp_info *info){
     strncpy(info->name, "dummy plugin", MAX_PLUGIN_NAME_SIZE);
     info->version = 1L;
     strncpy(info->description, "Dummy plugin description", MAX_PLUGIN_DESC_SIZE);

     return WG_SUCCESS;
}
