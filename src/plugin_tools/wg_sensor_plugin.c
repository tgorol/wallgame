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
#include <wg_msg.h>
#include <wgp.h>

#include "include/wg_sensor_plugin.h"

/*! \defgroup plugin Sensor Plugin Interface
 * \ingroup plugin_tools
 */

/*! @{ */

WG_PRIVATE const wg_char * const func[] = {
    [WGP_INIT] = "init"    ,
    [WGP_RUN]  = "run"
}; 


WG_PRIVATE wg_status
get_function_address(void *lib, const wg_char *func_name, void **address);

WG_PRIVATE wg_status
fill_functions(Wgp_plugin *plugin);

/**
 * @brief Load plugin.
 *
 * @param name   name of the plugin
 * @param[out] plugin memory to store plugin instance   
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
wgp_load(const wg_char *name, Wgp_plugin *plugin)
{
    void *lib = NULL;
    wg_status status = WG_FAILURE;

    CHECK_FOR_NULL_PARAM(name);
    CHECK_FOR_NULL_PARAM(plugin);

    WG_ZERO_STRUCT(plugin);

    /* open a plugin                        */
    lib = dlopen(name, RTLD_LAZY);
    if (NULL == lib){
        WG_LOG("%s\n", dlerror());
        return WG_FAILURE; 
    }

    plugin->lib = lib;
   
    do {
        status = fill_functions(plugin);
        if(WG_FAILURE == status) { break;}
        /* initialize plugin                           */
        status = WGP_CALL_INIT(plugin, &plugin->info);
    }while (0);
    /* If error in above loop exit with error */
    if (WG_FAILURE == status){
        dlclose(lib);
        return WG_FAILURE;
    }

    return WG_SUCCESS;
}


/**
 * @brief Get plugin info
 *
 * @param plugin
 * @param info   memory to store apointer to plugin info
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
wgp_info(Wgp_plugin *plugin, const Wgp_info **info)
{
    CHECK_FOR_NULL_PARAM(plugin);

    *info = &(plugin->info);

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
    CHECK_FOR_NULL_PARAM(plugin);

    dlclose(plugin->lib);

    WG_ZERO_STRUCT(plugin);

    return WG_SUCCESS;
}

WG_STATIC wg_status
fill_functions(Wgp_plugin *plugin)
{
    wg_int i = 0;
    wg_status status = WG_SUCCESS;

    CHECK_FOR_NULL_PARAM(plugin);

    /* loop through all functions and store their address */
    for (i = 0; ((i < ELEMNUM(func)) && (status == WG_SUCCESS)); ++i){
        status = get_function_address(plugin->lib, func[i], 
        (void**)&plugin->f[i]);
    }

    return status;
}

/** @brief Get function address from the library
 *
 * @param lib   library handle
 * @param[in] func_name name of the function
 * @param[out] address memmory to store address of the function
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
WG_STATIC wg_status
get_function_address(void *lib, const wg_char *func_name, void **address)
{
    void *func_addr = NULL;

    CHECK_FOR_NULL_PARAM(lib);
    CHECK_FOR_NULL_PARAM(func_name);
    CHECK_FOR_NULL_PARAM(address);

    func_addr = dlsym(lib, func_name);
    CHECK_FOR_NULL(func_addr);

    *address = func_addr;

    WG_DEBUG("Plugin: function %s at %p\n", func_name, func_addr);

    return WG_SUCCESS;
}

/*! @} */
