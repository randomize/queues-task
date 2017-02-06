

#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>


#define BUFFER_LIMIT 2048
static unsigned char buffer[BUFFER_LIMIT];


static int has_out_of_mem;
static int has_illegal_op;
static queueMetrics_t metrics;

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

    // sequential push-pop
    for (int j = 0; j < metrics.max_els_in_single; j++)
    {
        for (int i = 0; i < j; i++)
            enqueueByte(q0, i);

        for (int i = 0; i < j; i++)
        {
            unsigned char d = dequeueByte(q0);
            unsigned char c = i;
            if (d != c)
                printf("%d) step %d expected: %d but got %d\n", j, i, c, d);
            assert_int_equal(d, c);
        }
    }

    // randomzised push-pop
    for (int j = 0; j < 10000; j++)
    {
        int l = rand() % metrics.max_els_in_single;

        for (int i = 0; i < l; i++)
            enqueueByte(q0, i);

        for (int i = 0; i < l; i++)
        {
            unsigned char d = dequeueByte(q0);
            unsigned char c = i;
            if (d != c)
                printf("%d) randstep %d expected: %d but got %d\n", l, i, c, d);
            assert_int_equal(d, c);
        }
    }

    destroyQueue(q0);


    Q* q1 = createQueue();

    // same byte series
    for (int j = 0; j < metrics.max_els_in_single; j++)
    {
        for (int i = 0; i < j; i++)
            enqueueByte(q0, j);

        for (int i = 0; i < j; i++)
        {
            unsigned char d = dequeueByte(q0);
            unsigned char c = j;
            if (d != c)
                printf("%d) step %d expected: %d but got %d\n", j, i, c, d);
            assert_int_equal(d, c);
        }
    }

    // same byte random series
    for (int j = 0; j < 10000; j++)
    {
        unsigned char b = rand();
        int l = rand() % metrics.max_els_in_single;
        for (int i = 0; i < l; i++)
        {
            enqueueByte(q1, b);
        }
        for (int i = 0; i < l; i++)
        {
            unsigned char d = dequeueByte(q1);
            if (d != b)
                printf("%d) series-push %d expected: %d but got %d\n", l, i, b, d);
            assert_int_equal(d, b);
        }

    }

    Q* q2 = createQueue();

    for (int i = 0; i < metrics.max_els_in_single_with_63_empty; i++)
        enqueueByte(q2, i);

    for (int i = 0; i < metrics.max_els_in_single_with_63_empty; i++)
        assert_int_equal(dequeueByte(q2), i % 256);

    for (int i = 0; i < metrics.max_els_in_single_with_63_empty; i++)
        enqueueByte(q1, i);

    destroyQueue(q2);
    destroyQueue(q1);

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


    // test empty queues max

    resetErrors();

    const int MAX_EQ = metrics.max_empty_queues;

    Q* full_eq[MAX_EQ];

    for (int i = 0; i < MAX_EQ; i++)
        full_eq[i] = createQueue();

    assert_int_equal(has_out_of_mem, 0);
    assert_int_equal(has_illegal_op, 0);

    for (int i = 0; i < MAX_EQ; i++)
        destroyQueue(full_eq[i]);

    assert_int_equal(has_out_of_mem, 0);
    assert_int_equal(has_illegal_op, 0);

    // randomize destroy (same, but shuffled)

    resetErrors();

    for (int i = 0; i < MAX_EQ; i++)
        full_eq[i] = createQueue();

    assert_int_equal(has_out_of_mem, 0);
    assert_int_equal(has_illegal_op, 0);

    for(int i = MAX_EQ - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        Q* t = full_eq[i]; full_eq[i] = full_eq[j]; full_eq[j] = t;
    }

    for (int i = 0; i < MAX_EQ; i++)
        destroyQueue(full_eq[i]);

    assert_int_equal(has_out_of_mem, 0);
    assert_int_equal(has_illegal_op, 0);

    // test max even queues

    resetErrors();

    const int MAX_Q = metrics.max_nonempty_queues;
    const int MAX_EL = metrics.max_els_in_max_even_queues;
    Q* full_q[MAX_Q];


    for (int i = 0; i < MAX_Q; i++)
        full_q[i] = createQueue();

    for (int i = 0; i < MAX_Q; i++)
        for (int j = 0; j < MAX_EL; j++)
            enqueueByte(full_q[i], i);

    for (int i = 0; i < MAX_Q; i++)
        destroyQueue(full_q[i]);

    assert_int_equal(has_out_of_mem, 0);
    assert_int_equal(has_illegal_op, 0);

    for (int i = 0; i < MAX_Q; i++)
        full_q[i] = createQueue();

    for (int i = 0; i < MAX_Q; i++)
        for (int j = 0; j < MAX_EL; j++)
            enqueueByte(full_q[i], i);

    for (int i = 0; i < MAX_Q; i++)
        for (int j = 0; j < MAX_EL; j++)
            assert_int_equal(dequeueByte(full_q[i]), i % 256);

    for (int i = 0; i < MAX_Q; i++)
        destroyQueue(full_q[i]);

    assert_int_equal(has_out_of_mem, 0);
    assert_int_equal(has_illegal_op, 0);
    resetErrors();


}

