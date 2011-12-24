#ifndef _WGP_H
#define _WGP_H

#define MAX_PLUGIN_NAME_SIZE 32
#define MAX_PLUGIN_DESC_SIZE 512

typedef wg_status (*Msg_handler)(void *gh, Wg_message *msg);

typedef enum WGP_FUNC_ID {
     WGP_INIT = 0     ,
     WGP_RUN          ,
     WGP_FUNC_ID_NUM
} WGP_FUNC_ID;


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
    fvoid f[WGP_FUNC_ID_NUM];
}Wgp_plugin;

#define WGP_CALL_INIT(p, ...)                          \
    CAST_FUNC(GET_FUNC(p, WGP_INIT) , wg_status, Wgp_info *)(__VA_ARGS__) 

#define WGP_CALL_RUN(p, ...)                           \
    CAST_FUNC(GET_FUNC(p, WGP_RUN) , wg_int, void*, Msg_handler)(__VA_ARGS__) 
   

#define GET_FUNC(p, func_id)                      \
    ((p)->f[func_id])

#define CAST_FUNC(func, ret, ...)                       \
    ((ret (*)(__VA_ARGS__))func)
  

WG_PUBLIC wg_status
wgp_load(const wg_char *name, Wgp_plugin *plugin);

WG_PUBLIC wg_status
wgp_unload(Wgp_plugin *plugin);

WG_PUBLIC wg_int
wgp_read(Wgp_plugin *plugin, wg_char *buffer, wg_int **readed, wg_size size);

WG_PUBLIC wg_status
wgp_info(Wgp_plugin *plugin, const Wgp_info **info);

#endif

