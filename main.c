

#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>

void onOutOfMemory()
{
    printf("Out of memory reported - exiting");
    exit(-1);
}

void onIllegalOperation()
{
    printf("Illegal operation - exiting");
    exit(-2);
}


static void test_0(void **state)
{
    (void) state; // unused

    Q* q0 = createQueue();
    enqueueByte(q0, 0);
    enqueueByte(q0, 1);

    Q* q1 = createQueue();
    enqueueByte(q1, 3);
    enqueueByte(q0, 2);
    enqueueByte(q1, 4);

    assert_int_equal(dequeueByte(q0), 0);
    assert_int_equal(dequeueByte(q0), 1);

    enqueueByte(q0, 5);
    enqueueByte(q1, 6);

    assert_int_equal(dequeueByte(q0), 2);
    assert_int_equal(dequeueByte(q0), 5);

    destroyQueue(q0);

    assert_int_equal(dequeueByte(q1), 3);
    assert_int_equal(dequeueByte(q1), 4);
    assert_int_equal(dequeueByte(q1), 6);

    destroyQueue(q1);
}


static void test_1(void **state)
{
    (void) state; // unused

    Q* q0 = createQueue();
    for (int i = 0; i < 512; i++) {
        enqueueByte(q0, 42);
    }
    dequeueByte(q0);
    Q* q1 = createQueue();
    enqueueByte(q1, 42);
    destroyQueue(q0);
    destroyQueue(q1);
}



int main(void)
{
    setIllegalOperationCallback(onIllegalOperation);
    setOutOfMemoryCallback(onOutOfMemory);

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_0),
        cmocka_unit_test(test_1)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