static void test_4(void **state)
{
    (void) state; // unused

    // pop empty
    resetErrors();

    Q* q0 = createQueue();
    dequeueByte(q0);

    assert_int_equal(has_illegal_op, 1);
    destroyQueue(q0);


    // mem out
    resetErrors();

    const int MAX_Q = metrics.max_empty_queues;
    Q* full_q[MAX_Q];

    for (int i = 0; i < MAX_Q; i++)
        full_q[i] = createQueue();

    // try one more
    Q* out = createQueue();
    (void)out;

    assert_int_equal(has_out_of_mem, 1);

    for (int i = 0; i < MAX_Q; i++) {
        destroyQueue(full_q[i]);
    }

    // mem out2
    Q* q1 = createQueue();

    for (int i = 0; i < metrics.max_els_in_single; i++)
        enqueueByte(q1, i);

    // try one more
    enqueueByte(q1, 0);
    assert_int_equal(has_out_of_mem, 1);

    destroyQueue(q1);

    resetErrors();


}

static void test_5(void **state) // random load
{
    (void) state; // unused

#define LENN 512

    unsigned char correct[LENN];
    Q* qs[16];
    int qs_l[16];
    int op_cnt = 0;

    for (int t = 0; t < 100; t++)
    {

        resetErrors();

        Q* in = createQueue();
        Q* out = createQueue();
        int in_l = 0;

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
            if (rand()%2000 == 0 && in_l > 0) {
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

            if (rand()%2000 == 0) {
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


        assert_int_equal(has_out_of_mem, 0);
        assert_int_equal(has_illegal_op, 0);

    }

    resetErrors();

    printf("> tested %d operatins\n", op_cnt);
}

static void test_6(void **state) // bug with destroy
{

    (void) state; // unused

    resetErrors();

    for (int j = 0; j < metrics.max_els_in_single; j++)
    {
        Q* q0 = createQueue();
        assert_non_null(q0);

        for (int i = 0; i < 14; i++)
        {
            enqueueByte(q0, i);
        }
        destroyQueue(q0);
        
    }

    assert_int_equal(has_out_of_mem, 0);
    assert_int_equal(has_illegal_op, 0);
    resetErrors();

}

/////////////////////////////////////////////////////////////////////////////

static void perf_test_0()
{

    const int MAX_N = 1500;
    long results[MAX_N];
    int s = 0; // optimization killer

    struct timespec begin, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &begin); // warmup clock
    Q* q = createQueue();
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    s += (begin.tv_nsec - end.tv_nsec);

    for (int i = 0; i < MAX_N; i++)
    {
        unsigned char ii = i;
        /* enqueueByte(q, ii); */
        /* s += dequeueByte(q); */
        /* for (int j = 0; j < BUFFER_LIMIT; j++) { s += buffer[i]; } // warm cache */
        /* s += (begin.tv_nsec - end.tv_nsec); */
        /* s += buffer[0]; */
        /* s += buffer[8]; */

        clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
        enqueueByte(q, ii);
        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
        results[i] = end.tv_nsec - begin.tv_nsec;
    }

    int min = 0;
    int max = 0;
    long long sum = 0;
    FILE* f = fopen("bench_0.txt", "wt");
    for (int i = 0; i < MAX_N; i++)
    {
        long diff = results[i];
        if (results[max] < diff) max = i;
        if (results[min] > diff) min = i;
        sum += diff;
        fprintf(f, "%lu ", diff);
    }
    fclose(f);


    printf("min: %lu at %d\nmax %lu at %d\navg: %f\ns=%d\n",
            results[min], min,
            results[max], max,
            sum / (double)MAX_N,
            s);

    destroyQueue(q);
}

/////////////////////////////////////////////////////////////////////////////

int main(void)
{
    srand(0);
    metrics = initQueues(buffer, BUFFER_LIMIT);
    setIllegalOperationCallback(onIllegalOperation);
    setOutOfMemoryCallback(onOutOfMemory);

    perf_test_0();

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_6), // bad destroy bug test
        cmocka_unit_test(test_1),
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
