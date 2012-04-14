#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/types.h>
#include <unistd.h>
#include <pthread.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_trans.h>
#include <wg_string.h>
#include <wg_linked_list.h>
#include <wg_sync_linked_list.h>
#include <wg_slab.h>
#include <wg_msg.h>
#include <wgp.h>
#include <wg_msgpipe.h>

#include "include/gpm_game.h"
#include "include/gpm_console.h"

/*! \defgroup gpm_game Gameplay Game Control 
 * @ingroup gameplay
 */

/*! @{ */

/**
 * @brief Game Instance Structure
 */
typedef struct Game{
    Wg_transport *transport;  /*!< transport connected to the game */
    Wg_transport *server;     /*!< transport server                */
    pthread_mutex_t mutex;    /*!< mutex critical section          */
    pthread_t thread;         /*!< pipe thread                     */
    pthread_spinlock_t lock;  /*!< pipe thread lock                */
    wg_uint is_blocked:1;     /*!< is passing blocked              */
    wg_char *log_file;        /*!< log file                        */
}Game;

/** Function prototypes                 */
WG_PRIVATE wg_status add_default_hooks(void);

WG_PRIVATE wg_status def_cinfo(wg_uint argc, wg_char *args[], 
                              void *private_data);

WG_PRIVATE void
gpm_game_enter_critical(Game *game);

WG_PRIVATE void
gpm_game_exit_critical(Game *game);

WG_PRIVATE
void *pipe_thread(void *data);

WG_PRIVATE wg_status
start_server(pthread_t *thread, void *user_data);

WG_PRIVATE wg_status
stop_server(pthread_t *thread);

WG_PRIVATE void
set_server_state(Game *game, wg_boolean state);

WG_PRIVATE wg_boolean
get_server_state(Game *game);

/** Static symbols                              */

/** 
* @brief Hooks created by game module
*/
WG_PRIVATE Console_hook def_cmd_info[] = {
    {
        .name         = "cinfo"                      ,
        .description  = "Print running game stats"   ,
        .cb_hook      = def_cinfo                    ,
        .flags        = HOOK_SYNC                    ,
        .private_data = NULL             
    }
};

/**
 * @brief Running game instance;
 */
WG_PRIVATE Game running_game = {
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

/**
 * @brief lock for exit_pipe_thread
 */
WG_PRIVATE pthread_spinlock_t sp;

/** 
* @brief exit thread pipe flag
*/
WG_PRIVATE wg_boolean exit_pipe_thread = WG_FALSE;

/**
 * @brief Initialize game control module
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
gpm_game_init(void)
{
    wg_status status = WG_FAILURE;

    add_default_hooks();

    memset(&running_game, '\0', sizeof (Game));

    running_game.server    = NULL;
    running_game.transport = NULL;

    pthread_spin_init(&sp, PTHREAD_PROCESS_SHARED);

    pthread_spin_init(&running_game.lock, PTHREAD_PROCESS_SHARED);

    gpm_game_block();

    wg_strdup("wglog.txt", &running_game.log_file);

    unlink(running_game.log_file);

    /* Create server                  */
    status = gpm_game_set_server("inet:127.0.0.1:7777");
    if (WG_SUCCESS != status){
        return WG_FAILURE;
    }

    return status;
}

/** 
* @brief Release resources allocated by gpm_game_init()
* 
* @retval WG_SUCCESS
*/
wg_status
gpm_game_cleanup(void)
{
    stop_server(&running_game.thread);
    pthread_spin_destroy(&sp);
    pthread_spin_destroy(&running_game.lock);
    gpm_game_clear_transport();
    gpm_game_clear_server();
    WG_FREE(running_game.log_file);

    return WG_SUCCESS;
}

