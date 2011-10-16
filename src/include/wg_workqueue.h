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

WG_PUBLIC wg_status wg_workq_init (WorkQ *queue, wg_int offset);
WG_PUBLIC wg_status   wg_workq_seal (WorkQ *queue);
WG_PUBLIC wg_boolean  wg_workq_is_empty (WorkQ *queue);
WG_PUBLIC wg_status   wg_workq_cleanup (WorkQ *queue);
WG_PUBLIC wg_status   wg_workq_add (WorkQ *queue, List_head *elem);
WG_PUBLIC wg_status   wg_workq_get (WorkQ *queue, void **elem);

#endif
