#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_trans.h>
#include <wg_linked_list.h>
#include <wg_workqueue.h>
#include <wg_slab.h>
#include <wgp.h>

#include "include/gpm_msg.h"
#include "include/gpm_game.h"

/*! \defgroup gpm_game Gameplay Game Control 
 */

/*! @{ */

enum {THREAD_PROD = 0, THREAD_CONS, THREAD_NUM}; 

#define MSG_QUEUE_MAX   1024

/**
 * @brief Game Instance Structure
 */
typedef struct Game{
    Transport transport;  /*!< transport connected to the game */
    pid_t process_id;     /*!< process id of the game          */
    wg_uint game_id;      /*!< user defined id                 */
}Game;

/**
 * @brief Running game instance;
 */
WG_PRIVATE Game running_game;
WG_PRIVATE pthread_t threads[THREAD_NUM];
WG_PRIVATE WorkQ msg_queue;
WG_PRIVATE Wg_slab  msg_slab;
WG_PRIVATE Wgp_plugin plugin;


WG_PRIVATE void * msg_from_sensor(void *queue);
WG_PRIVATE void * msg_to_game(void *queue);
WG_PRIVATE wg_status
create_pipe_threads(void* (*producer)(void*), void* (*consumer)(void *),
        WorkQ *queue, pthread_t thread[THREAD_NUM]);

WG_PRIVATE wg_status
kill_pipe_threads(pthread_t thread[THREAD_NUM], int ret_status[THREAD_NUM]);

/**
 * @brief Start a game
 *
 * @param argv[]    arguments
 * @param address  address to use to communicate with a game
 * @param plugin_name  name of the plugin to use with the game
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
gpm_game_run(wg_char *argv[], wg_char *address, const wg_char *plugin_name)
{
    pid_t game_pid = 0;
    wg_status status = WG_FAILURE;
    Game game ;
    const Wgp_info *plug_info = NULL;

    CHECK_FOR_NULL(argv);

    if (gpm_game_is_running() == WG_TRUE){
        WG_LOG("Game is running\n");
        return WG_FAILURE;
    }
    switch (game_pid = fork()){
        case 0:
            execvp(argv[0], argv);
            WG_ERROR("BUG: Shoudn't be here\n");
            /* TODO make this more readable */
            exit(1);
            break;
        case -1:
            WG_ERROR("forking game %s: %s\n", argv[0], strerror(errno));
            return WG_FAILURE;
        default:
            game.process_id = game_pid;
            WG_DEBUG("Process started with $$=%ld\n", (long)game_pid);

            do {
                /* Create a connection to a game */
                status = trans_unix_new(&game.transport, address);
                if (WG_FAILURE == status){
                    break;
                }

                WG_DEBUG("Transaction created at %s\n", address);

                /* TODO Add error checking */

                status = wg_workq_init(&msg_queue, 
                        GET_OFFSET(Wg_message, list));
                if (WG_FAILURE == status){
                    trans_unix_close(&game.transport);
                    break;
                }

                status = wg_slab_init(sizeof (Wg_message), 
                        MSG_QUEUE_MAX, &msg_slab);
                if (WG_FAILURE == status){
                    wg_workq_cleanup(&msg_queue);
                    trans_unix_close(&game.transport);
                    break;
                }

                /* load plugin               */
                memset(&plugin, '\0', sizeof (Wgp_plugin));
                status = wgp_load(plugin_name, &plugin);
                if (WG_FAILURE == status){
                    wg_slab_cleanup(&msg_slab);
                    wg_workq_cleanup(&msg_queue);
                    trans_unix_close(&game.transport);
                    break;
                }

                wgp_info(&plugin, &plug_info);

                WG_LOG("Plugin loaded : %s\n", plug_info->name);

                status = create_pipe_threads(msg_from_sensor, 
                        msg_to_game, &msg_queue, threads);
                if (WG_FAILURE == status){
                    wgp_unload(&plugin);
                    wg_slab_cleanup(&msg_slab);
                    wg_workq_cleanup(&msg_queue);
                    trans_unix_close(&game.transport);
                    break;
                }

                running_game = game;
            } while (0);
            if (WG_FAILURE == status){
                kill(game.process_id, SIGTERM);
                waitpid(game.process_id, NULL, WNOHANG);
                return WG_FAILURE;
            }

            break;
    }

    return WG_SUCCESS;
}

wg_status
gpm_game_add_message(Msg_type type, ...)
{
    wg_status status = WG_FAILURE;
    Wg_message *message = NULL;

    status = wg_slab_alloc(&msg_slab, (void**)&message);
    CHECK_FOR_FAILURE(status);

    message->type = type;

    status = wg_workq_add(&msg_queue, &message->list);
    if (WG_FAILURE == status){
        wg_slab_free(&msg_slab, message);
        return WG_FAILURE;
    }

    return WG_SUCCESS;
}


