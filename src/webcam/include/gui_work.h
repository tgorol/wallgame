#ifndef _GUI_WORK_H
#define _GUI_WORK_H

WG_PUBLIC wg_status
gui_work_thread_init();

WG_PUBLIC wg_status
gui_work_thread_cleanup();

WG_PUBLIC void *
gui_work_create(wg_size size, Wg_wq_cb work_cb);

WG_PUBLIC void
gui_work_destroy(void *data);

WG_PUBLIC void
gui_work_add(void *data);

#endif /* _GUI_WORK_H */

