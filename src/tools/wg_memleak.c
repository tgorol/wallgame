#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>

#include <wg_linked_list.h>
#include <wg_iterator.h>

#define FILE_NAME_SIZE_MAX  32

typedef enum alloc_type{
    MEMLEAK_INVALID = 0,
    MEMLEAK_MALLOC     ,
    MEMLEAK_CALLOC    
}alloc_type;

typedef struct Memleak{
    wg_char filename[FILE_NAME_SIZE_MAX + 1]; 
    wg_uint line;
    size_t size;
    size_t num;
    alloc_type allocation_type;
    List_head leaf;
}Memleak;

WG_PRIVATE
wg_boolean is_started(void);

pthread_mutex_t memleak_lock = PTHREAD_MUTEX_INITIALIZER;

List_head first;
wg_uint alloc_num = 0;
wg_uint free_num = 0;
wg_uint max_allocated_size = 0;
wg_uint allocated_size = 0;
wg_boolean init_flag = WG_FALSE;

void
wg_memleak_start()
{
    pthread_mutex_lock(&memleak_lock);
    if (WG_FALSE == init_flag){
        list_init(&first);
        init_flag = WG_TRUE;
        pthread_mutex_unlock(&memleak_lock);

        WG_LOG("Memleak checking starded\n");
    }else{
        pthread_mutex_unlock(&memleak_lock);
    }

    return;
}

void
wg_memleak_stop()
{
    Iterator itr;
    Memleak *leak = NULL;

    if (! is_started()){
        return;
    }

    iterator_list_init(&itr, &first, GET_OFFSET(Memleak, leaf));

    WG_DEBUG("Memleak Report\n"
             "Allocated memory blocks : %u\n"
             "Freed memory blocks     : %u\n" 
             "Maximum alloc size      : %u [B]\n" 
             "Maximum alloc size      : %u [kB]\n" ,
             alloc_num, free_num, max_allocated_size, 
             max_allocated_size / SIZE_1KB);

    while ((leak = iterator_list_next(&itr))){
        switch (leak->allocation_type){
        case MEMLEAK_MALLOC:
            WG_DEBUG("memleak: malloc addr=%p size=%u file=%s:%u\n",
                    (void*)(leak + 1), leak->size, leak->filename, leak->line);
            break;
        case MEMLEAK_CALLOC:
            WG_DEBUG("memleak: calloc addr=%p num=%u size=%u file=%s:%u\n",
                    (void*)(leak + 1), leak->num, leak->size, leak->filename, 
                    leak->line);
            break;
        default:
            WG_ERROR("BUG ! Shouldnt be executed\n");
        }

        free(leak);
    }

    return;
}

void *
wg_calloc(size_t num, size_t size, const wg_char *filename, wg_uint line)
{
    void *mem_block = NULL;
    Memleak *ml = NULL;
    size_t s = 0;

    if (! is_started()){
        return calloc(num, size);
    }

    s = (size * num) + sizeof (Memleak);
    mem_block = malloc(s);
    memset(mem_block, '\0', s);

    if (NULL != mem_block){
        ml = mem_block;
        strncpy(ml->filename, filename, FILE_NAME_SIZE_MAX);

        ml->filename[FILE_NAME_SIZE_MAX] = '\0';
        ml->size = size * num;
        ml->num  = num;
        ml->line = line;
        ml->allocation_type = MEMLEAK_CALLOC;

        pthread_mutex_lock(&memleak_lock);

        list_add(&first, &ml->leaf);
        ++alloc_num;
        allocated_size += size * num;
        max_allocated_size = WG_MAX(allocated_size, max_allocated_size);

        pthread_mutex_unlock(&memleak_lock);

        mem_block = ml + 1;
    }
    return mem_block;
}

void *
wg_malloc(size_t size, const wg_char *filename, wg_uint line)
{
    void *mem_block = NULL;
    Memleak *ml = NULL;

    if (! is_started()){
        return malloc(size);
    }
    
    mem_block = malloc(size + sizeof (Memleak));

    if (NULL != mem_block){
        ml = mem_block;
        strncpy(ml->filename, filename, FILE_NAME_SIZE_MAX);

        ml->filename[FILE_NAME_SIZE_MAX] = '\0';
        ml->size = size;
        ml->line = line;
        ml->allocation_type = MEMLEAK_MALLOC;

        pthread_mutex_lock(&memleak_lock);

        list_add(&first, &ml->leaf);
        ++alloc_num;
        allocated_size += size;
        max_allocated_size = WG_MAX(allocated_size, max_allocated_size);

        pthread_mutex_unlock(&memleak_lock);

        mem_block = ml + 1;
    }
    return mem_block;
}

void
wg_free(void *ptr)
{
    Memleak *ml = NULL;
    
    if (! is_started()){
         free(ptr);
         return;
    }

    if (NULL != ptr){
        ml = ptr;
        ml -= 1;

        pthread_mutex_lock(&memleak_lock);

        list_remove(&ml->leaf);

        allocated_size -= ml->size;

        ++free_num;

        pthread_mutex_unlock(&memleak_lock);

        free(ml);

    }

    return;
}

WG_PRIVATE
wg_boolean is_started(void)
{
    wg_boolean flag = WG_FALSE;

    pthread_mutex_lock(&memleak_lock);
    flag = (WG_TRUE == init_flag);
    pthread_mutex_unlock(&memleak_lock);

    return flag;
}
