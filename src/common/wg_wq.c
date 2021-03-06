#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <pthread.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_linked_list.h>
#include <wg_sync_linked_list.h>

#include <wg_wq.h>

/*! @defgroup workq Work queue
 *  @ingroup misc
 */
/*! @{ */

/** 
* @brief Work type
*/
typedef enum Work_type {
    EVENT_INVALID = 0,     /*!< invalid type   */
    EVENT_WORK       ,     /*!< regural task   */
    EVENT_SETUP      ,     /*!< setup task     */
    EVENT_EXIT             /*!< exit task      */
}Work_type;

/** 
* @brief Work task
*/
typedef struct Wg_work{
    Work_type type;        /*!< type of the task */
    List_head leaf;        /*!< list of tasks    */
    Wg_wq_cb work_cb;      /*!< task function    */
}Wg_work;


/** 
* @brief Work queue process
* 
* @param workq_instance work queue instance
* 
* @retval NULL
*/
WG_PRIVATE void*
process_event(void *workq_instance)
{
    WorkQ *workq = NULL;
    Wg_work *work = NULL;

    workq = (WorkQ*)workq_instance;

    for (;;){
        wg_workq_get(workq, (void**)&work);
        switch (work->type){
        case EVENT_WORK:
            if (NULL != work->work_cb){
                work->work_cb(work + 1);

                WG_FREE(work);
            }
            break;
        case EVENT_EXIT:
            WG_FREE(work);
            goto end_of_thread;
            break;
        case EVENT_SETUP:
        case EVENT_INVALID:
            WG_FREE(work);
            break;
        }
    }
    end_of_thread:

    pthread_exit(NULL);
}

/** 
* @brief Initialize work queue
* 
* @param wq   instance of work queue to initialize
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
wg_wq_init(Wg_wq *wq)
{
    pthread_attr_t attr;

    CHECK_FOR_NULL_PARAM(wq);

    wg_workq_init(&wq->workq, GET_OFFSET(Wg_work, leaf));

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&wq->thread, &attr, process_event, &wq->workq);
    pthread_attr_destroy(&attr);

    return WG_SUCCESS;
}

/** 
* @brief Release resources allocated by wg_wq_init()
* 
* @param wq work queue instance

* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
wg_wq_cleanup(Wg_wq *wq)
{
    Wg_work *work = NULL;

    CHECK_FOR_NULL_PARAM(wq);

    work = wg_wq_work_create(0, NULL);

    work[-1].type = EVENT_EXIT;

    wg_wq_add(wq, work);

    pthread_join(wq->thread, NULL);

    memset(wq, '\0', sizeof (Wg_wq));

    return WG_SUCCESS;
}


/** 
* @brief Create a task
*
* Create a task instance. User can specify size of memory allocated for
* task data.
* 
* @param size      size of user data
* @param work_cb   task function
* 
* @return pointer to user defined data or NULL for failure
*/
void *
wg_wq_work_create(wg_size size, Wg_wq_cb work_cb)
{
    Wg_work *work = NULL;

    work = WG_CALLOC(1, sizeof (Wg_work) + size);
    if (NULL == work){
        return NULL;
    }
  
    list_init(&work->leaf);
    work->type    = EVENT_WORK;
    work->work_cb = work_cb;

    return (void*)(work + 1);
}

/** 
* @brief Destroy task
*
* In most cases user won't have to call this function explicitly cause
* tasks are destroyed after compleation. Only need to call it by user will be
* when task won't be placed on the work queue.
* 
* @param data user defined data returned by wg_wq_work_create()
*
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
wg_wq_work_destroy(void *data)
{
    Wg_work *work = NULL;

    CHECK_FOR_NULL_PARAM(data);

    work = (Wg_work*)data;

    WG_FREE(work - 1);

    return WG_SUCCESS;
}

/** 
* @brief Place task on the queue
* 
* @param wq    destination queue
* @param data  user task data returned by wg_wq_work_create()
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
wg_wq_add(Wg_wq *wq, void *data)
{
    Wg_work *work = NULL;
    wg_status status = WG_FAILURE;

    CHECK_FOR_NULL_PARAM(data);

    work = (Wg_work*)data;

    status = wg_workq_add(&wq->workq, &(work[-1].leaf));

    return status;
}
/*! @} */
