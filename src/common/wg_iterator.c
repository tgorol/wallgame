#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_linked_list.h>
#include <wg_iterator.h>

/** @defgroup Iterator Linked List Iterator
 */

/*! @{ */

/**
 * @brief Initialize iterator
 *
 * @param iterator iterator to initialize
 * @param list     list to create iterator for
 * @param offset   linked list offset
 */
void
iterator_list_init(Iterator *iterator, List_head *list, wg_uint offset)
{
    memset(iterator, 0, sizeof (*iterator));

    iterator->list = list;
    iterator->next = list->next;
    iterator->prev = list->prev;
    iterator->offset = offset;

    return;
}

/**
 * @brief Get previous object
 *
 * @param iterator Iterator
 *
 * @return object pointer or NULL if no more objects
 */
void*
iterator_list_prev(Iterator *iterator)
{
    void *obj;

    if ( ! iterator_list_is_first(iterator)){
        obj = GET_CONTAINER(iterator->next, iterator->offset);
        iterator->next = ((List_head*)iterator->next)->next;
    }else{
        obj = NULL;
    }

    return obj;
}

/**
 * @brief Get next element
 *
 * @param iterator Iterator
 *
 * @return object pointer or NULL if no more objects
 */
void*
iterator_list_next(Iterator *iterator)
{
    void *obj;

    if ( ! iterator_list_is_last(iterator)){
        obj = GET_CONTAINER(iterator->prev, iterator->offset);
        iterator->prev = ((List_head*)iterator->prev)->prev;
    }else{
        obj = NULL;
    }

    return obj;
}

/**
 * @brief Check if the last element
 *
 * @param iterator Iterator
 *
 * @retval WG_YES
 * @retval WG_NO
 */
wg_boolean
iterator_list_is_last(Iterator *iterator)
{
    return iterator->prev == iterator->list;
}

/**
 * @brief Check if first element
 *
 * @param iterator Iterator
 *
 * @retval WG_YES
 * @retval WG_NO
 */
wg_boolean
iterator_list_is_first(Iterator *iterator)
{
    return iterator->next == iterator->list;
}

/*! @} */
