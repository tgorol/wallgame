#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <errno.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_linked_list.h>
#include <wg_workqueue.h>

#include <wg_msgpipe.h>

/*! \defgroup Msgpipe Message Pipe Implemantation
 */

/*! @{ */

WG_PRIVATE void* wg_msgpipe_producer(void *data);
WG_PRIVATE void* wg_msgpipe_consumer(void *data);
WG_PRIVATE void producer_cleanup(void *arg);
WG_PRIVATE void consumer_cleanup(void *arg);

/**
 * @brief Kill pipe
 *
 * Error codes of threads can be retrive by wg_msgpipe_get_exit_codes
 *
 * @param msgpipe pipe to kill
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
wg_msgpipe_kill(Msgpipe *msgpipe)
{
    wg_int status = 0;
    CHECK_FOR_NULL_PARAM(msgpipe);

    /* cancel consumer */
    status = pthread_cancel(msgpipe->thread[MSG_THREAD_CONS]);
    msgpipe->cancel_status[MSG_THREAD_CONS] = status;
    /* If thread exited already print wraning */
    if (ESRCH == status){
        WG_DEBUG("Consumer thread finished already.\n");
    }
 
    /* wait for consumer */
    pthread_join(msgpipe->thread[MSG_THREAD_CONS],
            &(msgpipe->exit_code[MSG_THREAD_CONS]));
    
    WG_DEBUG("Consumer thread joined.\n");

    /* cancel producer */
    status = pthread_cancel(msgpipe->thread[MSG_THREAD_PROD]);
    msgpipe->cancel_status[MSG_THREAD_PROD] = status;
    /* If thread exited already print wraning */
    if (ESRCH == status){
        WG_DEBUG("Consumer thread finished already.\n");
    }

    /* wait for producer */
    pthread_join(msgpipe->thread[MSG_THREAD_PROD],
            &(msgpipe->exit_code[MSG_THREAD_PROD]));

    WG_DEBUG("Producer thread joined.\n");

    return WG_SUCCESS;
}

/**
 * @brief Get exit codes for producer and consumer threads
 *
 * @param msgpipe
 * @param exit_producer
 * @param exit_consumer
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
wg_msgpipe_get_exit_codes(Msgpipe *msgpipe, 
        void **exit_producer, void **exit_consumer)
{
    CHECK_FOR_NULL_PARAM(msgpipe);

    *exit_producer = msgpipe->exit_code[MSG_THREAD_PROD];
    *exit_consumer = msgpipe->exit_code[MSG_THREAD_CONS];

    return WG_SUCCESS;
}

/**
 * @brief Create a message pipe
 *
 * @param producer 
 * @param consumer
 * @param queue     queue to use as pipe
 * @param msgpipe   memory to store message pipe instance
 * @param user_data user specified data
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
wg_msgpipe_create(void* (*producer)(Msgpipe_param *), 
        void* (*consumer)(Msgpipe_param *),
        WorkQ *queue, Msgpipe *msgpipe, void *user_data)
{
   pthread_t thread_prod = 0;
   pthread_t thread_cons = 0;
   pthread_attr_t attr;
   int rc = 0;

   CHECK_FOR_NULL_PARAM(producer);
   CHECK_FOR_NULL_PARAM(consumer);
   CHECK_FOR_NULL_PARAM(queue);
   CHECK_FOR_NULL_PARAM(msgpipe);

   memset(msgpipe, '\0', sizeof (Msgpipe));

   msgpipe->producer  = producer;
   msgpipe->consumer  = consumer;
   msgpipe->user_data = user_data; 
   msgpipe->queue     = queue;

   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

   rc = pthread_create(&thread_cons, &attr, wg_msgpipe_consumer, 
           (void*)msgpipe);
   if (0 != rc){
       WG_ERROR("Thread creation failed: %d\n", rc);
       pthread_attr_destroy(&attr);
       return WG_FAILURE;
   }
   msgpipe->thread[MSG_THREAD_CONS] = thread_cons;

   WG_DEBUG("Consumer thread created.\n");

   rc = pthread_create(&thread_prod, &attr, wg_msgpipe_producer, 
           (void*)msgpipe);
   if (0 != rc){
       WG_ERROR("Thread creation failed: %d\n", rc);
       pthread_attr_destroy(&attr);
       pthread_cancel(thread_cons);
       return WG_FAILURE;
   }
   WG_DEBUG("Producer thread created.\n");

   pthread_attr_destroy(&attr);

   msgpipe->thread[MSG_THREAD_PROD] = thread_prod;

   return WG_SUCCESS;
}

WG_PRIVATE void
producer_cleanup(void *arg)
{
    return;
}

/**
 * @brief Producer thread prolog function.
 *
 * This function setup a producer thread and then calls user define 
 * function.
 *
 * @param data   pointer to Msgpipe structute
 *
 * @return user defined value
 */
WG_PRIVATE void *
wg_msgpipe_producer(void *data)
{
    Msgpipe *msgpipe = NULL;
    int old_state = 0;
    int old_type = 0;
    Msgpipe_param param = {0};
    void *ret_value = NULL;

    CHECK_FOR_NULL_PARAM(data);

    msgpipe = (Msgpipe *)data;

    CHECK_FOR_NULL_PARAM(msgpipe->producer);

    /* enable thread cancelation    */
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old_state);

    /* make cancellation only when thread fell asleep */
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old_type);

    /* create data passed to the user define function */
    param.queue     = msgpipe->queue;
    param.user_data = msgpipe->user_data;
    
    /* set cleanup routine */
    pthread_cleanup_push(producer_cleanup, msgpipe);

    /* call producer function */
    ret_value =  msgpipe->producer(&param);

    pthread_cleanup_pop(1);

    return ret_value;
}

WG_PRIVATE void
consumer_cleanup(void *arg)
{
    return;
}

/**
 * @brief Consumer thread prolog function.
 *
 * This function setup a consumer thread and then calls user define 
 * function.
 *
 * @param data   pointer to Msgpipe structute
 *
 * @return user defined value
 */
WG_PRIVATE void *
wg_msgpipe_consumer(void *data)
{
    Msgpipe *msgpipe = NULL;
    int old_state = 0;
    int old_type = 0;
    void *ret_value = NULL;
    Msgpipe_param param = {0};

    CHECK_FOR_NULL_PARAM(data);

    msgpipe = (Msgpipe *)data;

    CHECK_FOR_NULL_PARAM(msgpipe->consumer);

    /* enable thread cancelation    */
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old_state);

    /* make cancellation only when thread fell asleep */
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old_type);

    /* create data passed to the user define function */
    param.queue     = msgpipe->queue;
    param.user_data = msgpipe->user_data;

    pthread_cleanup_push(consumer_cleanup, msgpipe);

    /* call consumer function */
    ret_value = msgpipe->consumer(&param);

    pthread_cleanup_pop(1);

    return ret_value;
}

/*! @} */
