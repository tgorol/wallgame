#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_linked_list.h>
#include <wg_workqueue.h>

#include <ut_tools.h>

typedef struct Elem{
    List_head head;
    wg_int val;
}Elem;


WG_PRIVATE Elem elem[200];


UT_DEFINE(workqueue_test_1)
    WorkQ queue;    
    wg_int i = 0;
    Elem *e = NULL;

    for (i = 0; i < ELEMNUM(elem); ++i){
        list_init(&elem[i].head);
        elem[i].val = i;
    }

    UT_PASS_ON(wg_workq_init(&queue, GET_OFFSET(Elem, head)) == WG_SUCCESS)

    UT_PASS_ON(wg_workq_is_empty(&queue) == WG_TRUE)

    UT_PASS_ON(wg_workq_add(&queue, &elem[0].head) == WG_SUCCESS)

    UT_PASS_ON(wg_workq_is_empty(&queue) == WG_FALSE)

    UT_PASS_ON(wg_workq_get(&queue, (void**)&e) == WG_SUCCESS)
    UT_PASS_ON(elem[0].val == 0)

    UT_PASS_ON(wg_workq_is_empty(&queue) == WG_TRUE)

    for (i = 0; i < ELEMNUM(elem); ++i){
        UT_PASS_ON(wg_workq_add(&queue, &elem[i].head) == WG_SUCCESS)
    }
    for (i = 0; i < ELEMNUM(elem); ++i){
        UT_PASS_ON(wg_workq_get(&queue, (void**)&e) == WG_SUCCESS)
        UT_PASS_ON(elem[i].val == i)
    }

    UT_PASS_ON(wg_workq_is_empty(&queue) == WG_TRUE)

UT_END


int
main(int argc, char *argv[])
{
    UT_RUN_TEST(workqueue_test_1);

    return 0;
}
