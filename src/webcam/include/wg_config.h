#ifndef _WG_CONFIG_H
#define _WG_CONFIG_H

/** 
* @brief Config file structure
*/
typedef struct Wg_config{
    List_head lines;         /*!< list of lines    */
    wg_char *filename;       /*!< config file name */
}Wg_config;

WG_PUBLIC wg_status
wg_config_init(const wg_char *filename, Wg_config *config);

WG_PUBLIC void
wg_config_cleanup(Wg_config *config);

WG_PUBLIC wg_status
wg_config_get_value(Wg_config *config, const wg_char *key, wg_char *value,
        wg_size size);

WG_PUBLIC wg_status
wg_config_add_value(Wg_config *config, const wg_char *key, wg_char *value);

WG_PUBLIC wg_status
wg_config_sync(Wg_config *config);

#endif
