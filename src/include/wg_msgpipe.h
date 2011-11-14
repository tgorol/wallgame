#ifndef _WG_MSGPIPE_H
#define _WG_MSGPIPE_H


enum {MSG_THREAD_PROD = 0, MSG_THREAD_CONS, MSG_THREAD_NUM}; 

/**
 * @brief Message pipe thread function paramaters
 */
typedef struct Msgpipe_param{
    WorkQ *queue;        /*!< message queue used by a pipe */
    void  *user_data;    /*!< user defined data            */
}Msgpipe_param;

/**
 * @brief Message pipe structure
 */
typedef struct Msgpipe{
    pthread_t thread[MSG_THREAD_NUM];        /*!< threads id's               */
    int       cancel_status[MSG_THREAD_NUM]; /*!< cancel status              */
    void*     exit_code[MSG_THREAD_NUM];     /*!< exit code                  */
    WorkQ     *queue;                        /*!< queue assigned to the pipe */
    void*   (*producer)(Msgpipe_param *);    /*!< producer function          */
    void*   (*consumer)(Msgpipe_param *);    /*!< consuler function          */
    void    *user_data;                      /*!< user defined data          */
}Msgpipe;

WG_PUBLIC wg_status
wg_msgpipe_kill(Msgpipe *msgpipe);

WG_PUBLIC wg_status
wg_msgpipe_get_exit_codes(Msgpipe *msgpipe, 
        void **exit_producer, void **exit_consumer);

WG_PUBLIC wg_status
wg_msgpipe_create(void* (*producer)(Msgpipe_param *), 
        void* (*consumer)(Msgpipe_param *),
        WorkQ *queue, Msgpipe *msgpipe, void *user_data);

#endif