/** 
* @brief Create new server and start listening
* 
* @param address listening address
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
gpm_game_set_server(const wg_char *address)
{
    wg_status status = WG_FAILURE;

    gpm_game_enter_critical(&running_game);

    /* Close old server if exists or allocate memory for new
    */
    if (running_game.server != NULL){
        stop_server(&running_game.thread);
        transport_close(running_game.server);
    }else{
        running_game.server = WG_MALLOC(sizeof (Wg_transport));
        if (NULL == running_game.server){
            WG_LOG("WG_MALLOC:%s\n", strerror(errno));
            return WG_FAILURE;
        }
    }

    /* Create server                  */
    status = transport_server_init(running_game.server, address);
    if (WG_SUCCESS != status){
        return WG_FAILURE;
    }
    WG_DEBUG("Server created at %s\n", address);

    start_server(&running_game.thread, &running_game);

    gpm_game_exit_critical(&running_game);

    return WG_SUCCESS;
}


/** 
* @brief Create new transport
* 
* @param address trasport address
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
gpm_game_set_transport(const wg_char *address)
{
    wg_status status = WG_FAILURE;

    CHECK_FOR_NULL_PARAM(address);

    gpm_game_enter_critical(&running_game);

    /* Release old transport or allocate memory for new one */
    if (NULL != running_game.transport){
        transport_close(running_game.transport);
    }else{
        running_game.transport = WG_MALLOC( sizeof (Wg_transport));
        if (NULL == running_game.transport){
            return WG_FAILURE;
        }
    }

    /* Initialize new transport                             */
    status = transport_init(running_game.transport, address);

    gpm_game_exit_critical(&running_game);

    return status;
}

/** 
* @brief Stop server and release resources
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
gpm_game_clear_server(void)
{
    wg_status status = WG_FAILURE;
    Wg_transport *server = NULL;

    gpm_game_enter_critical(&running_game);

    server = running_game.server;

    if (NULL != server){
        status = transport_close(server);
        WG_FREE(server);
        running_game.server = NULL;
    }

    gpm_game_exit_critical(&running_game);

    return status;
}

/** 
* @brief Clear transport and release resources
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
gpm_game_clear_transport(void)
{
    wg_status status = WG_FAILURE;
    Wg_transport *transport = NULL;

    gpm_game_enter_critical(&running_game);

    transport = running_game.transport;

    if (NULL != transport){
        status = transport_close(transport);
        WG_FREE(transport);
        running_game.transport = NULL;
    }

    gpm_game_exit_critical(&running_game);

    return status;
}

/** 
* @brief Stop passing data to the game
* 
* @retval WG_SUCCESS
*/
wg_status
gpm_game_block(void)
{
    set_server_state(&running_game, WG_TRUE);

    return WG_SUCCESS;
}

/** 
* @brief Start passing data to the game
* 
* @retval WG_SUCCESS
*/
wg_status
gpm_game_unblock(void)
{
    set_server_state(&running_game, WG_FALSE);

    return WG_SUCCESS;
}

/** 
* @brief Get pipe state
* 
* @param state memory for state. WG_TRUE blocked, WG_FALSE unblocked
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
gpm_get_blocking_state(wg_boolean *state)
{
    CHECK_FOR_NULL_PARAM(state);

    *state =  get_server_state(&running_game);

    return WG_SUCCESS;
}

/*
 * Static functions 
 */

WG_PRIVATE void
gpm_game_enter_critical(Game *game)
{
    pthread_mutex_lock(&game->mutex);

    return;
}

WG_PRIVATE void
gpm_game_exit_critical(Game *game)
{
    pthread_mutex_unlock(&game->mutex);

    return;
}

WG_PRIVATE wg_status
add_default_hooks(void)
{
    wg_uint index = 0;
    Console_hook *def_cmd = NULL;
    Console_hook *hook = NULL;
    wg_status  status = WG_FAILURE;

    for (index = 0; index < ELEMNUM(def_cmd_info); ++index){
        def_cmd = &(def_cmd_info[index]);
        status = gpm_console_new_hook(def_cmd->name, def_cmd->cb_hook, NULL,
                &hook);
        CHECK_FOR_FAILURE(status);

        gpm_console_set_hook_flags(hook, def_cmd->flags);
        gpm_console_add_hook_description(hook, def_cmd->description, NULL);
        gpm_console_register_hook(hook);
    }

    return WG_SUCCESS;
}

