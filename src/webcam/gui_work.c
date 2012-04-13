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

#include "include/gui_work.h"

/*! \defgroup gui_work User Interface Work Queue.
*
*    Mostly used to update UI
*
*  \ingroup plugin_webcam
*/

/*! @{ */

WG_PRIVATE Wg_wq wq;
WG_PRIVATE wg_boolean init_flag = WG_FALSE;

/** 
* @brief Initialize gui_work
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
gui_work_thread_init()
{
    wg_status status = WG_SUCCESS;

    if (WG_FALSE == init_flag){
        status = wg_wq_init(&wq);
    }

    return status;
}

/** 
* @brief Release resources allocated by gui_work
*/
void
gui_work_thread_cleanup()
{
    if (WG_TRUE == init_flag){
        wg_wq_cleanup(&wq);
    }

    return;
}

/** 
* @brief Create new work
*
* Allocate work instance. The instance contain 'size' number of bytes for
* using as user data. This cam be filled by user. When work is scheduled to 
* execute pointer to this data is passed as user data parameter.
* Retuned instance can be casted to user defined structure.
* 
* @param size     size of bytes to allocate for user use
* @param work_cb  work function;
* 
* @return instance of new work. 
*/
void *
gui_work_create(wg_size size, Wg_wq_cb work_cb)
{
    return wg_wq_work_create(size, work_cb);
}

/** 
* @brief Destroy work
* 
* @param data work instance retuned by gui_work_create()
*/
void
gui_work_destroy(void *data)
{
    wg_wq_work_destroy(data);

    return;
}

/** 
* @brief Add work to queue
* 
* @param data instance retuned by gui_work_create()
*/
void
gui_work_add(void *data)
{
    wg_wq_add(&wq, data);

    return;
}

/*! @} */
