#ifndef _WG_MSGPIPE_H
#define _WG_MSGPIPE_H


enum {MSG_THREAD_PROD = 0, MSG_THREAD_CONS, MSG_THREAD_NUM}; 

typedef struct Msgpipe_param{
    WorkQ *queue;
    void  *user_data;
}Msgpipe_param;

typedef struct Msgpipe{
    pthread_t thread[MSG_THREAD_NUM];
    int       cancel_status[MSG_THREAD_NUM];
    void*     exit_code[MSG_THREAD_NUM];
    WorkQ     *queue;
    void*   (*producer)(Msgpipe_param *);
    void*   (*consumer)(Msgpipe_param *);
    void    *user_data;
}Msgpipe;

WG_PUBLIC wg_status
wg_msgpipe_kill(Msgpipe *msgpipe);

WG_PUBLIC wg_status
wg_msgpipe_get_exit_codes(Msgpipe *msgpipe, 
        int *exit_producer, int *exit_consumer);

WG_PUBLIC wg_status
wg_msgpipe_create(void* (*producer)(Msgpipe_param *), 
        void* (*consumer)(Msgpipe_param *),
        WorkQ *queue, Msgpipe *msgpipe, void *user_data);

#endif
