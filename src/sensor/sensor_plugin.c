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

#include "include/sensor_plugin.h"

/*! \defgroup Plugin Sensor Plugin Interface
 */

/*! @{ */


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
wgp_load(const wg_char *name, Wgp_plugin *plugin)
{
    void *lib = NULL;
    wg_status status = WG_FAILURE;

    CHECK_FOR_NULL_PARAM(name);
    CHECK_FOR_NULL_PARAM(plugin);

    /* open plugin                        */
    lib = dlopen(name, RTLD_LAZY);
    if (NULL == lib){
        return WG_FAILURE; 
    }

    plugin->lib = lib;
   
    do {
        /* get pointer to 'init' function              */
        status = get_function_address(plugin->lib, "init", 
                (void**)&plugin->init);
        if (WG_FAILURE == status){break;}

        /* initialize plugin                           */
        status = plugin->init(&plugin->info);
        if (WG_FAILURE == status){break;}

        /* get pointer to 'read' function              */
        status = get_function_address(plugin->lib, "run", 
                (void**)&plugin->run);

        if (WG_FAILURE == status){break;}
    }while (0);
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

    memset(plugin, '\0', sizeof (Wgp_plugin));

    return WG_SUCCESS;
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

/*! @} */
