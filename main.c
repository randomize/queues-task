

#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>


#define BUFFER_LIMIT 2048
static unsigned char buffer[BUFFER_LIMIT];


static int has_out_of_mem;
static int has_illegal_op;
static int max_nodes;

void onOutOfMemory()
{
    /* printf("Out of memory reported - exiting"); */
    has_out_of_mem = 1;
}


void onIllegalOperation()
{
    /* printf("Illegal operation - exiting"); */
    has_illegal_op = 1;
}

void resetErrors()
{
    has_illegal_op = 0;
    has_out_of_mem = 0;
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


static void test_2(void **state)
{
    (void) state; // unused

    Q* q0 = createQueue();
    enqueueByte(q0, 5);
    enqueueByte(q0, 1);

    Q* q1 = createQueue();
    enqueueByte(q1, 3);
    enqueueByte(q0, 2);
    enqueueByte(q1, 4);

    assert_int_equal(dequeueByte(q1), 3);
    assert_int_equal(dequeueByte(q0), 5);

    enqueueByte(q0, 5);
    enqueueByte(q1, 6);

    destroyQueue(q0);

    assert_int_equal(dequeueByte(q1), 4);

    destroyQueue(q1);
}

static void test_3(void **state) // stress
{
    (void) state; // unused

    Q* full_q[512];

    for (int i = 0; i < 512; i++) {
        full_q[i] = createQueue();
    }

    for (int i = 0; i < 512; i++) {
        destroyQueue(full_q[i]);
    }

    for (int i = 0; i < 512; i++) {
        full_q[i] = createQueue();
    }

    for (int i = 0; i < 512; i++) {
        enqueueByte(full_q[i], i);
    }

    for (int i = 0; i < 512; i++) {
        destroyQueue(full_q[i]);
    }

    for (int i = 0; i < 512; i++) {
        full_q[i] = createQueue();
    }

    for (int i = 0; i < 512; i++) {
        enqueueByte(full_q[i], i);
    }

    for (int i = 0; i < 512; i++) {
        dequeueByte(full_q[i]);
    }

    for (int i = 0; i < 512; i++) {
        destroyQueue(full_q[i]);
    }


}

static void test_4(void **state)
{
    (void) state; // unused
    resetErrors();

    Q* q0 = createQueue();
    dequeueByte(q0);

    assert_int_equal(has_illegal_op, 1);

    resetErrors();

}

int main(void)
{
    max_nodes = initQueues(buffer, BUFFER_LIMIT);
    setIllegalOperationCallback(onIllegalOperation);
    setOutOfMemoryCallback(onOutOfMemory);

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_0),
        cmocka_unit_test(test_1),
        cmocka_unit_test(test_2),
        cmocka_unit_test(test_3),
        cmocka_unit_test(test_0), // sanyty check after stress
        cmocka_unit_test(test_4),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
