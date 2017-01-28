
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/*
 assume int = 4 x char and CHAR_BIT == 8
 Need ajustmens for other platforms
 ints are aligned by 4
 TODO: how to force compile time check?

 Implemented as 2-linked list, each node is an int, because of best alignment
 enqueueByte/dequeueByte garanteed to be linear in time on average amortized,
 worst case is linear to nubmer of queues.


 Stucture of bit fields
 XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX
 [ data ] [    nxt    ][    prw    ]

 data    = 8 bit of unsigned char payload
 nxt = index of next node
 prw = index of prew node

 Some conventions:

      node types:
          root node - used as handle and also has data
          normal node - for data only

      root node states:
          empty - state when queue was just created
          data - node has payload, represents "current element"

      *int where node is stored is called 'place'

 To distinguish node states nxt and  prw can have neganive values:

      1. Only positive part used for offsets;
      2. Negative vals have special meaning:
          nxt < 0 means node has no data.
          prw < 0 means node is root


 Allocation/Deallocation:

          Standard free pointer increment for allocation and swap+decrement
      for deallocation idiom used. Pointer prfee set to highest free place.
      Still, a twist exists - root node address are used as handles because of
      memory constrain. These should never be moved/swaped.
          When a node is allocated - prfee is advanced until it points to
      free node place. When a node is deallocated swap iniom is used:
          prfree decrements and if it points to normal node its swapped
          if it points to root node, prfree decrements until finds normal node or reaches node deallocated

 Dequeue:
      use handre as pointer to node, read root node.
      If its empty - raise error
      if its single el - pop it, set empty
      if it has prew = copy data, copy prew to root, deallocate prew
 Enquoue:
      use handre as pointer to node, read root node.
      if its empty - write val, set sign
 */


// ========================================================================== //


// Static buffer for data, and some aliases
#define BUFFER_LIMIT 2048
static const unsigned char buffer[BUFFER_LIMIT] = {0};
static const int* pstart = (int*) buffer;
static const int* pend   = (int*) (buffer + BUFFER_LIMIT);


// Dynamic pointer
static int* pfree = (int*) buffer;


// 32 bit bit filed node struct to access data and indexes
typedef struct
{
    unsigned char data  : 8  ;
    signed short  nxt   : 12 ;
    signed short  prw   : 12 ;
} node_t;


// Callbacks
static onOutOfMem_cb_t onOutOfMemory;
static onIllegalOperation_cb_t onIllegalOperation;


// ========================================================================== //


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

// creates emty root
static inline void make_empty_root(int* place)
{
    assert(bounds_check(place));

    *place = 0;
    node_t* this_n = (node_t*) place;
    this_n->prw = -1; // root
    this_n->nxt = -1; // has no data
}

// empty root has no data
static inline int is_empty_root(int* place)
{
    assert(bounds_check(place));
    assert(is_root(place));

    node_t* this_n = (node_t*) place;
    return this_n->nxt < 0;
}

// adds data to emty root
static inline void fill_empty_root(int* place, unsigned char data)
{
    assert(bounds_check(place));
    assert(is_empty_root(place));
    assert(is_root(place));

    node_t* this_n = (node_t*) place;
    short d = place - pstart;

    this_n->prw = -d; // root
    this_n->nxt = d ; // has data
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
static void updateChain(int* place)
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
static void moveNode(int* place, int* new_place)
{
    assert(bounds_check(place));
    assert(bounds_check(new_place));
    assert(!is_root(place));

    *new_place = *place;
    updateChain(new_place);
}

// Short ciruts neibours to prepare exclude node at place
static void excludeFromChain(int* place)
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

// gets new node to work with
static int* get_free_node()
{
    if (pfree == pend)
        onOutOfMemory();

    int* ret = pfree;

    // Advance pointer, untill reaches free place
    while (++pfree != pend && is_root(pfree));

    return ret;
}

static void cleanup_node(int* place)
{
    place* = 0;
    // TODO: need usecase for this firs
}


// ========================================================================== //


Q* createQueue()
{
    // create new empty root node and return it as handle
    int* root = get_free_node();
    make_empty_root(root);
    return root;
}

void destroyQueue(Q* q)
{
    int* root = q;
    if (!bounds_check(root))
        onIllegalOperation();

    // TODO: DESTROY contents??????
}

void enqueueByte(Q* q, unsigned char b)
{
    int* root = q;
    if (!bounds_check(root))
        onIllegalOperation();

    if (is_empty_root(root))
    {
        fill_empty_root(root, b);
        return;
    }

    int* place = get_free_node();

    // TODO: put

}

unsigned char dequeueByte(Q* q)
{
    int* root = q;
    if (!bounds_check(root))
        onIllegalOperation();

    if (is_empty_root(root))
        onIllegalOperation(); // must not return

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
