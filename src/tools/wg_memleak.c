#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>

#include <wg_linked_list.h>
#include <wg_iterator.h>

/*! @defgroup tools Tools
 */

/*! @defgroup memleak Memory leak checker
 *  @ingroup tools
 */

/*! @{ */

/** 
* @brief Maximum size of the file name stored by Memleak structure
*/
#define FILE_NAME_SIZE_MAX  32

/** 
* @brief Number of bytes allocated to trigger release thread
*/
#define FREE_SIZE_THRESHOLD (SIZE_1KB * 10000)

/** 
* @brief Number of allocation to trigger release thread
*/
#define FREE_NUM_THRESHOLD  512

#define SIGNATURE        'T'

/** 
* @brief Allocation type
*/
typedef enum alloc_type{
    MEMLEAK_INVALID = 0,  /*!< invalid allocation    */
    MEMLEAK_MALLOC     ,  /*!< allocated with malloc */
    MEMLEAK_CALLOC        /*!< allocated with calloc */
}alloc_type;

/** 
* @brief Memory block header
*/
typedef struct Memleak{
    wg_char filename[FILE_NAME_SIZE_MAX + 1];  /*!< file name where allocated */
    wg_uint line;                   /*!< line number in the file    */
    size_t size;                    /*!< number of allocated bytes  */
    size_t num;                     /*!< number of blocks allocated */
    alloc_type allocation_type;     /*!< allocation type            */
    List_head leaf;                 /*!< list leaf                  */
    wg_boolean last_block;          /*!< indicates last block       */
}Memleak;


WG_PRIVATE wg_boolean is_started(void);

/** 
* @brief Memleak lock
*/
WG_PRIVATE pthread_mutex_t memleak_lock = PTHREAD_MUTEX_INITIALIZER;

/** 
* @brief Free thread instance
*/
WG_PRIVATE pthread_t free_thread;

/** 
* @brief Threshold free thread wake up condition
*/
WG_PRIVATE pthread_cond_t free_cond = PTHREAD_COND_INITIALIZER;

/** 
* @brief Allocated memory list
*/
WG_PRIVATE List_head first;

/** 
* @brief Release memory list
*/
WG_PRIVATE List_head free_list;

/** 
* @brief Number of allocations
*/
WG_PRIVATE wg_uint alloc_num = 0;

/** 
* @brief Number of releases
*/
WG_PRIVATE wg_uint free_num = 0;

/** 
* @brief Peak size of memory allocated
*/
WG_PRIVATE wg_uint max_allocated_size = 0;

/** 
* @brief Allocated memory size counter
*/
WG_PRIVATE wg_uint allocated_size = 0;

/** 
* @brief Multiple initialization flag
* 
* @return 
*/
WG_PRIVATE wg_boolean init_flag = WG_FALSE;

/** 
* @brief Number of releases in the release list
*/
WG_PRIVATE wg_uint free_count = 0;

/** 
* @brief Entire size of memory to release in release list
*/
WG_PRIVATE wg_uint free_size = 0;

/** 
* @brief Release memory thread
*
*  This thread gets wake up on threshold
* 
* @param data
* 
* @return 
*/
WG_PRIVATE void *
free_thread_func(void *data)
{
    Memleak *ml = NULL;
    Iterator itr;
    wg_boolean exit_flag = WG_FALSE;

    pthread_mutex_lock(&memleak_lock);
    for (;exit_flag != WG_TRUE;){

        /* wait for waking up */
        pthread_cond_wait(&free_cond, &memleak_lock);

        iterator_list_init(&itr, &free_list, GET_OFFSET(Memleak, leaf));

        /* release all block grom free_list */
        while ((ml = iterator_list_next(&itr)) != NULL){
            if (ml->last_block == WG_TRUE){
                exit_flag = WG_TRUE;
            }
            --free_count;
            free_size -= ml->size;
            list_remove(&ml->leaf);
            free(ml);
        }
    }

    pthread_mutex_unlock(&memleak_lock);

    return data;
}

/** 
* @brief Start memory leak checked
*/
void
wg_memleak_start(void)
{
    pthread_attr_t attr;
    pthread_mutex_lock(&memleak_lock);
    if (WG_FALSE == init_flag){
        init_flag = WG_TRUE;
        list_init(&first);
        list_init(&free_list);

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_create(&free_thread, &attr, free_thread_func, NULL);
        pthread_attr_destroy(&attr);

        pthread_mutex_unlock(&memleak_lock);

        WG_DEBUG("Memleak checking starded\n");
    }else{
        pthread_mutex_unlock(&memleak_lock);
    }

    return;
}

