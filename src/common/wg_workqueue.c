#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_linked_list.h>
#include <wg_iterator.h>
#include <wg_workqueue.h>

/*! \defgroup squeue Pthread synchronized queue implementation.
 */

/*! @{ */

/**
 * @brief Initialize the queue
 *
 * @param queue  pointer to the queue 
 * @param offest numbers of bytes from start of the object structure to the List_head structure.
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

    status = pthread_mutexattr_init(&queue->attr);
    if (0 != status){
        return WG_FAILURE;
    }

    status = pthread_mutex_init(&queue->lock, &queue->attr);
    if (0 != status){
        pthread_mutexattr_destroy(&queue->attr);
        return WG_FAILURE;
    }

    status = pthread_cond_init(&queue->not_empty, NULL);
    if (0 != status){
        pthread_mutex_destroy(&queue->lock);
        pthread_mutexattr_destroy(&queue->attr);
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

    pthread_mutex_lock(&queue->lock);

    is_empty = list_empty(&queue->head);

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

    pthread_cond_destroy(&queue->not_empty);

    pthread_mutex_destroy(&queue->lock);

    pthread_mutexattr_destroy(&queue->attr);

    memset(queue, '\0', sizeof (WorkQ));

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

    pthread_mutex_lock(&queue->lock);

    if (queue->sealed == WG_TRUE){
        pthread_mutex_unlock(&queue->lock);
        return WG_FAILURE;
    }
    
    list_add(&queue->head, elem);

    pthread_cond_signal(&queue->not_empty);

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
    pthread_mutex_lock(&queue->lock);

    while (dlist_empty(&queue->head) == WG_TRUE){
        pthread_cond_wait(&queue->not_empty, &queue->lock);
    }

    *elem = dlist_get_first(&queue->head, queue->offset);
    dlist_pop_first(&queue->head, queue->offset);

    pthread_mutex_unlock(&queue->lock);

    return WG_SUCCESS;
}

/*! @} */
