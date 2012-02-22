#ifndef _WG_WQ_H
#define _WG_WQ_H

typedef void (*Wg_wq_cb)(void *data);

typedef struct Wg_wq{
    pthread_t thread;
    WorkQ workq;
}Wg_wq;

WG_PUBLIC wg_status
wg_wq_init(Wg_wq *wq);

WG_PUBLIC void
wg_wq_cleanup(Wg_wq *wq);

WG_PUBLIC void *
wg_wq_work_create(wg_size size, Wg_wq_cb work_cb);

WG_PUBLIC void
wg_wq_work_destroy(void *data);

WG_PUBLIC void
wg_wq_add(Wg_wq *wq, void *data);

#endif /* _GUI_WORK_H */

