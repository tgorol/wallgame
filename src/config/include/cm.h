#ifndef _CONFIG_MGR_H
#define _CONFIG_MGR_H

#define SERVER_SECTION_NAME   "server"
#define GAMES_SECTION_NAME    "games"

#define GAME_PATH_KEY         "path"
#define GAME_ARGUMENTS_KEY    "arguments"

#define SERVER_PIPE_PATH_KEY  "socket"
#define SERVER_PLAYER_DB_KEY  "playersDBPath"
#define SERVER_SCORE_DB_KEY   "scoreDBPath"

#define GAME_PREFIX           "game"

#define CHECK_FOR_INI_OPEN_ERROR(ini_fd)         \
    if (0 == (ini_fd)){                          \
        WG_ERROR("libini error : " #ini_fd"\n");     \
        return WG_FAILURE;                       \
    }

#define CHECK_FOR_INI_CLOSE_ERROR(ini_fd)        \
    if (-1 == (ini_fd)){                         \
        WG_ERROR("libini error : " #ini_fd"\n");     \
        return WG_FAILURE;                       \
    }

#define CHECK_FOR_INI_ERROR(status)               \
    if (-1 == (status)){                          \
        WG_ERROR("libini error : " #status"\n");      \
        return WG_FAILURE;                        \
    }

#define INI_ERROR  (-1)

#endif
