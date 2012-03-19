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

WG_PRIVATE Wg_wq wq;
WG_PRIVATE wg_boolean init_flag = WG_FALSE;

wg_status
gui_work_thread_init()
{
    wg_status status = WG_SUCCESS;

    if (WG_FALSE == init_flag){
        status = wg_wq_init(&wq);
    }

    return status;
}

void
gui_work_thread_cleanup()
{
    if (WG_TRUE == init_flag){
        wg_wq_cleanup(&wq);
    }

    return;
}


void *
gui_work_create(wg_size size, Wg_wq_cb work_cb)
{
    return wg_wq_work_create(size, work_cb);
}

void
gui_work_destroy(void *data)
{
    wg_wq_work_destroy(data);

    return;
}

void
gui_work_add(void *data)
{
    wg_wq_add(&wq, data);

    return;
}