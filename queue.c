
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>

/*

## Preconditions

    assume sizeof(int) == sizeof(node_t) == 4
    assume that CHAR_BIT == 8
    best alignment on target platform is 4


    Queues are implemented as single-linked lists, each node
    has payload of 2 byte and 9 bits for next index.
    Root nodes have 1 byte of data and two indexes.
    Node takes 4 bytes, because of alignment so indexes have
    12 bits, in case buffer_len is bigger then 2048 (max 16384)

    Only one int value at begigging of buffer is used by allocttor.
    Maximum capacity varies between 958 for 64 queues (limit given by task)
    and 1021 for single queue - because root queue node can only
    take 1 byte of payload.


## Performance

    enqueueByte/dequeueByte/createQueue - garanteed to be constant time
    destroyQueue is linear on elements in that queue
    printQueue is linear on emlemenit in that queue


## Memory

    Only memory in buffer used, layout:

        [pfree|node|node|.....|node]

    Stucture of bit fields;

         XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX
         [ data ] [    prw    ][    nxt    ]

     data = 8 bit  payload
     prw  = 12 bit index of prew node (tail)
     nxt  = 12 bit index of next node (head)

         XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX
         [ data ] [ datb ] 1111[    nxt    ]

     data = 8 bit  payload data A
     datb = 8 bit  payload data B
     nxt  = 10 bit index of next node
     1111 = 4 bit  pad, used to check if data B avail

## Some conventions and naming

    Node types:

        root node   - used as handle returned to client,
                      and also has payload, uses both
                      nxt and prw indexes. data field
                      represets "to be dequeued" element.

        normal node - for data only, uses nxt and.


    Root node states:

        empty   - state when queue was just created nxt == NULL
        single  - has payload, nxt == prw == this node
        root    - has payload and more nodes in chain


    Normal node states:

        normal - only data A is avail
        full   - both A and B payloads used

## Allocation/Deallocation:

    Generic node allocator with free node list implemented in free space
used. pfree index always points to free region that will be returned by
allocate_node() call. deallocate() stores previous pfree value in free
memory region to recover pfree after next allocatiens. Both funcitons
work in constant time. Allocater returns zeroed out node.


 Dequeue:
      use handle as pointer to node, read root node.
      If its empty - raise error, return null;
      if its single - set empty, return old data;
      take next node;
      if its full - return B, make it normal, copy B to root, return old root data;
      if its normal - copy A to root. deallocate it, return old root data;

 Enquoue:
      use handle as pointer to node, read root node.
      if its empty - set data, wire nxt and prw, return;
      if its single - goto make new node
      take prew node
      if prew node is normal - add data B, swap B<->A, return;
      make new node, set its data A and nxt to root
      link new node: prew->nxt = new, root->prw = new


list as FIFO semantic:

      Oueue:
      1 2 3 ... N , where 1 is last come and N is first to out

      [root N] --nxt-> [N-1] -> [N-2] -> ... [3] -> [2] -> [1] -.
        |  ^                                                ^   |
        |  '----------------------nxt-----------------------|---'
        '-----------------prw-------------------------------'

*/

// ========================================================================== //

// 32 bit bit filed node struct to access data and indexes
typedef struct
{
    unsigned char data  : 8  ;
    signed short  nxt   : 12 ;
    signed short  prw   : 12 ;
} __attribute__((packed)) node_t;

static_assert(sizeof(int) == 4,    "Algorithm relies on 4 byte ints");
static_assert(sizeof(node_t) == 4, "Algorithm relies on 4 byte nodes");


// Static buffer for data - can be set from outside
// with initQueues() call, and its len. No other data used.
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

// Allocates a node, returns it all zeroed
static node_t* alloc_node();

// Deallocates node, should not be used after free
static void free_node(node_t* node);

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

static node_t* alloc_node()
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

static void free_node(node_t* node)
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


int initQueues(unsigned char* buf, unsigned int len)
{
    assert(buf != NULL);
    assert(len >= 2 * sizeof(node_t)); // at least one node ;)

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
    node_t* root = alloc_node();

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
    free_node(root);
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

    node_t* newman = alloc_node();
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
    free_node(prew);

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
