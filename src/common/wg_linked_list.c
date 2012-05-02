#include <stdlib.h>
#include <sys/types.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_linked_list.h>
#include <wg_iterator.h>

/*! @defgroup List Linked List Implementation.
 *  @ingroup misc
 */

/*! @{ */


/*! \brief Initialise list.
 *
 * \param[in] head list to initialize.
 * \return void.
 */

void
dlist_init(List_head *head)
{
    register List_head *h;

    h = head;

    h->prev = h->next = h;

    return;
}

/*! \brief Remove an object from the list
 *
 * \param[in] object an object to remove.
 * \return void.
 */

void
dlist_remove(List_head *object)
{
    register List_head *o;

    o = object;
    o->next->prev = o->prev;
    o->prev->next = o->next;

    o->next = o->prev = o;

    return;
}

/*! \brief Add an object to the list.
 *
 * \param[in] head a head of the list.
 * \param[in] object an object to add.
 * \return void.
 */

void
dlist_add(List_head *head, List_head *object)
{
    register List_head *h;
    register List_head *o;

    o = object;
    h = head;

    h->next->prev = o;
    o->next = h->next;
    o->prev = h;
    h->next = o;

    return;
}

/*! \brief Returns WG_TRUE when the list is empty.
 *
 * \param[in] head a head of the list.
 * \retval WG_TRUE list empty.
 * \retval WG_FALSE list not empty.
 */

wg_boolean
dlist_empty(List_head *head)
{
    register List_head *h;

    h = head;

    return h->next == h;
}

/*! \brief Returns number of object in the list.
 *
 * \param[in] head a head of the list.
 * \return number of object in the list.
 */

wg_size
dlist_size(List_head *head)
{
    wg_size i;
    register List_head *h;

    h = head->next;

    for(i = 0; head != h; ++i) h = h->next;

    return i;
}

/*! \brief Call a function for every object in the list.
 *
 * If objects of the list are dinamicly allocated the callback must not free them.
 * Use dlist_forall_safe instead.
 *
 * \param[in] head a head of the list.
 * \param[in] f callback function.
 * \param[in] user_data user data passed to each callback
 * \param[in] offset numbers of bytes from start of the object structure to the List_head structure.
 * \return void.
 */

void
dlist_forall(List_head *head, void (*f)(void*, void*),
        void * user_data, wg_int offset)
{
    register List_head *h;

    h = head->next;

    while(head != h)
    {
        f(GET_CONTAINER(h, offset), user_data);
        h = h->next;
    }

    return;
}
/*! \brief Call a function for every object in the list.
 *
 * If objects of the list are dinamicly allocated the callback can free them.
 *
 * \param[in] head a head of the list.
 * \param[in] f callback function.
 * \param[in] user_data user data passed to each callback
 * \param[in] offset numbers of bytes from start of the object structure to the List_head structure.
 * \return void.
 */

void
dlist_forall_safe(List_head *head, void (*f)(void*, void*),
        void *user_data, wg_int offset)
{
    register List_head *h;
    List_head *tmp;

    h = head->next;

    while(head != h)
    {
        tmp = h->next;
        f(GET_CONTAINER(h, offset), user_data);
        h = tmp;
    }

    return;
}


/*! \brief Get n'th element from the list.
 *
 * \param[in] head a head of the list.
 * \param[in] n index of the element to get.
 * \param[in] offset numbers of bytes from start of the object structure to the List_head structure.
 * \return object pointer or NULL if error. Error occurs only if index out of bound of the list.
 */

void *
dlist_get(List_head *head, wg_uint n, wg_int offset)
{
    register List_head *h;

    h = head->next;

    ++n;

    while((head != h) && (--n)) h = h->next;

    return (head == h) ? NULL : GET_CONTAINER(h, offset);
}

/*! \brief Get last element from the list.
 *
 * \param[in] head a head of the list.
 * \param[in] offset numbers of bytes from start of the object structure to the List_head structure.
 * \return object pointer or NULL if list empty.
 */

void *
dlist_get_last(List_head *head, wg_int offset)
{
    register List_head *h;

    h = head->next;

    return h != head ? GET_CONTAINER(h, offset) : NULL;
}

/*! \brief Get first element from the list.
 *
 * \param[in] head a head of the list.
 * \param[in] offset numbers of bytes from start of the object structure to the List_head structure.
 * \return object pointer or NULL if list empty.
 */

void *
dlist_get_first(List_head *head, wg_int offset)
{
    register List_head *h;

    h = head->prev;

    return h != head ? GET_CONTAINER(h, offset) : NULL;
}


/**
 * @brief Convert list to an array
 *
 * @param head   a head of the list
 * @param array  array to store data in
 * @param[in] offset numbers of bytes from start of the object structure to the List_head structure.
 *
 * @return 
 */
wg_uint
dlist_to_array(List_head *head, void **array, wg_int offset)
{
    Iterator itr;
    wg_size index = 0;
    void *elem = NULL;

    iterator_list_init(&itr, head, offset);

    while ((elem = iterator_list_next(&itr)) != NULL){
        array[index++] = elem;
    }

    return index;
}

/**
 * @brief Get and remove first element of the list
 *
 * @param head   head of the list
 * @param offset numbers of bytes from start of the object structure to the List_head structure.
 *
 * @return pinter to the element or NULL if empty
 */
void *
dlist_pop_first(List_head *head, wg_int offset)
{
    register List_head *h;

    h = head->prev;

    h->prev->next = h->next;
    h->next->prev = h->prev;

    return h != head ? GET_CONTAINER(h, offset) : NULL;
}

/**
 * @brief Fist head after shallow copy
 *
 * @param head head of the list
 */
void 
dlist_fix_after_copy(List_head *head)
{
    head->next->prev = head;
    head->prev->next = head;

    return;
}

/*! @} */
