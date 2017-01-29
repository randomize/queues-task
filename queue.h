
#ifndef QUEUE_H
#define QUEUE_H


typedef int Q;

/*
 * Sets buffer to work with and inits library,
 * returs nubmer of elements max capacity
 * zeros out buffer with memset
 * Complexity: O(n) on len (memset)
 */
int initQueues(unsigned char* buffer, unsigned int len);


/*
 *     Creates a FIFO byte queue,
 * returns a handle to it.
 *
 * Complexity: O(1) worst case
 */
Q* createQueue();


/*
 *     Destroy an earlier created byte queue.
 * If queue has data, it gets deallocated.
 * Q* q must be value returned by createQueue,
 * otherwise dehavior is undefined.
 *
 * Complexity: O(n) on number of elements in q
 */
void destroyQueue(Q* q);


/*
 *     Adds a new byte to a queue.
 * Q* q must be value returned by createQueue,
 * otherwise dehavior is undefined.
 *     May call onOutOfMemory if buffer
 * capacity is exeded.
 *
 * Complexity: O(1) worst case
 */
void enqueueByte(Q* q, unsigned char b);


/*
 *     Pops the next byte off the FIFO queue.
 * Q* q must be value returned by createQueue,
 * otherwise dehavior is undefined.
 *     May call onIllegalOperation if called
 * on empty queue.
 *
 * Complexity: O(1) worst case
 */
unsigned char dequeueByte(Q* q);

/*
 *     Debug helper print, outputs queue to
 * stdout in form [1 9 2 5 1]
 * Q* q must be value returned by createQueue,
 * otherwise dehavior is undefined.
 *
 * Complexity: O(n) on number of elements is q
 */
void printQueue(Q* q);

// Callback types
typedef void (*onOutOfMem_cb_t)();
typedef void (*onIllegalOperation_cb_t)();

/*
*     Sets outOfMemory callback.
* When createQueue/enqueByte is unable to satisfy
* a request due to lack of memory this callback
* will be called, which should not return
*/
void setOutOfMemoryCallback(onOutOfMem_cb_t cb);

/*
*     Sets onIllegalOperation callback.
* When illegal request, like attempting to
* dequeue a byte from an empty queue this
* callback will be called, which should not return
*/
void setIllegalOperationCallback(onIllegalOperation_cb_t cb);



#endif // QUEUE_H
