#ifndef _GPM_GAME_H
#define _GPM_GAME_H

#define MAX_PATH_LENGTH   512


WG_PUBLIC wg_status
gpm_game_run(wg_char *argv[], wg_char * address_index, 
        const wg_char *plugin_name);

WG_PUBLIC wg_status
gpm_game_send(wg_uchar *line, wg_size size);

WG_PUBLIC wg_status
gpm_game_kill(void);

WG_PUBLIC wg_boolean
gpm_game_is_running(void);

WG_PUBLIC wg_status
gpm_game_get_id(wg_uint *id);

WG_PUBLIC wg_status
gpm_game_set_id(wg_uint id);

WG_PUBLIC wg_status
gpm_game_add_message(Msg_type type, ...);

#endif
