#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_cm.h>
#include <wg_gpm.h>
#include <wg_string.h>
#include <wgp.h>

#include "include/sensor_plugin.h"

WG_STATIC wg_status
get_function_address(void *lib, wg_char *func_name, void **address);

/**
 * @brief Load plugin.
 *
 * @param name
 * @param plugin
 *
 * @return 
 */
wg_status
wgp_load(wg_char *name, Wgp_plugin *plugin)
{
    void *lib = NULL;
    wg_status status = WG_FAILURE;

    lib = dlopen(name, RTLD_LAZY);
    if (NULL == lib){
        return WG_FAILURE; 
    }

    plugin->lib = lib;
   
    do {
        status = get_function_address(plugin->lib, "init", (void**)&plugin->init);
        if (WG_FAILURE == status){break;}

        status = plugin->init(&plugin->info);
        if (WG_FAILURE == status){break;}

        status = get_function_address(plugin->lib, "read", (void**)&plugin->read);
        if (WG_FAILURE == status){break;}
    }while (0);
    if (WG_FAILURE == status){
        dlclose(lib);
        return WG_FAILURE;
    }

    return WG_SUCCESS;
}

/**
 * @brief Unload plugin.
 *
 * @param plugin
 *
 * @return 
 */
wg_status
wgp_unload(Wgp_plugin *plugin)
{
    CHECK_FOR_NULL(plugin);

    dlclose(plugin->lib);

    memset(plugin, '\0', sizeof (Wgp_plugin));

    return WG_SUCCESS;
}

WG_STATIC wg_status
get_function_address(void *lib, wg_char *func_name, void **address)
{
    void *func_addr = NULL;
#ifdef WGDEBUG
    DECLARE_FUNC_STR_BUFFER(buf);
#endif

    CHECK_FOR_NULL_PARAM(lib);
    CHECK_FOR_NULL_PARAM(func_name);
    CHECK_FOR_NULL_PARAM(address);

    func_addr = dlsym(lib, func_name);
    CHECK_FOR_NULL(func_addr);

    *address = func_addr;

#ifdef WGDEBUG
    wg_fptr_2_str((fvoid)func_addr, buf);
#endif

    WG_DEBUG("Plugin: function %s at %s\n", func_name, buf);

    return WG_SUCCESS;
}
