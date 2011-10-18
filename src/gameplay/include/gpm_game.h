#ifndef _GPM_GAME_H
#define _GPM_GAME_H

#define MAX_PATH_LENGTH   512

typedef struct Game{
    Transport transport;
    pid_t process_id;
}Game;

WG_PUBLIC wg_status
gpm_game_run(wg_char *argv[], wg_uint address_index, Game *game);

WG_PUBLIC wg_status
gpm_game_send_ln(Game *game, wg_char *line);

WG_PUBLIC wg_status
gpm_game_kill(Game *game);

#endif
