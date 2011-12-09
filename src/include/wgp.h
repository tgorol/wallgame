#ifndef _WGP_H
#define _WGP_H

#define MAX_PLUGIN_NAME_SIZE 32
#define MAX_PLUGIN_DESC_SIZE 512

typedef wg_status (*Msg_handler)(void *gh, Wg_message *msg);


/**
 * @brief Plugin info structure
 */
typedef struct Wgp_info{
     wg_char name[MAX_PLUGIN_NAME_SIZE];        /*!< plugin name        */
     wg_long version;                           /*!< pligin version     */
     wg_char description[MAX_PLUGIN_DESC_SIZE]; /*!< plugin description */
}Wgp_info;

/**
 * @brief Plugin structure
 */
typedef struct Wgp_plugin{
    Wgp_info info;                             /*!< plugin information */
    void *lib;                                 /*!< plugin file handle */
    /* plugin interface */
    wg_int (*run)(void *gh, Msg_handler handler); /*!< run plugin */
    wg_status (*init)(Wgp_info*);              /*!< info */
}Wgp_plugin;

WG_PUBLIC wg_status
wgp_load(const wg_char *name, Wgp_plugin *plugin);

WG_PUBLIC wg_status
wgp_unload(Wgp_plugin *plugin);

WG_PUBLIC wg_int
wgp_read(Wgp_plugin *plugin, wg_char *buffer, wg_int **readed, wg_size size);

WG_PUBLIC wg_status
wgp_info(Wgp_plugin *plugin, const Wgp_info **info);

#endif

