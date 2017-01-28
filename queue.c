
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

      *node_t where node is stored is called 'place'

 To distinguish node states nxt and  prw can have neganive values:

      1. Only positive part used for offsets;
      2. Negative vals have special meaning:
          nxt < 0 means node has no data.
          prw < 0 means node is root
      3. nxt and prw values are stored with +1 and read with -1
      offsets to keep sign alive


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

 DoubleLinked list semantic:

      Oueue:
      1 2 3 ... N , where 1 is N is last come and 1 is first to out

      List (arrow -> means "next" direction)
      [root 1] -> [N] -> [N-1] -> ... [3] -> [2] ->  [2] -.
         ^                                                |
         '------------------------------------------------'
 */


// ========================================================================== //

// 32 bit bit filed node struct to access data and indexes
typedef struct
{
    unsigned char data  : 8  ;
    signed short  nxt   : 12 ;
    signed short  prw   : 12 ;
} node_t;



// Static buffer for data, and some aliases
#define BUFFER_LIMIT 2048
static unsigned char buffer[BUFFER_LIMIT] = {0};
static node_t* const pstart = (node_t*) buffer;
static node_t* const pend   = (node_t*) (buffer + BUFFER_LIMIT);


// Dynamic pointer
static node_t* pfree = (node_t*) buffer;



// Callbacks
static onOutOfMem_cb_t onOutOfMemory;
static onIllegalOperation_cb_t onIllegalOperation;


// ========================================================================== //

// helper, returns true if pointer is withit [buffer, buffer + MA
static inline int bounds_check(node_t* place);

// checks if node is root
static inline int is_root(node_t* place);

// creates emty root
static inline void make_empty_root(node_t* place);

// empty root has no data
static inline int is_empty_root(node_t* place);

// adds data to emty root
static inline void set_empty_root_data(node_t* place, unsigned char data);

// make non root node - fills it with data
static inline void set_node_data(node_t* place, unsigned char data);

// get roots data, root becomes empty
static inline unsigned char extract_root_data(node_t* root);

// copy data in prew node
static inline unsigned char copy_data_to_root(node_t* root, node_t* src);


// helpers to get/set to write sign indexes
static inline void set_nxt(node_t* node, node_t* place);
static inline void set_prw(node_t* node, node_t* place);
static inline node_t* get_nxt(node_t* node);
static inline node_t* get_prw(node_t* node);

// Allocition/deallocation
static node_t* get_free_node();
static void cleanup_node(node_t* place);

// Updates next and prew node in chain
// to link to place, because its new place for node
// only may be used on non empty root nodes
static void update_chain(node_t* place);

// Only non root nodes can be moved
// Erases to and updates links in froms neibourgs
static void move_node(node_t* place, node_t* new_place);

// Short ciruts neibours to prepare exclude node at place
static void exclude_from_chain(node_t* place);

// puts new node after root, updates links
static void insert_after_root(node_t* root, node_t* newman);

// ========================================================================== //


static inline int bounds_check(node_t* place)
{
    return (place != NULL) && (pstart <= place && place < pend);
}

static inline int is_root(node_t* node)
{
    assert(bounds_check(node));
    return node->prw < 0;
}

static inline void make_empty_root(node_t* node)
{
    assert(bounds_check(node));

    node->prw = -1; // root
    node->nxt = -1; // has no data
    node->data = 0; // zero data
}

static inline int is_empty_root(node_t* node)
{
    assert(bounds_check(node));
    return !is_root(node) ? 0 :  node->nxt < 0;
}

static inline void set_empty_root_data(node_t* root, unsigned char data)
{
    assert(bounds_check(root));
    assert(is_empty_root(root));
    assert(is_root(root));

    short d = root - pstart;

    d += 1; // we use sign, so we avoid zeros

    root->prw = -d; // root
    root->nxt = d ; // has data
    root->data = data; 
}

static inline void set_node_data(node_t* node, unsigned char data)
{
    assert(bounds_check(node));

    node->data = data;
    node->nxt = 1;
    node->prw = 1;
}

static inline unsigned char extract_root_data(node_t* root)
{
    assert(bounds_check(root));
    assert(is_root(root));
    assert(!is_empty_root(root));

    root->nxt = -1; // mark as no data
    return root->data;
}

static inline unsigned char copy_data_to_root(node_t* root, node_t* src)
{
    assert(bounds_check(root));
    assert(bounds_check(src));
    assert(is_root(root));
    assert(!is_root(src));
    assert(src != root);
    assert(!is_empty_root(root));

    unsigned char data = root->data;
    root->data = src->data;

    return data;
}


