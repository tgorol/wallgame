#ifndef _GPM_GAME_H
#define _GPM_GAME_H

#define MAX_PATH_LENGTH   512

WG_PUBLIC wg_status
gpm_game_init(void);

WG_PUBLIC wg_status
gpm_game_cleanup(void);

WG_PUBLIC wg_status
gpm_game_set_transport(const wg_char *address);

WG_PUBLIC wg_status
gpm_game_set_server(const wg_char *address);

WG_PUBLIC wg_status
gpm_game_clear_transport(void);

WG_PUBLIC wg_status
gpm_game_clear_server(void);

WG_PUBLIC wg_status
gpm_game_block(void);

WG_PUBLIC wg_status
gpm_game_unblock(void);

WG_PUBLIC wg_status
gpm_get_blocking_state(wg_boolean *state);

#endif
