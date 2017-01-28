
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>

/*
 
Preconditions:

    assume sizeof(int) = 4
    CHAR_BIT == 8


    Implemented as 2-linked list, each node takes 4 bytes, because of best alignment


Performance:

    enqueueByte/dequeueByte/createQueue - garanteed to be constant time
    destroyQueue is linear on elements in that queue
    printQueue is linear on emlemenit in that queue


Memory:

    Only one buffer used:

        [pfree|node|node|.....|node]

    Stucture of bit fields;

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

 Allocation/Deallocation:

            !!!!!!!!!!!!!!!!!!!REWRITE!!!!!!!!!!!!!!!!!!!!!!!!!
      Still, a twist exists - root node address are used as handles because of
      memory constrain. These should never be moved/swaped.
          When a node is allocated - prfee is advanced until it points to
      free node place. When a node is deallocated swap idiom is used:
          prfree decrements and if it points to normal node its swapped
          if it points to root node, prfree decrements until finds normal node or reaches node deallocated
            !!!!!!!!!!!!!!!!!!!REWRITE!!!!!!!!!!!!!!!!!!!!!!!!!

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
} __attribute__((packed)) node_t;



// Static buffer for data, non const to make dyramic setup with 
// initQueues() call - can be satic and cons! no cheating!
static unsigned char* buffer;
static int buffer_len;


// Callbacks
static onOutOfMem_cb_t onOutOfMemory;
static onIllegalOperation_cb_t onIllegalOperation;


// ========================================================================== //

// helper, returns true if pointer is withit [buffer, buffer + MA
static inline int bounds_check(node_t* node);

// creates emty root
static inline void make_empty_root(node_t* node);

// make non root node - fills it with data
static inline void make_node(node_t* node, unsigned char data);

// empty root has no data
static inline int is_empty_root(node_t* node);

// adds data to emty root
static inline void set_empty_root_data(node_t* node, unsigned char data);


// get roots data, root becomes empty
static inline unsigned char extract_root_data(node_t* root);

// copy data in prew node
static inline unsigned char copy_data_to_root(node_t* root, node_t* src);


// helpers to get/set to write sign indexes
static inline void set_nxt(node_t* node, node_t* target);
static inline void set_prw(node_t* node, node_t* target);
static inline node_t* get_nxt(node_t* node);
static inline node_t* get_prw(node_t* node);

// Allocition/deallocation
static node_t* get_free_node();
static void cleanup_node(node_t* node);

// Short ciruts neibours to prepare node removing
static void exclude_from_chain(node_t* node);

// puts new node after root, updates links
static void insert_after_root(node_t* root, node_t* newman);

// ========================================================================== //


static inline int bounds_check(node_t* node)
{
    // buffer_len may not be aligned by node_t size
    int max_nodes = buffer_len / sizeof(node_t) ;
    node_t* pstart = (node_t*) buffer;
    node_t* pend   = pstart + max_nodes;

    return (node != NULL) && (pstart < node && node < pend);
}

static inline void make_empty_root(node_t* node)
{
    assert(bounds_check(node));

    node->nxt = node->prw = 0;
}

static inline void make_node(node_t* node, unsigned char data)
{
    assert(bounds_check(node));

    node->data = data;
}

static inline int is_empty_root(node_t* node)
{
    assert(bounds_check(node));
    return node->nxt == 0 && node->prw == 0;
}

static inline void set_empty_root_data(node_t* root, unsigned char data)
{
    assert(bounds_check(root));
    assert(is_empty_root(root));

    short d = root - (node_t*)buffer;

    root->prw = d;
    root->nxt = d;
    root->data = data;
}


static inline unsigned char extract_root_data(node_t* root)
{
    assert(bounds_check(root));
    assert(!is_empty_root(root));

    root->nxt = root->prw = 0;
    return root->data;
}

static inline unsigned char copy_data_to_root(node_t* root, node_t* src)
{
    assert(bounds_check(root));
    assert(bounds_check(src));
    assert(src != root);
    assert(!is_empty_root(root));

    unsigned char data = root->data;
    root->data = src->data;

    return data;
}


static inline void set_nxt(node_t* node, node_t* target)
{
    assert(bounds_check(node));
    assert(bounds_check(target));

    short d = target - (node_t*)buffer;
    assert(d > 0);

    node->nxt = d;
}

static inline void set_prw(node_t* node, node_t* target)
{
    assert(bounds_check(node));
    assert(bounds_check(target));

    short d = target - (node_t*)buffer;
    assert(d > 0);

    node->prw = d;
}

static inline node_t* get_nxt(node_t* node)
{
    assert(bounds_check(node));
    assert(!is_empty_root(node));
    assert(node->nxt > 0);

    return (node_t*)buffer + node->nxt;
}

static inline node_t* get_prw(node_t* node)
{
    assert(bounds_check(node));
    assert(!is_empty_root(node));
    assert(node->prw > 0);

    return (node_t*)buffer + node->prw;
}


// ========================================================================== //

static node_t* get_free_node()
{

    int* pfree = (int*)buffer; // first el is index of next free

    assert(*pfree != 0);

    int max_nodes = buffer_len / sizeof(node_t) - 1;
    if (*pfree > max_nodes)
    { 
        onOutOfMemory();
        return NULL;
    }

    int* ret = pfree + *pfree;
    if (*ret == 0)
    {
        *pfree += 1;
    } else {
        *pfree = *ret;
        *ret = 0;
    }

    return (node_t*)ret;
}

static void cleanup_node(node_t* node)
{
    assert(bounds_check(node));

    int* pfree = (int*)buffer; // first el is index of next free

    int* ret = (int*) node;
    assert(ret != pfree); // we dont free free stuff

    *ret = *pfree;
    *pfree = ret - pfree;
}

static void exclude_from_chain(node_t* node)
{
    assert(bounds_check(node));

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
    assert(!is_empty_root(root));

    node_t* oldman = get_nxt(root); // can be root itself

    set_nxt(root, newman);
    set_prw(oldman, newman);

    set_nxt(newman, oldman);
    set_prw(newman, root);
}


// ========================================================================== //


int initQueues(unsigned char* buf, int len)
{
    assert(sizeof(node_t) == 4); // make sure pad/packing work
    assert(buf != NULL);
    assert(len >= 4); // at least one node ;)

    buffer = buf;
    buffer_len = len;

    memset(buf, 0, len);
    int* pfree = (int*) buf;
    *pfree = 1;

    return len / sizeof(node_t) - 1; // one is used for pfree index
}


Q* createQueue()
{
    // create new empty root node and return it as handle
    node_t* root = get_free_node();

    if (root == NULL)
        return NULL;

    make_empty_root(root);
    return (Q*)root;
}

void destroyQueue(Q* q)
{
    node_t* root = (node_t*)q;

    if (!bounds_check(root))
    {
        onIllegalOperation();
        return;
    }

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
    {
        onIllegalOperation();
        return;
    }

    if (is_empty_root(root))
    {
        set_empty_root_data(root, b);
        return;
    }

    node_t* newman = get_free_node();
    if (newman == NULL)
        return;

    make_node(newman, b);
    insert_after_root(root, newman);

}

unsigned char dequeueByte(Q* q)
{
    node_t* root = (node_t*)q;

    if (!bounds_check(root) || is_empty_root(root))
    {
        onIllegalOperation();
        return 0;
    }

    node_t* prew = get_prw(root);
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
    {
        onIllegalOperation();
        return;
    }

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
