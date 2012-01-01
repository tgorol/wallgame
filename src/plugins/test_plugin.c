#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>

#include <wg_msg.h>
#include <wgp.h>

#define LOOP_NUM 1000

WG_PRIVATE void get_random_coordinate(float *coord);

wg_status
init(Wgp_info *info){
     strncpy(info->name, "dummy plugin", MAX_PLUGIN_NAME_SIZE);
     info->version = 1L;
     strncpy(info->description, "Dummy plugin description", MAX_PLUGIN_DESC_SIZE);

     return WG_SUCCESS;
}

wg_int
run(void *gh, Msg_handler handler) 
{
    Wg_message msg;
    int i = 0;

    msg.type = MSG_XY;

    srand(time(NULL));

    for (i = 0; i < LOOP_NUM; ++i){
        get_random_coordinate(&msg.value.point.x);
        get_random_coordinate(&msg.value.point.y);
        handler(gh, &msg);
    }

    return 0;
}


WG_PRIVATE void
get_random_coordinate(float *coord)
{
    *coord = ((float)rand() / (float)RAND_MAX) * 100.0f;

    return;
}
