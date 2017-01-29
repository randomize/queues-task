
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
    unsigned char   data  : 8  ;
    unsigned short  nxt   : 12 ;
    unsigned short  prw   : 12 ;
} __attribute__((packed)) node_t;

static_assert(sizeof(int) == 4,    "Algorithm relies on 4 byte ints");
static_assert(sizeof(node_t) == 4, "Algorithm relies on 4 byte nodes");
static_assert(sizeof(char) == 1,   "In case C standard broken by compiler");
/* static_assert(CHAR_BIT == 8,   "In case platform is weird"); */


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

// empty root has no data
static inline int is_empty_root(node_t* node);

// single root points to itself and is not empty
static inline int is_single_root(node_t* node);

// full node has both data A and data B
static inline int is_full_node(node_t* node);

// get roots data, root becomes empty
static inline unsigned char extract_root_data(node_t* root);

// copy data in prew node
static inline unsigned char copy_data_to_root(node_t* root, node_t* src);

// sets data B for node, make node full
static inline void add_data_B(node_t* node, unsigned char b);

// gets data B, makes node normal
static inline unsigned char remove_data_B(node_t* node);

// swaps A and B in full node
static inline void swap_data_A_B(node_t* node);

// helpers to get/set to write sign indexes
static inline void set_nxt(node_t* node, node_t* target);
static inline void set_prw(node_t* node, node_t* target);
static inline node_t* get_nxt(node_t* node);
static inline node_t* get_prw(node_t* node);

// Allocates a node, returns it all zeroed
static node_t* alloc_node();

// Deallocates node, should not be used after free
static void free_node(node_t* node);


// ========================================================================== //


static inline int bounds_check(node_t* node)
{
    // buffer_len may not be aligned by node_t size
    int max_nodes = buffer_len / sizeof(node_t) ;
    node_t* pstart = (node_t*) buffer;
    node_t* pend   = pstart + max_nodes;

    return (node != NULL) && (pstart < node && node < pend);
}

static inline int get_node_index(node_t* node)
{
    assert(bounds_check(node));
    int d =  node - (node_t*)buffer;
    assert(d > 0);
    return d;
}

static inline int is_empty_root(node_t* node)
{
    assert(bounds_check(node));
    return node->nxt == 0 ;
}

static inline int is_single_root(node_t* node)
{
    assert(bounds_check(node));
    return !is_empty_root(node) && node->nxt == get_node_index(node);
}

static inline void make_single_root(node_t* root, unsigned char data)
{
    assert(bounds_check(root));
    assert(is_empty_root(root));

    short d = get_node_index(root);

    root->prw = d;
    root->nxt = d;
    root->data = data;
}

static inline int is_full_node(node_t* node)
{
    assert(bounds_check(node));
    assert(!is_empty_root(node));
    return node->prw != 0;
}


static inline unsigned char extract_root_data(node_t* root)
{
    assert(bounds_check(root));
    assert(!is_empty_root(root));

    unsigned char b = root->data;
    int* p = (int*)root; *p = 0; // perf: only need to zero ->nxt, but its faster to wipe whole

    return b;
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

static inline void add_data_B(node_t* node, unsigned char b)
{
    assert(bounds_check(node));
    assert(!is_empty_root(node));
    assert(!is_full_node(node));

    unsigned short data = b;
    data |= 0xFF00;
    node->prw = data;
}

static inline unsigned char remove_data_B(node_t* node)
{
    assert(bounds_check(node));
    assert(!is_empty_root(node));
    assert(is_full_node(node));

    unsigned short b = node->prw;
    node->prw = 0;
    return b;
}

static inline void swap_data_A_B(node_t* node)
{
    assert(bounds_check(node));
    assert(!is_empty_root(node));
    assert(is_full_node(node));

    unsigned short data = node->data;
    data |= 0xFF00;
    node->data = node->prw;
    node->prw = data;
}

static inline void set_nxt(node_t* node, node_t* target)
{
    assert(bounds_check(node));
    assert(bounds_check(target));

    node->nxt = get_node_index(target);
}

static inline void set_prw(node_t* node, node_t* target)
{
    assert(bounds_check(node));
    assert(bounds_check(target));

    node->prw = get_node_index(target);
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
    return (Q*) alloc_node();
}

void destroyQueue(Q* q)
{
    node_t* root = (node_t*)q;
    assert(bounds_check(root));

    if (is_empty_root(root) || is_single_root(root))
    {
        free_node(root);
        return;
    }

    node_t* p = get_nxt(root);
    while (p != root)
    {
        node_t* pp = get_nxt(p);
        free_node(p);
        p = pp;
    }

    // now root is en empty root
    free_node(root);
}


void enqueueByte(Q* q, unsigned char b)
{
    node_t* root = (node_t*)q;
    assert(bounds_check(root));

    if (is_empty_root(root))
    {
        make_single_root(root, b);
        return;
    }

    node_t* prew = get_prw(root); // can be root itself!

    if (!is_single_root(root))
    {
        if (!is_full_node(prew))
        {
            add_data_B(prew, b);
            swap_data_A_B(prew);
            return;
        }

    }

    node_t* newman = alloc_node();
    if (newman == NULL)
        return;

    newman->data = b;

    set_nxt(newman, root);
    set_nxt(prew, newman);
    set_prw(root, newman);

}

unsigned char dequeueByte(Q* q)
{
    node_t* root = (node_t*)q;

    if (is_empty_root(root))
    {
        onIllegalOperation();
        return 0;
    }

    if (is_single_root(root))
    {
        return extract_root_data(root);
    }

    unsigned char old = root->data;
    node_t* next = get_nxt(root);

    if (is_full_node(next))
    {
        root->data = remove_data_B(next);
        return old;
    }

    root->data = next->data;
    set_nxt(root, get_nxt(next));
    if (get_prw(root) == next) // it was last node
        set_prw(root, root);   // so make me single again

    free_node(next);

    return old;
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
