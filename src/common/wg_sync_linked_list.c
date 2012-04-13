#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_linked_list.h>
#include <wg_iterator.h>
#include <wg_sync_linked_list.h>

/*! \defgroup squeue Synchronized Queue Implementation.
 * @ingroup misc
 */

/*! @{ */

/**
 * @brief Initialize the queue
 *
 * @param queue  pointer to the queue 
 * @param offset numbers of bytes from start of the object structure to the List_head structure.
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
wg_workq_init(WorkQ *queue, wg_int offset)
{
    int status = 0;

    CHECK_FOR_NULL(queue);

    list_init(&queue->head);
   
    /*initialize mutex attributes */ 
    status = pthread_mutexattr_init(&queue->attr);
    if (0 != status){
        return WG_FAILURE;
    }

    /* initialize mutex */
    status = pthread_mutex_init(&queue->lock, &queue->attr);
    if (0 != status){
        pthread_mutexattr_destroy(&queue->attr);
        return WG_FAILURE;
    }

    /* initialize not_empty condition */
    status = pthread_cond_init(&queue->not_empty, NULL);
    if (0 != status){
        pthread_mutex_destroy(&queue->lock);
        pthread_mutexattr_destroy(&queue->attr);
        return WG_FAILURE;
    }

    queue->offset = offset;
    queue->sealed = WG_FALSE;

    return WG_SUCCESS;
}

/**
 * @brief Seal the queue.
 *
 * Make adding new elements impossible
 *
 * @param queue
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
wg_workq_seal(WorkQ *queue)
{
    CHECK_FOR_NULL(queue);
    pthread_mutex_lock(&queue->lock);

    queue->sealed = WG_TRUE;

    pthread_mutex_unlock(&queue->lock);

    return WG_SUCCESS;
}

/**
 * @brief Check if the queue is empty
 *
 * @param queue 
 *
 * @retval WG_TRUE
 * @retval WG_FALSE
 */
wg_boolean
wg_workq_is_empty(WorkQ *queue)
{
    wg_boolean is_empty = WG_FALSE;

    CHECK_FOR_NULL(queue);

    /* enter critical section */
    pthread_mutex_lock(&queue->lock);

    /* get empty status */
    is_empty = list_empty(&queue->head);

    /* exit critical section */
    pthread_mutex_unlock(&queue->lock);

    return is_empty;
}

/**
 * @brief Release resources used by the queue
 * 
 * @param queue Queue to release
 *
 * @retval WG_SUCCESS
 * @retval WG_FALIURE
 */
wg_status
wg_workq_cleanup(WorkQ *queue)
{
    CHECK_FOR_NULL(queue);

    /* destroy not_empty condition */
    pthread_cond_destroy(&queue->not_empty);
 
    /* destroty lock */
    pthread_mutex_destroy(&queue->lock);

    /*destroy attributes */ 
    pthread_mutexattr_destroy(&queue->attr);

    WG_ZERO_STRUCT(queue);

    return WG_SUCCESS;

}

/**
 * @brief Add element to the end of the queue
 *
 * @param queue Destination queue
 * @param elem  Element to add
 *
 * @retval WG_SUCCESS
 * @retval WG_FALIURE
 */
wg_status
wg_workq_add(WorkQ *queue, List_head *elem)
{
    CHECK_FOR_NULL(queue);
    CHECK_FOR_NULL(elem);

    /* enter critical section */
    pthread_mutex_lock(&queue->lock);

    /* if queue sealed return error */
    if (queue->sealed == WG_TRUE){
        pthread_mutex_unlock(&queue->lock);
        return WG_FAILURE;
    }
    
    /* add element to the queue */
    list_add(&queue->head, elem);

    /* wake up waiting threads */
    pthread_cond_signal(&queue->not_empty);

    /* exit critical section */ 
    pthread_mutex_unlock(&queue->lock);

    return WG_SUCCESS;
}

/**
 * @brief Get element from the front of the queue
 *
 * @param queue Source queue
 * @param elem Memory to store pointer to the element
 *
 * @retval WG_SUCCESS
 * @retval WG_FALIURE
 */
wg_status
wg_workq_get(WorkQ *queue, void **elem)
{

    /* enter critical section*/ 
    pthread_mutex_lock(&queue->lock);

    /* on empty queue wait */ 
    while (list_empty(&queue->head) == WG_TRUE){
        pthread_cond_wait(&queue->not_empty, &queue->lock);
    }

    /* get element fro the top of the queue */
    *elem = dlist_get_first(&queue->head, queue->offset);
    dlist_pop_first(&queue->head, queue->offset);

    /* exit critical section*/
    pthread_mutex_unlock(&queue->lock);

    return WG_SUCCESS;
}

/*! @} */
