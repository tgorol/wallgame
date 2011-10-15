#ifndef _LLITERATOR_H
#define _LLITERATOR_H


WG_PUBLIC wg_boolean iterator_list_is_last(Iterator*);

WG_PUBLIC wg_boolean iterator_list_is_first(Iterator*);

WG_PUBLIC void* iterator_list_next(Iterator*);

WG_PUBLIC void iterator_list_init(Iterator *, List_head *, wg_uint);

WG_PUBLIC void* iterator_list_prev(Iterator *iterator);

#endif
