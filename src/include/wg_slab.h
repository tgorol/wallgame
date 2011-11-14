#ifndef _WG_DLAB_H
#define _WG_DLAB_H

/**
 * @brief Slab structure
 */
typedef struct Wg_slab{
    wg_size block_size;       /*!< slab block size       */
    List_head head;           /*!< list head             */
    wg_int offset;            /*!< slab object offest    */
    void *slab;               /*!< slab memory           */
    pthread_mutex_t lock;     /*!< slab mutex            */
    pthread_cond_t empty;     /*!< empty condition event */
    pthread_mutexattr_t attr; /*!< attributes            */
    wg_uint alloc_num;        /*!< allocations counter   */
    wg_uint free_num;         /*!< deallocation counter  */
}Wg_slab;

WG_PUBLIC wg_status
wg_slab_init(wg_size block_size, wg_size num, Wg_slab *slab);

WG_PUBLIC wg_status
wg_slab_cleanup(Wg_slab *slab);

WG_PUBLIC wg_status
wg_slab_free(Wg_slab *slab, void *block);

WG_PUBLIC wg_status
wg_slab_alloc(Wg_slab *slab, void **block);

WG_PUBLIC wg_status
wg_slab_print_stat(Wg_slab *slab);

#endif
