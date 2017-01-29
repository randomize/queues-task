
#ifndef QUEUE_H
#define QUEUE_H


typedef int Q;

// Sets buffer to work with and inits library, returs nubmer of elements max
int initQueues(unsigned char* buffer, unsigned int len);


// Creates a FIFO byte queue, returning a handle to it.
Q* createQueue();


// Destroy an earlier created byte queue.
void destroyQueue(Q* q);


// Adds a new byte to a queue.
void enqueueByte(Q* q, unsigned char b);


// Pops the next byte off the FIFO queue.
unsigned char dequeueByte(Q* q);

// Debug only print
void printQueue(Q* q);

// Callbacks
typedef void (*onOutOfMem_cb_t)();
typedef void (*onIllegalOperation_cb_t)();

// Sets outOfMemory callback.
// When createQueue/enqueByte is unable to satisfy a request due to lack of memory
// this callback will be called, which should not return
void setOutOfMemoryCallback(onOutOfMem_cb_t cb);

// Sets onIllegalOperation callback.
// When illegal request, like attempting to dequeue a byte from an empty queue
// this callback will be called, which should not return
void setIllegalOperationCallback(onIllegalOperation_cb_t cb);



#endif // QUEUE_H
