
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// assume int = 4 x char and CHAR_BIT == 8
// Need ajustmens for other platforms
// ints are aligned by 4
// TODO: how to force compile time check?


// Callbacks
static onOutOfMem_cb_t onOutOfMemory;
static onIllegalOperation_cb_t onIllegalOperation;


// Static buffer for data, and some aliases
#define BUFFER_LIMIT 2048
static const unsigned char buffer[BUFFER_LIMIT] = {0};
static const int* pstart = (int*) buffer;
static const int* pend   = (int*) (buffer + BUFFER_LIMIT);

// Dynamic pointer
static int* pfree = (int*) buffer;

// Implemented as 2-linked list, each node is an int, because of best alignment
// enqueueByte/dequeueByte garanteed to be linear in time on average amortized,
// worst case is linear to nubmer of queues.
//
//
// Stucture of bit fields
// XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX 
// [ data ] [    nxt    ][    prw    ]  
// 
// data    = 8 bit of unsigned char payload
// nxt = index of next node
// prw = index of prew node
//
// Some conventions: 
//
//      node types:
//          root node - used as handle and also has data
//          normal node - for data only
//
//      root node states:
//          empty - state when queue was just created
//          data - node has payload, represents "current element"
//
// To distinguish node states nxt, prw can have neganive values:
//
//      1. Only positive part used for offsets;
//      2. Negative have spetial meaning: 
//          nxt < 0 means node has no data.
//          prw < 0 means node is root
//
//
// Allocation:
//      prfee is pointing to highest free node place
//      when a node is allocated - prfee is advanced until it points to free node place
//      wnen node is deallocated swap iniom is used:
//          prfree decrements and if it points to normal node its swapped
//          if it points to root node, prfree decrements until finds normal node or reaches node deallocated
//
// Dequeue:
//      use handre as pointer to node, read root node.
//      If its empty - raise error
//      if its single el - pop it, set empty
//      if it has prew = copy data, copy prew to root, deallocate prew
// Enquoue:
//      use handre as pointer to node, read root node.
//      if its empty - write val, set sign
//      
//

// 32 bit bit filed node struct to access data and indexes
typedef struct { 
    unsigned char data  : 8  ;
    signed short  nxt   : 12 ;
    signed short  prw   : 12 ;
} node_t;

static inline int bounds_check(int* place)
{
    return (place != NULL) && (pstart <= place && place < pend);
}

// empty root has no data
static inline int is_root(int* place)
{
    assert(bounds_check(place));

    node_t* this_n = (node_t*) place;
    return this_n->prw < 0;
}

// empty root has no data
static inline int is_empty_root(int* place)
{
    assert(bounds_check(place));

    node_t* this_n = (node_t*) place;
    return this_n->nxt < 0;
}

// helpers to get/set to write sign indexes

static inline void set_nxt(node_t* node, int* place)
{
    assert(bounds_check(place));

    short d = place - pstart;
    node->nxt = node->nxt < 0 ? -d : d;
}

static inline void set_prw(node_t* node, int* place)
{
    assert(bounds_check(place));

    short d = place - pstart;
    node->prw = node->prw < 0 ? -d : d;
}

static inline int* get_nxt(node_t* node)
{
    assert(bounds_check((int*)node));
    assert(!is_empty_root(node));

    return (int*) (pstart + abs(node->nxt));
}

static inline int* get_prw(node_t* node)
{
    assert(bounds_check((int*)node));
    assert(!is_empty_root(node));

    return (int*) (pstart + abs(node->prw));
}
    
// Updates next and prew node in chain 
// to link to place, because its new place for node
// only may be used on non empty root nodes
void updateChain(int* place)
{

    assert(place != NULL);
    assert(buffer <= place && place < buffer_end);
    assert(!is_empty_root(place));

    node_t* this_n = (node_t*) place;

    node_t* next_n = (node_t*) get_nxt(this_n);
    node_t* prew_n = (node_t*) get_nxt(this_n);

    assert(abs(prew_n->nxt) == abs(next_n->prw));

    set_nxt(prew_n, place);
    set_prw(next_n, place);
}

// Only non root nodes can be moved
// Erases to and updates links in froms neibourgs
void moveNode(int* place, int* new_place)
{
    assert(bounds_check(place));
    assert(bounds_check(new_place));
    assert(!is_root(place));

    *new_place = *place;
    updateChain(new_place);
}

// Short ciruts neibours to prepare exclude node at place
void excludeFromChain(int* place)
{
    assert(bounds_check(place));
    assert(!is_root(place));

    node_t* this_n = (node_t*) place;

    int* next = get_nxt(this_n);
    int* prew = get_prw(this_n);

    node_t* next_n = (node_t*) next;
    node_t* prew_n = (node_t*) prew;

    assert(abs(prew_n->inxnxt) == abs(next_n->inxprw));
    assert(get_prw(prew_n) == place);

    set_nxt(prew_n, next);
    set_prw(next_n, prew);
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
