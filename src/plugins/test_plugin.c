#include <string.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>

#include <wgp.h>


wg_int
read(wg_char *buffer, wg_int **readed, wg_size size)
{
    return 0;
}

wg_status
init(Wgp_info *info){
     strncpy(info->name, "dummy plugin", MAX_PLUGIN_NAME_SIZE);
     info->version = 1L;
     strncpy(info->description, "Dummy plugin description", MAX_PLUGIN_DESC_SIZE);

     return WG_SUCCESS;
}
