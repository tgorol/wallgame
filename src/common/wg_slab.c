#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_linked_list.h>
#include <wg_iterator.h>
#include <wg_workqueue.h>
#include <wg_slab.h>

wg_status
wg_slab_init(wg_size block_size, wg_size num, Wg_slab *slab)
{
    wg_size real_size = 0;
    Wg_slab slb = {0};
    wg_int index = 0;
    List_head *block_head = NULL; 
    wg_uchar *block = NULL;
    wg_status status = WG_FAILURE;

    real_size = block_size + sizeof (List_head);

    status = pthread_mutexattr_init(&slb.attr);
    if (0 != status){
        return WG_FAILURE;
    }

    status = pthread_mutex_init(&slb.lock, &slb.attr);
    if (0 != status){
        pthread_mutexattr_destroy(&slb.attr);
        return WG_FAILURE;
    }

    status = pthread_cond_init(&slb.empty, NULL);
    if (0 != status){
        pthread_mutex_destroy(&slb.lock);
        pthread_mutexattr_destroy(&slb.attr);
        return WG_FAILURE;
    }

    slb.slab = WG_CALLOC(num, real_size);
    CHECK_FOR_NULL(slb.slab);

    slb.offset = block_size;
    slb.block_size = block_size;
    list_init(&slb.head);

    for (index = 0; index < num; ++index){
        block = ((wg_uchar*)slb.slab) + (index * real_size);
        block_head = (List_head*)(block + block_size);
        list_add(&slb.head, block_head);
    }

    *slab = slb;

    return WG_SUCCESS;
}


wg_status
wg_slab_alloc(Wg_slab *slab, void **block)
{
    void *object = NULL;

    CHECK_FOR_NULL(slab);
    CHECK_FOR_NULL(block);

    pthread_mutex_lock(&slab->lock);

    while (list_empty(&slab->head) == WG_TRUE){
        pthread_cond_wait(&slab->empty, &slab->lock);
    }

    object = dlist_pop_first(&slab->head, slab->offset);

    pthread_mutex_unlock(&slab->lock);

    *block = object;

    return WG_SUCCESS;
}

wg_status
wg_slab_free(Wg_slab *slab, void *block)
{
    List_head *block_list = {0};

    CHECK_FOR_NULL(slab);
    CHECK_FOR_NULL(block);

    memset(block, '\0', slab->block_size);

    block_list = (List_head*)((wg_uchar*)block + slab->offset);

    list_init(block_list);

    pthread_mutex_lock(&slab->lock);

    list_add(&slab->head, block_list);

    pthread_mutex_unlock(&slab->lock);

    return WG_SUCCESS;
}

wg_status
wg_slab_cleanup(Wg_slab *slab)
{
    CHECK_FOR_NULL(slab);

    /* TODO Add code: wait for all blocks */

    WG_FREE(slab->slab);
    pthread_cond_destroy(&slab->empty);
    pthread_mutex_destroy(&slab->lock);
    pthread_mutexattr_destroy(&slab->attr);

    memset(slab, '\0', sizeof (Wg_slab));

    return WG_SUCCESS;

}
