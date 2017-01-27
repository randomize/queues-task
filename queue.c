
#include "queue.h"
#include <stdio.h>
#include <assert.h>


#define BUFFER_LIMIT 2048

static onOutOfMem_cb_t onOutOfMemory;
static onIllegalOperation_cb_t onIllegalOperation;


//Creates a FIFO byte queue, returning a handle to it.
Q* createQueue()
{
    return NULL;
}


//Destroy an earlier created byte queue.
void destroyQueue(Q* q)
{

}


//Adds a new byte to a queue.
void enqueueByte(Q* q, unsigned char b)
{

}


//Pops the next byte off the FIFO queue.
unsigned char dequeueByte(Q* q)
{
    return 0;
}

void setOutOfMemoryCallback(onOutOfMem_cb_t cb)
{
    assert(cb != NULL);
    onOutOfMemory = cb;
}


void setIllegalOperationCallback(onIllegalOperation_cb_t cb)
{
    assert(cb != NULL);
    onIllegalOperation = cb;
}
