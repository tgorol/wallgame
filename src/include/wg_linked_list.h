#ifndef _WG_LIST_H
#define _WG_LIST_H


#define GET_CONTAINER(obj, off) ((void*)(((char*)(obj))-(off)))

#define GET_OFFSET(type, field) ((int)(&((type*)0)->field))

typedef struct List_head List_head;

/*! \brief Double linked list structure.
 */
struct List_head{
    List_head *next;               /*!< next element in the list. */
    List_head *prev;               /*!< previous element in the list */
};

/**
 * @brief Linked list iterator
 */
typedef struct Iterator{
    void *next;       /*!< next element in the list     */
    void *prev;       /*!< previuos element in the list */
    void *list;       /*!< head of the list             */
    wg_uint offset;   /*!< list element offset          */
    void *reserved;   /*!< reserved                     */
}Iterator;

WG_PUBLIC void dlist_init(List_head *);
WG_PUBLIC void dlist_remove(List_head *);
WG_PUBLIC void dlist_add(List_head *, List_head *);
WG_PUBLIC wg_boolean dlist_empty(List_head *);
WG_PUBLIC wg_size dlist_size(List_head *);
WG_PUBLIC void dlist_forall(List_head *, void (*)(void*, void*),void *, int);
WG_PUBLIC void *dlist_get(List_head *, wg_uint, int );
WG_PUBLIC void *dlist_get_last(List_head *, int );
WG_PUBLIC void dlist_forall_safe(List_head *, void (*)(void*, void*),void *, int);
WG_PUBLIC wg_uint dlist_to_array(List_head *head, void **array, wg_int offset);
WG_PUBLIC void * dlist_get_first(List_head *head, wg_int offset);
WG_PUBLIC void * dlist_pop_first(List_head *head, wg_int offset);

#define list_init(head)                                            \
    dlist_init(head)

#define list_add(head, elem)                                       \
    dlist_add(head, elem)

#define list_forall(head, type, field, func, udata)                \
    dlist_forall(head, func, udata, GET_OFFSET(type, field))

#define list_forall_safe(head, type, field, func, udata)           \
    dlist_forall_safe(head, func, udata, GET_OFFSET(type, field))

#define list_size(head)                                            \
    dlist_size(head)

#define list_empty(head)                                           \
    dlist_empty(head)

#define list_get(head, n, type, field)                             \
    dlist_get(head, n, GET_OFFSET(type, field))

#define list_pop_first(head, type, field)                          \
    dlist_get(head, GET_OFFSET(type, field))

#define list_get_last(head, type, field)                           \
    dlist_get(head, GET_OFFSET(type, field)

#define list_get_first(head, type, field)                           \
    dlist_get(head, GET_OFFSET(type, field)

#define list_remove(head)                                          \
            dlist_remove(head)

#define list_to_array(head, type, field,  array)                   \
            dlist_to_array(head, array, GET_OFFSET(type, field))


#endif /* _WG_LIST_H */
