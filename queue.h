/*

 Byte queue library, implemented on single buffer

 Copyright © 2017 Eugene Mihailenco <mihailencoe@gmail.com>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef QUEUE_H
#define QUEUE_H


typedef long Q; // TODO: how to forward declare node_t here? may shoot foot as is

typedef struct
{
    const char* name;                    // just name for your impl
    int max_empty_queues;                // if just calling createQueue - how many will it handdle before out of mem?
    int max_nonempty_queues;             // if calling createQueue and putting there at least 1 byte in each - how manu queues will be there before out of mem?
    int max_els_in_single;               // if created one queue and put there as many bytes as passible before out of mem, what is number of bytes
    int max_els_in_16even;               // with 16 queues, how many bytes will each have being evenly distributed
    int max_els_in_64even;               // same with 64 queues
    int max_els_in_max_even_queues;      // same with max_nonempty_queues value specified before
    int max_els_in_single_with_63_empty; // if I create 63 empty queues and one work queue and put all data to 64th work queue - how many bytes will that be?
} queueMetrics_t;


/*
 * Sets buffer to work with and inits library,
 * returs nubmer of elements max capacity
 * zeros out buffer with memset
 * Complexity: O(n) on len (memset)
 */
queueMetrics_t initQueues(unsigned char* buffer, unsigned int len);


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

///////////////// non-mandotory api ///////////////////////////////////////////////

/*
 *     Debug helper print, outputs queue to
 * stdout in form [1 9 2 5 1]
 * Q* q must be value returned by createQueue,
 * otherwise dehavior is undefined.
 *
 * Complexity: O(n) on number of elements is q
 */
void printQueue(Q* q);


#endif // QUEUE_H
