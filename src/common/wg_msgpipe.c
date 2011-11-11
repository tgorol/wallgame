#include <stdlib.h>
#include <string.h>
#include <pthread.h>

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
    CHECK_FOR_NULL_PARAM(msgpipe);

    /* cancel consumer */
    msgpipe->cancel_status[MSG_THREAD_CONS] = 
        pthread_cancel(msgpipe->thread[MSG_THREAD_CONS]);

    /* cancel producer */
    msgpipe->cancel_status[MSG_THREAD_PROD] = 
        pthread_cancel(msgpipe->thread[MSG_THREAD_PROD]);

    /* wait for consumer */
    pthread_join(msgpipe->thread[MSG_THREAD_CONS],
            &(msgpipe->exit_code[MSG_THREAD_CONS]));

    /* wait for producer */
    pthread_join(msgpipe->thread[MSG_THREAD_PROD],
            &(msgpipe->exit_code[MSG_THREAD_PROD]));

    return ((msgpipe->cancel_status[MSG_THREAD_CONS] == 0) && 
            (msgpipe->cancel_status[MSG_THREAD_PROD] == 0)) ?
        WG_SUCCESS   :
        WG_FAILURE   ;
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
        int *exit_producer, int *exit_consumer)
{
    CHECK_FOR_NULL_PARAM(msgpipe);

    *exit_producer = msgpipe->cancel_status[MSG_THREAD_PROD];
    *exit_consumer = msgpipe->cancel_status[MSG_THREAD_CONS];

    return WG_SUCCESS;
}

/**
 * @brief Create a message pipe
 *
 * @param producer 
 * @param consumer
 * @param queue     queue to use as pipe
 * @param msgpipe   memory to store message pipe instance
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

   memset(msgpipe, '\0', sizeof (Msgpipe));

   msgpipe->producer  = producer;
   msgpipe->consumer  = consumer;
   msgpipe->user_data = user_data; 
   msgpipe->queue = queue;

   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

   rc = pthread_create(&thread_cons, &attr, wg_msgpipe_consumer, 
           (void*)msgpipe);
   if (0 != rc){
       WG_LOG("Threads creation failed: %d\n", rc);
       pthread_attr_destroy(&attr);
       return WG_FAILURE;
   }
   msgpipe->thread[MSG_THREAD_CONS] = thread_cons;

   rc = pthread_create(&thread_prod, &attr, wg_msgpipe_producer, 
           (void*)msgpipe);
   if (0 != rc){
       WG_LOG("Threads creation failed: %d\n", rc);
       pthread_attr_destroy(&attr);
       pthread_cancel(thread_cons);
       return WG_FAILURE;
   }
   pthread_attr_destroy(&attr);

   msgpipe->thread[MSG_THREAD_PROD] = thread_prod;

   return WG_SUCCESS;
}

WG_PRIVATE void *
wg_msgpipe_producer(void *data)
{
    Msgpipe *msgpipe = NULL;
    int old_state = 0;
    int old_type = 0;
    Msgpipe_param param = {0};


    msgpipe = (Msgpipe *)data;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old_state);

    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old_type);

    param.queue     = msgpipe->queue;
    param.user_data = msgpipe->user_data;

    return msgpipe->producer(&param);
}

WG_PRIVATE void *
wg_msgpipe_consumer(void *data)
{
    Msgpipe *msgpipe = NULL;
    int old_state = 0;
    int old_type = 0;
    Msgpipe_param param = {0};

    msgpipe = (Msgpipe *)data;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old_state);

    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old_type);

    param.queue     = msgpipe->queue;
    param.user_data = msgpipe->user_data;

    return msgpipe->consumer(&param);
}

/*! @} */
