
#include "queue.h"
#include <stdio.h>
#include <assert.h>

// assume int = 4 x char and CHAR_BIT == 8
// Need ajustmens for other platforms
// ints are aligned by 4



#define BUFFER_LIMIT 2048

static onOutOfMem_cb_t onOutOfMemory;
static onIllegalOperation_cb_t onIllegalOperation;


// Static data
static const unsigned char buffer[BUFFER_LIMIT] = {0};
static const unsigned char* buffer_end = buffer + BUFFER_LIMIT;

// Dynamic pointers
static int* pst;
static int* pend;

// Implemented as 2-linked list, each node is an int, because of best alignment
// enqueueByte/dequeueByte garanteed to be linear in time
//
//
// Stucture of bit fields
// XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX 
// [ data ] [ inxnxt ]   [ inxprw ]  #
// 
// data   = 8 bit of unsigned char payload
// inxnxt = index of next node 9 bits are enough, but can be 12 in BUFFER_LIMIT is bigger
// inxprw = index of prew node 9 bits
// #      = root node flag
//
//
// We allocate first node in any queue in beginning of and work nodes
// and use them as handles for Queues (Q is typedef to *int)
//
//
// Some conventions: 
//
//      indexes: inxnxt and inxprw are pointer offsets 'buffer' addr, but
//      stored as index-1 values, so 0 value has spetial meanin
//      used as marker
//      when inxnxt == inxprw != NULL - Queue has 1 item
//      when inxnxt == NULL
//
//
//

void initQeuesLib()
{
    pst  = (int*) buffer     + 1;
    pend = (int*) buffer_end - 1;
}

// 32 bit bit filed struct to access data and indexes
typedef struct { 
    unsigned char data     : 8  ;
    short         inxnxt   : 12 ;
    short         inxprw   : 12 ;

} node_t;


// empty root has no data
inline int is_empty_root(int* place)
{
    assert(place != NULL);
    assert(buffer <= place && place < buffer_end);

    node_t* from_n = (node_t*) place;
    return from_n->inxprw == 0 && from_n->inxnxt == 0;
}

// must never be called on empty root
inline short get_prew_index(int* place)
{
    assert(place != NULL);
    assert(buffer <= place && place < buffer_end);

    node_t* from_n = (node_t*) place;
    return from_n->inxprw - 1;
}

    
// Updates next and prew node in chain 
// to link to place, because its new place for node
void updateChain(int* place)
{

    assert(place != NULL);
    assert(buffer <= place && place < buffer_end);

    node_t* from_n = (node_t*) place;

    short inxnxt = from_n->inxnxt;
    short inxprw = from_n->inxprw;

    node_t* prew_n = (node_t*) (buffer + inxnxt);
    node_t* next_n = (node_t*) (buffer + inxprw);

    assert(prew_n->inxnxt == next_n->inxprw);
    assert(buffer + prew_n->inxnxt == place);

    prew_n->inxnxt = inxprw;
    next_n->inxprw = inxprw;
}

// Only non root nodes can be swapped
// Erases to and updates links in froms neibourgs
void moveNode(int* old_place, int* new_place)
{
    *new_place = *old_place;



}

// Short ciruts neibours to exclude node at place
void excludeFromChain(int* place)
{
    assert(place != NULL);
    assert(buffer <= place && place < buffer_end);

    node_t* from_n = (node_t*) place;

    short inxnxt = from_n->inxnxt;
    short inxprw = from_n->inxprw;

    node_t* prew_n = (node_t*) (buffer + inxnxt);
    node_t* next_n = (node_t*) (buffer + inxprw);

    assert(prew_n->inxnxt == next_n->inxprw);
    assert(buffer + prew_n->inxnxt == place);

    prew_n->inxnxt = inxprw;
    next_n->inxprw = inxprw;
}
    

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


// Pops the next byte off the FIFO queue.
unsigned char dequeueByte(Q* q)
{
    if (is_empty_root(q))
    {
        onIllegalOperation(); // must not return
        return 0;
    }

    // TODO:

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