static inline void set_nxt(node_t* node, node_t* place)
{
    assert(bounds_check(place));

    short d = place - pstart;
    d += 1; // we store +1 values
    node->nxt = node->nxt < 0 ? -d : d;
}

static inline void set_prw(node_t* node, node_t* place)
{
    assert(bounds_check(place));

    short d = place - pstart;
    d += 1;
    node->prw = node->prw < 0 ? -d : d;
}

static inline node_t* get_nxt(node_t* node)
{
    assert(bounds_check(node));
    assert(!is_empty_root(node));

    short idx = abs(node->nxt) - 1; // correct with -1

    return pstart + idx;
}

static inline node_t* get_prw(node_t* node)
{
    assert(bounds_check(node));
    assert(!is_empty_root(node));

    short idx = abs(node->prw) - 1;  // correct with -1

    return pstart + idx;
}


// ========================================================================== //

static node_t* get_free_node()
{
    if (pfree == pend)
        onOutOfMemory();

    node_t* ret = pfree;

    // Advance pointer, untill reaches free place
    while (++pfree != pend && is_root(pfree));

    return ret;
}

static void cleanup_node(node_t* place)
{
    assert(bounds_check(place));
    assert(place != pfree); // we dont free free stuff
    assert(!is_root(place) || (is_root(place) && is_empty_root(place)));

    // scroll down pfree until it points to non root node or reaches us
    while(--pfree != place && is_root(pfree));

    if (pfree == place) // nothing needs to be done
        return;

    // copy node under free and renink
    move_node(pfree, place);

    // clenupd pfree place
    pfree->prw = 1;
    pfree->nxt = 1;
    pfree->data = 0;

}

static void update_chain(node_t* node)
{

    assert(bounds_check(node));
    assert(!is_empty_root(node));

    node_t* next = get_nxt(node);
    node_t* prew = get_prw(node);

    assert(get_nxt(prew) == get_prw(next));

    set_nxt(prew, node);
    set_prw(next, node);
}

static void move_node(node_t* from, node_t* to)
{
    assert(bounds_check(from));
    assert(bounds_check(to));
    assert(!is_root(from));

    *to = *from;
    update_chain(to);
}

static void exclude_from_chain(node_t* node)
{
    assert(bounds_check(node));
    assert(!is_root(node));

    node_t* next = get_nxt(node);
    node_t* prew = get_prw(node);

    assert(get_nxt(prew) == node);
    assert(get_prw(next) == node);

    set_nxt(prew, next);
    set_prw(next, prew);
}

static void insert_after_root(node_t* root, node_t* newman)
{
    assert(bounds_check(root));
    assert(bounds_check(newman));
    assert(is_root(root));
    assert(!is_empty_root(root));
    assert(!is_root(newman));

    node_t* oldman = get_nxt(root); // can be root itself

    set_nxt(root, newman);
    set_prw(oldman, newman);

    set_nxt(newman, oldman);
    set_prw(newman, root);
}


// ========================================================================== //


Q* createQueue()
{
    // create new empty root node and return it as handle
    node_t* root = get_free_node();
    make_empty_root(root);
    return (Q*)root;
}

void destroyQueue(Q* q)
{
    node_t* root = (node_t*)q;
    if (!bounds_check(root))
        onIllegalOperation();

    // Slow =( : need to kill all elements
    while (!is_empty_root(root)) 
        dequeueByte(q);

    // now root is en empty root
    cleanup_node(root);
}

void enqueueByte(Q* q, unsigned char b)
{
    node_t* root = (node_t*)q;
    if (!bounds_check(root))
        onIllegalOperation();

    if (is_empty_root(root))
    {
        set_empty_root_data(root, b);
        return;
    }

    node_t* place = get_free_node();
    set_node_data(place, b);
    insert_after_root(root, place);

}

unsigned char dequeueByte(Q* q)
{
    node_t* root = (node_t*)q;
    if (!bounds_check(root))
        onIllegalOperation();

    if (is_empty_root(root))
        onIllegalOperation(); // must not return

    // TODO: copy_data_to_root can hande this too
    node_t* prew = get_nxt(root);
    if (prew == root) {
        // only root is here -> make it empty
        return extract_root_data(root);
    }


    unsigned char data = copy_data_to_root(root, prew);
    exclude_from_chain(prew);
    cleanup_node(prew);

    return data;
}

void printQueue(Q* q)
{
    node_t* root = (node_t*)q;
    if (!bounds_check(root))
        onIllegalOperation();

    if (is_empty_root(root))
    {
        printf("[empty]\n");
        return;
    }

    printf("[");

    node_t* p;
    while ((p = get_nxt(root)) != root)
    {
        printf("%d ", p->data);
    }

    printf("%d]\n", root->data );
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
