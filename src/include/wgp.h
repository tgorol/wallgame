#ifndef _WGP_H
#define _WGP_H

#define MAX_PLUGIN_NAME_SIZE 32
#define MAX_PLUGIN_DESC_SIZE 512


typedef struct Wgp_info{
     wg_char name[MAX_PLUGIN_NAME_SIZE];
     wg_long version;
     wg_char description[MAX_PLUGIN_DESC_SIZE];
}Wgp_info;

typedef struct Wgp_plugin{
    Wgp_info info;
    void *lib;
    wg_int (*read)(wg_char *buffer, wg_int **readed, wg_size size);
    wg_status (*init)(Wgp_info*);
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

