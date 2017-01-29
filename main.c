

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

/////////////////////////////////////////////////////////////////////////////

static void test_0(void **state)
{
    (void) state; // unused
    resetErrors();

    Q* q0 = createQueue();
    assert_non_null(q0);
    enqueueByte(q0, 0);
    enqueueByte(q0, 1);

    Q* q1 = createQueue();
    assert_non_null(q1);
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

    Q* q3 = createQueue();
    destroyQueue(q3);

    q3 = createQueue();
    enqueueByte(q3,0);
    destroyQueue(q3);

    q3 = createQueue();
    enqueueByte(q3,0);
    assert_int_equal(dequeueByte(q3), 0);
    destroyQueue(q3);

    q3 = createQueue();
    enqueueByte(q3,0);
    assert_int_equal(dequeueByte(q3), 0);
    enqueueByte(q3,0);
    destroyQueue(q3);

    q3 = createQueue();
    enqueueByte(q3,0);
    assert_int_equal(dequeueByte(q3), 0);
    enqueueByte(q3,0);
    assert_int_equal(dequeueByte(q3), 0);
    destroyQueue(q3);

    q3 = createQueue();
    enqueueByte(q3,0);
    enqueueByte(q3,0xff);
    assert_int_equal(dequeueByte(q3), 0);
    assert_int_equal(dequeueByte(q3), 0xFF);
    enqueueByte(q3,0);
    assert_int_equal(dequeueByte(q3), 0);
    destroyQueue(q3);

    q3 = createQueue();
    enqueueByte(q3,0);
    enqueueByte(q3,0xff);
    assert_int_equal(dequeueByte(q3), 0);
    assert_int_equal(dequeueByte(q3), 0xFF);
    enqueueByte(q3,0);
    assert_int_equal(dequeueByte(q3), 0);
    destroyQueue(q3);

    assert_int_equal(has_out_of_mem, 0);
    assert_int_equal(has_illegal_op, 0);
}

static void test_1(void **state)
{
    (void) state; // unused

    resetErrors();

    Q* q0 = createQueue();
    assert_non_null(q0);

    for (int i = 0; i < 1021; i++) {
        enqueueByte(q0, 42);
    }

    for (int i = 0; i < 4; i++)
        dequeueByte(q0);

    Q* q1 = createQueue();
    enqueueByte(q1, 42);
    enqueueByte(q1, 255);
    enqueueByte(q1, 0);

    destroyQueue(q0);

    assert_int_equal(dequeueByte(q1), 42);
    assert_int_equal(dequeueByte(q1), 255);
    assert_int_equal(dequeueByte(q1), 0);

    destroyQueue(q1);

    Q* q2 = createQueue();
    for (int i = 0; i < 256; i++) {
        enqueueByte(q2, i);
    }

    for (int i = 0; i < 256; i++) {
        assert_int_equal(dequeueByte(q2), i);
    }

    destroyQueue(q2);

    assert_int_equal(has_out_of_mem, 0);
    assert_int_equal(has_illegal_op, 0);

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

    Q* full_q[511];

    for (int i = 0; i < 511; i++) {
        Q* q = createQueue();
        assert_non_null(q);
        full_q[i] = q;
    }

    for (int i = 0; i < 511; i++) {
        destroyQueue(full_q[i]);
    }

    for (int i = 0; i < 511; i++) {
        full_q[i] = createQueue();
    }

    for (int i = 0; i < 511; i++) {
        enqueueByte(full_q[i], i);
    }

    for (int i = 0; i < 511; i++) {
        destroyQueue(full_q[i]);
    }

    for (int i = 0; i < 511; i++) {
        full_q[i] = createQueue();
    }

    for (int i = 0; i < 511; i++) {
        enqueueByte(full_q[i], i);
    }

    for (int i = 0; i < 511; i++) {
        dequeueByte(full_q[i]);
    }

    for (int i = 0; i < 511; i++) {
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
    destroyQueue(q0);


    // mem out
    resetErrors();

    Q* full_q[511];
    for (int i = 0; i < 511; i++) {
        full_q[i] = createQueue();
    }

    Q* out = createQueue();
    (void)out;
    assert_int_equal(has_out_of_mem, 1);

    for (int i = 0; i < 511; i++) {
        destroyQueue(full_q[i]);
    }

    resetErrors();

}

static void test_5(void **state) // random load
{
    (void) state; // unused

#define LENN 512

    unsigned char correct[LENN];
    Q* qs[16];
    int qs_l[16];

    for (int t = 0; t < 100; t++)
    {

        resetErrors();

        Q* in = createQueue();
        Q* out = createQueue();
        int in_l = 0;
        int op_cnt = 0;

        for (int i = 0; i < LENN; i++) {
            unsigned char b = rand()%256;
            correct[i] = b;
            enqueueByte(in, b);
            in_l++;
        }


        for (int i = 0; i < 16; i++) {
            qs[i] = createQueue();
            qs_l[i] = 0;
        }

        // random perturbatinos
        while(1)
        {
            unsigned char from;
            if (rand()%1000 == 0 && in_l > 0) {
                from = dequeueByte(in);
                assert_int_equal(has_illegal_op, 0);
                in_l--;
                op_cnt++;
            } else {
                int idx = rand()%16;
                int cnt = 0;
                while (qs_l[idx] == 0) {
                    idx = (idx + 1) % 16;
                    if (cnt++ > 16)
                        break;
                }
                if (cnt > 16) {
                    if (in_l > 0) {
                        from = dequeueByte(in);
                        in_l--;
                        op_cnt++;
                    }
                    else
                        break;
                } else
                {
                    from = dequeueByte(qs[idx]);
                    op_cnt++;
                    assert_int_equal(has_illegal_op, 0);
                    qs_l[idx]--;
                }
            }

            if (rand()%1000 == 0) {
                enqueueByte(out, from);
                op_cnt++;
            } else {
                int idx = rand()%16;
                enqueueByte(qs[idx], from);
                qs_l[idx]++;
                op_cnt++;
            }

        }

        destroyQueue(in);

        for (int i = 0; i < 16; i++) {
            destroyQueue(qs[i]);
        }

        // check
        for (int i = 0; i < LENN; i++) {
            unsigned char b = dequeueByte(out);
            assert_int_equal(has_illegal_op, 0);
            for (int q = 0; q < LENN; q++) {
                if (correct[q] == b)
                    correct[q] = 0;
            }
        }
        for (int i = 0; i < LENN; i++) {
            assert_int_equal(correct[i], 0);
        }
        destroyQueue(out);

        printf("> tested %d operatins\n", op_cnt);

        assert_int_equal(has_out_of_mem, 0);
        assert_int_equal(has_illegal_op, 0);

    }

    resetErrors();

}

/////////////////////////////////////////////////////////////////////////////

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
        cmocka_unit_test(test_4), // limits stress
        cmocka_unit_test(test_0), // sanyty check after stress
        cmocka_unit_test(test_5), // random stress
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