/**
 * @brief Send data to running game
 *
 * @param data buffer
 * @param size number of bytes to send 
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 *
 * @todo Make it static
 */
wg_status
gpm_game_send(wg_uchar *data, wg_size size)
{
    wg_status status = WG_FAILURE;

    CHECK_FOR_NULL(data);

    if (gpm_game_is_running() == WG_FALSE){
        return WG_FAILURE;
    }

    status = trans_unix_connect(&running_game.transport);
    CHECK_FOR_FAILURE(status);

    status = trans_unix_send(&running_game.transport, data, size);
    CHECK_FOR_FAILURE(status);

    status = trans_unix_disconnect(&running_game.transport);
    CHECK_FOR_FAILURE(status);

    return WG_SUCCESS;
}

/**
 * @brief Kill running game
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
gpm_game_kill(void)
{
    int ret_status[THREAD_NUM] = {0};

    if (gpm_game_is_running() == WG_TRUE){
        kill_pipe_threads(threads,  ret_status);

        trans_unix_close(&running_game.transport);

        kill(running_game.process_id, SIGTERM);
        waitpid(running_game.process_id, NULL, WNOHANG);
        WG_DEBUG("Game closed. $$=%ld\n", (long)running_game.process_id);

        wg_slab_cleanup(&msg_slab);

        wgp_unload(&plugin);

        /* TODO Clean a workq */

        memset(&running_game, '\0', sizeof (Game));
    }

    return WG_SUCCESS;
}

/**
 * @brief Check if a game is running
 *
 * @retval WG_TRUE 
 * @retval WG_FALSE
 */
wg_boolean
gpm_game_is_running(void)
{
    wg_boolean is_running = WG_FALSE;

    if (running_game.process_id != 0){
        if (0 == waitpid(running_game.process_id, NULL, WNOHANG)){
            is_running = WG_TRUE;
        }else{
            is_running = WG_FALSE;
            trans_unix_close(&running_game.transport);
            memset(&running_game, '\0', sizeof (Game));
        }
    }

    return is_running;
}

/**
 * @brief Set game id
 *
 * It is user define id. The game module has nothing to do with it.
 *
 * @param id user defined game id
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
gpm_game_set_id(wg_uint id)
{
    wg_status status = WG_FAILURE;

    if (gpm_game_is_running() == WG_TRUE){
        running_game.game_id = id;
        status = WG_SUCCESS;
    }

    return status;
}

/**
 * @brief Get game id
 *
 * @param id memory to store id
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
gpm_game_get_id(wg_uint *id)
{
    wg_status status = WG_FAILURE;

    CHECK_FOR_NULL(id);

    if (gpm_game_is_running() == WG_TRUE){
        *id = running_game.game_id;
        status = WG_SUCCESS;
    }

    return status;
}

WG_PRIVATE wg_status
kill_pipe_threads(pthread_t thread[THREAD_NUM], int ret_status[THREAD_NUM])
{
    CHECK_FOR_NULL_PARAM(thread);
    CHECK_FOR_NULL_PARAM(ret_status);

    ret_status[THREAD_PROD] = pthread_cancel(thread[THREAD_PROD]);
    ret_status[THREAD_CONS] = pthread_cancel(thread[THREAD_CONS]);

    return (ret_status[THREAD_CONS] == 0 && ret_status[THREAD_PROD] == 0) ?
        WG_SUCCESS   :
        WG_FAILURE;
}

WG_PRIVATE wg_status
create_pipe_threads(void* (*producer)(void*), void* (*consumer)(void *),
        WorkQ *queue, pthread_t thread[THREAD_NUM])
{
   pthread_t thread_prod = 0;
   pthread_t thread_cons = 0;
   int rc = 0;

   rc = pthread_create(&thread_prod, NULL, producer, (void*)queue);
   if (0 != rc){
       WG_LOG("Threads creation failed: %d\n", rc);
       return WG_FAILURE;
   }

   rc = pthread_create(&thread_cons, NULL, consumer, (void*)queue);
   if (0 != rc){
       WG_LOG("Threads creation failed: %d\n", rc);
       return WG_FAILURE;
   }

   thread[THREAD_PROD] = thread_prod;
   thread[THREAD_CONS] = thread_cons;

   return WG_SUCCESS;
}

WG_PRIVATE void *
msg_from_sensor(void *queue)
{

    for (;;){
        sleep(3);
    }
    return queue;
}

WG_PRIVATE void *
msg_to_game(void *queue)
{
    Wg_message *msg = NULL;

    for (;;){
        printf("\n%s: Waiting for a message\n", __PRETTY_FUNCTION__);
        wg_workq_get((WorkQ*)queue, (void**)&msg);
        printf("Received msg type = %d\n", msg->type);
        wg_slab_free(&msg_slab, msg);
    }

     return queue;
}

/*! @} */