WG_PRIVATE wg_status 
def_cinfo(wg_uint argc, wg_char *args[], void *private_data)
{
    gpm_game_enter_critical(&running_game);

    WG_PRINT("TODO\n");

    gpm_game_exit_critical(&running_game);

    return WG_SUCCESS;
}

WG_PRIVATE wg_boolean
is_finished(void)
{
    wg_boolean flag = WG_FALSE;
    pthread_spin_lock(&sp);
    flag = exit_pipe_thread;
    pthread_spin_unlock(&sp);

    return flag;
}

WG_PRIVATE void
set_server_state(Game *game, wg_boolean state)
{
    pthread_spin_lock(&game->lock);

    game->is_blocked = state;

    pthread_spin_unlock(&game->lock);

    return;
}

WG_PRIVATE wg_boolean
get_server_state(Game *game)
{
    wg_boolean state = WG_FALSE;

    pthread_spin_lock(&game->lock);

    state = game->is_blocked;

    pthread_spin_unlock(&game->lock);

    return state;
}

WG_PRIVATE void
handler(int sig)
{
    pthread_spin_lock(&sp);
    exit_pipe_thread = WG_TRUE;
    pthread_spin_unlock(&sp);
    return;
}

/** 
* @brief Pipe thread.
* 
* Read data from server and send to client
* 
* @param data pointer to Game
* 
* @retval NULL
*/
WG_PRIVATE
void *pipe_thread(void *data)
{
    Game *game = (Game*)data;
    FILE *log_file = NULL;
    wg_char buffer[1024];
    wg_size size = 0;
    sigset_t sig;
    wg_boolean block_state = WG_FALSE;
    static struct sigaction sigact;

    exit_pipe_thread = WG_FALSE;
  
    /* Set handler for SIGUSR1 signal */
    memset(&sigact, '\0', sizeof (sigact));
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigact.sa_handler = handler;
    sigaction(SIGUSR1, &sigact, NULL);

    /* Unmask SIGUSR1 for this thread */
    sigemptyset(&sig);
    sigaddset(&sig, SIGUSR1);
    pthread_sigmask(SIG_UNBLOCK, &sig, NULL);

    while (!is_finished() && (size = transport_receive(
                    running_game.server, buffer, sizeof (buffer) - 1)) != -1){
        gpm_get_blocking_state(&block_state);
        gpm_game_enter_critical(&running_game);
        block_state = ((block_state == WG_FALSE) && (game->transport != NULL));
        if (block_state == WG_TRUE){
            buffer[size] = '\0';
            log_file = fopen(game->log_file, "a");
            if (NULL != log_file){
                fprintf(log_file, "%s", buffer);
                fclose(log_file);
                log_file = NULL;
            }
            transport_connect(game->transport);
            transport_send(game->transport, buffer, size);
            transport_disconnect(game->transport);
        }
        gpm_game_exit_critical(&running_game);
    }

    WG_DEBUG("Pipe thread process exiting....\n");

    return NULL;
}

WG_PRIVATE wg_status
stop_server(pthread_t *thread)
{
    pthread_kill(*thread, SIGUSR1); 

    pthread_join(*thread, NULL);
    
    return WG_SUCCESS;
}

WG_PRIVATE wg_status
start_server(pthread_t *thread, void *user_data)
{
    pthread_attr_t attr;
    int status = -1;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    status = pthread_create(thread, &attr, 
            pipe_thread, user_data);
    if (0 != status){
        pthread_attr_destroy(&attr);
        return WG_FAILURE;
    }
    pthread_attr_destroy(&attr);

    return WG_SUCCESS;
}

/*! @} */