/** 
* @brief Stop memory leak checker
*
*  Release all memory allocated and print status about memory leaks.
*/
void
wg_memleak_stop()
{
    Iterator itr;
    Memleak *leak = NULL;
    wg_uint memleaks_num = 0;

    if (! is_started()){
        return;
    }

    /* create a dummy allocation to teel free thread to finish */
    leak = wg_malloc(1, __FILE__, __LINE__);
    leak[-1].last_block = WG_TRUE;

    wg_free(leak, WG_TRUE);

    /* wait for free thread completion */
    pthread_join(free_thread, NULL);

    iterator_list_init(&itr, &first, GET_OFFSET(Memleak, leaf));

    WG_DEBUG("Memleak Report\n"
             "Allocated memory blocks : %u\n"
             "Freed memory blocks     : %u\n" 
             "Maximum alloc size      : %u [B]\n" 
             "Maximum alloc size      : %u [kB]\n" ,
             alloc_num, free_num, max_allocated_size, 
             max_allocated_size / SIZE_1KB);

    memleaks_num = list_size(&first);
    if (memleaks_num == 0){
        WG_DEBUG("No memory leaks.\n");
    }else{
        WG_DEBUG("%u memory leaks.\n", memleaks_num);
    }

    /* print info about each leak */
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

/** 
* @brief Calloc
* 
* @param num       number of blocks to allocate
* @param size      size of block to allocate
* @param filename  file name of allocation
* @param line      line number of allocation
* 
* @return memory pointer or NULL if error
*/
void *
wg_calloc(size_t num, size_t size, const wg_char *filename, wg_uint line)
{
    void *mem_block = NULL;
    Memleak *ml = NULL;
    size_t s = 0;

    if (! is_started()){
        return calloc(num, size);
    }

    /* allocate requestes memory size + header size */
    s = (size * num) + sizeof (Memleak) + sizeof (SIGNATURE);
    mem_block = malloc(s);
    memset(mem_block, '\0', s);

    if (NULL != mem_block){
        /* fill header */
        ml = mem_block;
        strncpy(ml->filename, filename, FILE_NAME_SIZE_MAX);

        ml->filename[FILE_NAME_SIZE_MAX] = '\0';
        ml->size = size;
        ml->num  = num;
        ml->line = line;
        ml->allocation_type = MEMLEAK_CALLOC;
        ml->last_block = WG_FALSE;

        pthread_mutex_lock(&memleak_lock);

        /* add block to allocation list */
        list_add(&first, &ml->leaf);
        ++alloc_num;
        allocated_size += size * num;
        max_allocated_size = WG_MAX(allocated_size, max_allocated_size);

        pthread_mutex_unlock(&memleak_lock);

        mem_block = ml + 1;
	((wg_char*)mem_block)[size * num] = SIGNATURE;
    }
    return mem_block;
}

/** 
* @brief Malloc
* 
* @param size      number of bytes to allocate
* @param filename  file name of allocation
* @param line      line number of allocation
* 
* @return memory allocated pointer or NULL on error
*/
void *
wg_malloc(size_t size, const wg_char *filename, wg_uint line)
{
    void *mem_block = NULL;
    Memleak *ml = NULL;
    wg_uint real_size = 0;

    if (! is_started()){
        return malloc(size);
    }
    
    /* allocate memory size + header size */
    real_size = size + sizeof (Memleak) + sizeof (SIGNATURE);
    mem_block = malloc(real_size);
    memset(mem_block, SIGNATURE, real_size);

    if (NULL != mem_block){
        /* fill header */
        ml = mem_block;
        strncpy(ml->filename, filename, FILE_NAME_SIZE_MAX);

        ml->filename[FILE_NAME_SIZE_MAX] = '\0';
        ml->size = size;
        ml->line = line;
        ml->num  = 1;
        ml->allocation_type = MEMLEAK_MALLOC;
        ml->last_block = WG_FALSE;

        pthread_mutex_lock(&memleak_lock);

        /* add allocated block to allocation list */
        list_add(&first, &ml->leaf);
        ++alloc_num;
        allocated_size += size;
        max_allocated_size = WG_MAX(allocated_size, max_allocated_size);

        pthread_mutex_unlock(&memleak_lock);

        mem_block = ml + 1;
    }
    return mem_block;
}

/** 
* @brief Free memory
*
* Puts memory to release list. If ff == WG_TRUE starts release thread without
* checking thresholds.
* 
* @param ptr pointer to release
* @param ff  force free
*/
void
wg_free(void *ptr, wg_boolean ff)
{
    Memleak *ml = NULL;
    wg_uint size = 0;
    wg_uint signature_index = 0;
    wg_char *char_ptr = NULL;
    
    if (! is_started()){
         free(ptr);
         return;
    }

    if (NULL != ptr){
        ml = ptr;
        char_ptr = ptr;
        ml -= 1;

	signature_index = ml->size * ml->num;
        if (char_ptr[signature_index] != SIGNATURE){
            WG_ERROR("Out of bound access: %s\n", ml->filename);
            exit(1);
        }
 
        size = ml->size * ml->num;
        pthread_mutex_lock(&memleak_lock);

        ++free_num;
        ++free_count;
        free_size += size;
        allocated_size -= size;

        list_remove(&ml->leaf);

        list_add(&free_list, &ml->leaf);

        /* wake up release thread if threshold reached or forced free */
        if ((free_num >= FREE_NUM_THRESHOLD)   || 
            (free_size >= FREE_SIZE_THRESHOLD) ||
            (ff == WG_TRUE)){
            pthread_cond_signal(&free_cond);
        }

        pthread_mutex_unlock(&memleak_lock);
    }

    return;
}

/** 
* @brief Multiple initialization protection
* 
* @return 
*/
WG_PRIVATE
wg_boolean is_started(void)
{
    wg_boolean flag = WG_FALSE;

    pthread_mutex_lock(&memleak_lock);
    flag = (WG_TRUE == init_flag);
    pthread_mutex_unlock(&memleak_lock);

    return flag;
}

/*! @} */
