#ifndef _WG_WORKQUEUE_H
#define _WG_WORKQUEUE_H


typedef struct WorkQ{
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_mutexattr_t attr;
    List_head head;
    wg_int offset;
    wg_boolean sealed;
}WorkQ;


#endif
