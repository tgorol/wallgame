#ifndef _WG_DLAB_H
#define _WG_DLAB_H

typedef struct Wg_slab{
    wg_size block_size;
    List_head head;
    wg_int offset;
    void *slab;
    pthread_mutex_t lock;
    pthread_cond_t empty;
    pthread_mutexattr_t attr;
}Wg_slab;

WG_PUBLIC wg_status
wg_slab_init(wg_size block_size, wg_size num, Wg_slab *slab);

WG_PUBLIC wg_status
wg_slab_cleanup(Wg_slab *slab);

WG_PUBLIC wg_status
wg_slab_free(Wg_slab *slab, void *block);

WG_PUBLIC wg_status
wg_slab_alloc(Wg_slab *slab, void **block);


#endif
