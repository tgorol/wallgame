#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_linked_list.h>
#include <wg_iterator.h>

struct Test_struct_1{
    int val;
    List_head node;
};

int 
initialize_func(void)
{
    return 0;
}

int
cleanup_func(void)
{
    return 0;
}

static void
test_init(void)
{
    wg_size size = 0;

    List_head head;

    list_init(&head);

    size = list_size(&head);
    CU_ASSERT_EQUAL(size, 0);

    return;
}

#define TEST_SIZE 10
static void
test_get_size(void)
{
    wg_size size = 0;
    struct Test_struct_1 ts[TEST_SIZE];

    List_head head;

    list_init(&head);

    size = list_size(&head);
    CU_ASSERT_EQUAL(size, 0);

    CU_ASSERT_TRUE(list_empty(&head));

    list_add(&head, &ts[0].node);

    CU_ASSERT_FALSE(list_empty(&head));

    size = list_size(&head);
    CU_ASSERT_EQUAL(size, 1);

    list_add(&head, &ts[1].node);

    size = list_size(&head);
    CU_ASSERT_EQUAL(size, 2);
}

static void
test_get(void)
{
    struct Test_struct_1 ts[TEST_SIZE];
    struct Test_struct_1 *ptr;

    List_head head;

    list_init(&head);

    ptr = list_get(&head, 1, struct Test_struct_1, node);
    CU_ASSERT_PTR_NULL(ptr);

    ptr = list_get(&head, 0, struct Test_struct_1, node);
    CU_ASSERT_PTR_NULL(ptr);

    ptr = list_get(&head, -1,struct Test_struct_1, node);
    CU_ASSERT_PTR_NULL(ptr);

    ptr = list_get(&head, -2, struct Test_struct_1, node);
    CU_ASSERT_PTR_NULL(ptr);

    list_add(&head, &ts[0].node);
    ptr = list_get(&head, 0, struct Test_struct_1, node);
    CU_ASSERT_PTR_NOT_NULL(ptr);

    ptr = list_get(&head, 1, struct Test_struct_1, node);
    CU_ASSERT_PTR_NULL(ptr);

    ptr = list_get(&head, -1, struct Test_struct_1, node);
    CU_ASSERT_PTR_NULL(ptr);

    ptr = list_get(&head, -2, struct Test_struct_1, node);
    CU_ASSERT_PTR_NULL(ptr);

    list_add(&head, &ts[1].node);
    ptr = list_get(&head, 0, struct Test_struct_1, node);
    CU_ASSERT_PTR_NOT_NULL(ptr);

    ptr = list_get(&head, 1, struct Test_struct_1, node);
    CU_ASSERT_PTR_NOT_NULL(ptr);

    ptr = list_get(&head, 2, struct Test_struct_1, node);
    CU_ASSERT_PTR_NULL(ptr);

    ptr = list_get(&head, -1, struct Test_struct_1, node);
    CU_ASSERT_PTR_NULL(ptr);

    ptr = list_get(&head, -2, struct Test_struct_1, node);
    CU_ASSERT_PTR_NULL(ptr);

    ptr = list_get_last(&head, struct Test_struct_1, node);
    CU_ASSERT_PTR_EQUAL(ptr, &ts[1]);

    ptr = list_get_first(&head, struct Test_struct_1, node);
    CU_ASSERT_PTR_EQUAL(ptr, &ts[0]);
}

void
test_remove(void){
    struct Test_struct_1 ts[TEST_SIZE];
    int i = 0;

    List_head head;

    list_init(&head);

    list_add(&head, &ts[0].node);

    list_remove(&ts[0].node);

    CU_ASSERT_EQUAL(list_size(&head), 0);
 
    for (i = 0 ; i < ELEMNUM(ts); ++i){
        list_add(&head, &ts[i].node);
    }

    CU_ASSERT_EQUAL(list_size(&head), ELEMNUM(ts));

    list_remove(&ts[0].node);

    CU_ASSERT_EQUAL(list_size(&head), ELEMNUM(ts) - 1);

    list_remove(&ts[1].node);

    CU_ASSERT_EQUAL(list_size(&head), ELEMNUM(ts) - 2);

    list_add(&head, &ts[0].node);
    list_add(&head, &ts[1].node);
    for (i = 0 ; i < ELEMNUM(ts); ++i){
        list_remove(&ts[i].node);
    }

    CU_ASSERT_EQUAL(list_size(&head), 0);
}


CU_TestInfo test_suit_1[] = {
    { "test_init",     test_init },
    { "test_get",      test_get },
    { "test_get_size", test_get_size },
    { "test_remove",   test_remove },
    CU_TEST_INFO_NULL
};

CU_SuiteInfo suites[] = {
    {"Test suit 1", initialize_func, cleanup_func, test_suit_1}, 
    CU_SUITE_INFO_NULL
};

int
main(int agrc, char *argv[])
{
    CU_ErrorCode error;

    error = CU_initialize_registry();
    if (CUE_SUCCESS != error){
        printf("Can not initialize CUinit registry\n");
        return WG_FAILURE;
    }
    
    CU_register_suites(suites);

    CU_basic_run_tests();

    CU_cleanup_registry();
}
