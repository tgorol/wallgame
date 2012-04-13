#ifndef _WG_WQ_H
#define _WG_WQ_H


/** 
* @brief Work task function type
* 
* @param data user data received by calling wg_wq_work_create()
* 
* @return void
*/
typedef void (*Wg_wq_cb)(void *data);

/** 
* @brief Work queue
*
* @todo Change WorkQ type name to Sync_list 
*/
typedef struct Wg_wq{
    pthread_t thread; /*!< work queue thread   */
    WorkQ workq;      /*!< synced linked list  */
}Wg_wq;

WG_PUBLIC wg_status
wg_wq_init(Wg_wq *wq);

WG_PUBLIC wg_status
wg_wq_cleanup(Wg_wq *wq);

WG_PUBLIC void *
wg_wq_work_create(wg_size size, Wg_wq_cb work_cb);

WG_PUBLIC wg_status
wg_wq_work_destroy(void *data);

WG_PUBLIC wg_status
wg_wq_add(Wg_wq *wq, void *data);

#endif /* _GUI_WORK_H */

