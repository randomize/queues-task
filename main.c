

#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

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

int main(void)
{

    setIllegalOperationCallback(onIllegalOperation);
    setOutOfMemoryCallback(onOutOfMemory);

    Q* q0 = createQueue();
    enqueueByte(q0, 0);
    enqueueByte(q0, 1);

    Q* q1 = createQueue();
    enqueueByte(q1, 3);
    enqueueByte(q0, 2);
    enqueueByte(q1, 4);

    printf("%d ", dequeueByte(q0));
    printf("%d\n", dequeueByte(q0));

    enqueueByte(q0, 5);
    enqueueByte(q1, 6);

    printf("%d ", dequeueByte(q0));
    printf("%d\n", dequeueByte(q0));

    destroyQueue(q0);

    printf("%d ", dequeueByte(q1));
    printf("%d ", dequeueByte(q1));
    printf("%d\n", dequeueByte(q1));

    destroyQueue(q1);
    
    return 0;
}

