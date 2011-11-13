#ifndef _WG_WORKQUEUE_H
#define _WG_WORKQUEUE_H


/**
 * @brief Work queue structure
 */
typedef struct WorkQ{
    pthread_mutex_t lock;     /*!< workq mutex             */
    pthread_cond_t not_empty; /*!< on empty condtion event */
    pthread_mutexattr_t attr; /*!< attributes              */
    List_head head;           /*!< list head               */
    wg_int offset;            /*!< workq object offset     */
    wg_boolean sealed;        /*!< sealed flag             */
}WorkQ;

WG_PUBLIC wg_status wg_workq_init (WorkQ *queue, wg_int offset);
WG_PUBLIC wg_status   wg_workq_seal (WorkQ *queue);
WG_PUBLIC wg_boolean  wg_workq_is_empty (WorkQ *queue);
WG_PUBLIC wg_status   wg_workq_cleanup (WorkQ *queue);
WG_PUBLIC wg_status   wg_workq_add (WorkQ *queue, List_head *elem);
WG_PUBLIC wg_status   wg_workq_get (WorkQ *queue, void **elem);

#endif
