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


#include "queue2.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <limits.h>
#include <stddef.h>

/*

## Preconditions

    assume sizeof(int) == sizeof(node_t) == 4
    assume that CHAR_BIT == 8
    assume that best alignment on target platform is 4


## Base ideas

    Queues are implemented as single-linked lists, each node is 8 bytes;
    Buffer of size 2048 allows to allocate 256 nodes

    Node with index 0 is used for allocator, freelist index is stored
    in free mem, so effectively we have 255 queues and index 0 may
    be used as null pointer, while having 8 bit size;

    Normal node has 7 bytes payload and 1 next node index
    Tail node doest need next intex, so it has 8 bytes payload
    Root nodes have 5 bytes payload, head/tail indexes and counters
    used to check how many payload slots used in head, in tail and
    in root itself;

    Capacity depends on case, couple examples:

      (worst) created 63 empty queues and one full - 1343 = (255 - 64 - 1)*7 + 8 + 5
              created 64 all equally full          - 1721 = (255 - 64 - 64)*7 + 64*8 + 64*5
              created 1 full queue                 - 1784


## Performance

    enqueueByte/dequeueByte/createQueue - worst case linear
    destroyQueue is linear on element count in that queue
    printQueue is linear on element count in that queue


## Memory

    Only memory in buffer used, layout:

        [pfree|node|node|...|node]

    Structure of node bit fields:

    Root node:

         XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX
         [ data ] [ data ] [ data ] [ data ] [ data ] [ head ] [ tail ] [ cntr ]

     data = 8x5 bit  payload
     head = 8  bit index of head of queue
     tail = 8  bit index of tail of queue
     cntr = 8  bit counters - interpreted differently:
        if head == NULL - 8 bits is number of used payload slots in root
        if head != NULL - 4 bits - head, 4 bits - tail - nr of slots used in them

    Normal node:

         XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX
         [ data ] [ data ] [ data ] [ data ] [ data ] [ data ] [ data ] [ next ]

     data = 8x7 bit  payload data
     next = 8   bit  index of next node

    Tail node:

         XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX
         [ data ] [ data ] [ data ] [ data ] [ data ] [ data ] [ data ] [ data ]

     data = 8x8 bit  payload data


## Some conventions and naming

    Node types:

        root node   - used as handle returned to client,
                      and also has payload, stores
                      indexes for head and tail and countres.
                      1st data field represents "to be dequeued" element.

        normal node - for data only, has 7 payload byte fields
                      and index for next element


    Root node states:

        empty   - state when queue was just created head == tail == NULL, cntr == 0
        single  - has some payload, head == head == NULL, cntr != 0
        full    - has full payload, head != NULL uses head and tail counters

    Normal node states:

        normal - not all payload slots filled yet
        full   - all slots used

## Allocation/Deallocation:

    Generic node allocator with free nodes list implemented in free storage
used. pfree index always points to free region that will be returned by
allocate_node() call. deallocate() stores previous pfree value in free
memory region to recover pfree after subsequent allocations/deallocations.
Both functions work in constant time. Allocator returns zeroed out node.


 Enqueue:
      use Q handle as pointer to node, read root node.
      if its not full - set data, wire nxt and prw to self, return;
      if its single - goto make new node
      take prew node
      if prew node is normal - add data B, swap B<->A, return;
      make new node, set its data A and nxt to root
      link new node: prew->nxt = new, root->prw = new

 Dequeue:
      use Q handle as pointer to node, read root node.
      If its empty - raise error, return null;
      if its single - set empty, return old data;
      take next node;
      if its full - return B, make it normal, copy B to root, return old root data;
      if its normal - copy A to root. deallocate it, return old root data;



List as FIFO semantic:

      Oueue:
      1 2 3 ... N , where 1 is last come and N is first to out

      [root N] --head-> [N-1] -> [N-2] -> ... [3] -> [2] -> [1]
        |                                                   ^
        |                                                   |
        `----tail-------------------------------------------′

*/

// ========================================================================== //

#define ROOT_PAYLOAD 5
#define NODE_PAYLOAD 7
#define TAIL_PAYLOAD 8

// 64 bit bit-filed node struct to access data and indexes, unioned so same
// node_t can be seen: as_root, as_node, as_tail and as_pfree.
typedef union
{
    struct
    {
        unsigned char  data[NODE_PAYLOAD] ;
        unsigned char  next;
    } as_node;
    struct
    {
        unsigned char  data[TAIL_PAYLOAD] ;
    } as_tail;
    struct
    {
        unsigned char  data[ROOT_PAYLOAD] ;
        unsigned char  head;
        unsigned char  tail;
        unsigned char  cnth : 4 ;
        unsigned char  cntt : 4 ;
    } as_root;
    unsigned long int as_pfree;
    unsigned int      as_ints[2];
} __attribute__((packed)) node_t;

// TODO: how abount like uint64_t ?
static_assert(sizeof(unsigned long int) == 8, "Algorithm relies on 8 byte longs");
static_assert(sizeof(unsigned int) == 4,      "Algorithm relies on 4 byte ints");
static_assert(sizeof(node_t) == 8,            "Algorithm relies on 4 byte nodes");
static_assert(sizeof(char) == 1,              "In case C standard violated by compiler");
static_assert(CHAR_BIT == 8,                  "In case platform is weird");

#define NODE_COUNT 256

// Static buffer for data - can be set from outside
// with initQueues() call, and its len. No other data used.
static node_t* buffer;
static int     buffer_len;


// Callbacks
static onOutOfMem_cb_t onOutOfMemory;
static onIllegalOperation_cb_t onIllegalOperation;


// ========================================================================== //

// helper, returns true if pointer is withit [buffer, buffer + MA
static inline bool bounds_check(node_t* node);

// Get queue root
static inline node_t* get_queue_root(Q* q);

// get node's index
static inline unsigned char node_to_index(node_t* node);

// get node by index
static inline node_t* index_to_node(unsigned char index);



// empty root has no data and empty head
static inline bool is_empty_root(node_t* root);

// single root has data (cntt bytes), but has empty head
static inline bool is_single_root(node_t* root);

// root that has head and tail the same values
static inline bool is_headtail_root(node_t* root);

// gets head node of root
static inline node_t* get_root_head(node_t* root);

// gets tail node of root
static inline node_t* get_root_tail(node_t* root);

// gets byte from single root
static inline unsigned char pop_single_root_data(node_t* root);

// sets byte info single root
static inline void push_single_root_data(node_t* root, unsigned char b);

// gets byte from root, needs input byte to shift data, 
// non-single root must be full filled, function rotates
// bytes of root's payload to make it FIFO
static inline unsigned char pop_root_data(node_t* root, unsigned char in);

// gets byte from next node, decrements cnth
static inline unsigned char pop_head_data(node_t* root);

// gets byte from prew node, decrements cnth and cntt
// only used when head==tail
static inline unsigned char pop_tail_data(node_t* root);

// gets last byte of tail
static inline unsigned char pop_tail_last(node_t* root, unsigned char next);


// get node's next node
static inline node_t* get_node_next(node_t* node);



// Allocates a node, returns it all zeroed
static node_t* alloc_node();

// Deallocates node, should not be used after free
static void free_node(node_t* node);


// ========================================================================== //


// Core functions

static inline node_t* get_queue_root(Q* q)
{
    node_t* root = (node_t*)q;
    assert(bounds_check(root));
    return root;
}

static inline bool bounds_check(node_t* node)
{
    return (node != NULL) && (buffer < node && node < buffer + NODE_COUNT);
}

static inline unsigned char node_to_index(node_t* node)
{
    assert(bounds_check(node));
    return node - buffer;
}

static inline node_t* index_to_node(unsigned char index)
{
    return buffer + index;
}


// Root nodes related fuctions

static inline bool is_empty_root(node_t* root)
{
    assert(bounds_check(root));
    return root->as_root.head == 0 && root->as_root.cntt == 0;
}

static inline bool is_single_root(node_t* root)
{
    assert(bounds_check(root));
    return root->as_root.head == 0 && root->as_root.cntt != 0;
}

static inline bool is_headtail_root(node_t* root)
{
    assert(bounds_check(root));
    assert(!is_empty_root(root));
    assert(!is_single_root(root));
    return root->as_root.head == root->as_root.tail;
}

static inline node_t* get_root_head(node_t* root)
{
    assert(bounds_check(root));
    assert(!is_empty_root(root));
    assert(!is_single_root(root));

    node_t* h = index_to_node(root->as_root.head);
    assert(h != root);
    return h;
}

static inline node_t* get_root_tail(node_t* root)
{
    assert(bounds_check(root));
    assert(!is_empty_root(root));
    assert(!is_single_root(root));

    node_t* t = index_to_node(root->as_root.tail);
    assert(t != root);
    return t;
}

static inline unsigned char pop_single_root_data(node_t* root)
{
    assert(bounds_check(root));
    assert(!is_empty_root(root));
    assert(is_single_root(root));

    unsigned char cnt = root->as_root.cntt;
    assert(cnt > 0 && cnt <= ROOT_PAYLOAD);

    unsigned char* d = root->as_root.data;
    unsigned char p = d[0];

    // a bit hacky but fast - lets shift entire thing ;)
    unsigned long int root_d = root->as_pfree;
    root_d <<= 8;
    root->as_pfree = root_d;

    // recover and update data
    root->as_root.cntt = cnt - 1;
    root->as_root.head = 0;
    root->as_root.tail = 0;

    return p;
}

static inline void push_single_root_data(node_t* root, unsigned char b)
{
    assert(bounds_check(root));
    assert(!is_empty_root(root));
    assert(is_single_root(root));

    unsigned char cnt = root->as_root.cntt;
    assert(cnt > 0 && cnt <= ROOT_PAYLOAD);

    unsigned char* d = root->as_root.data;
    d[cnt] = b;
    root->as_root.cntt = cnt + 1;
}

static inline unsigned char pop_root_data(node_t* root, unsigned char new)
{
    assert(bounds_check(root));
    assert(!is_empty_root(root));
    assert(!is_single_root(root));


    unsigned char* d = root->as_root.data;
    unsigned char p = d[0];

    /* unsigned char h = root->as_root.head; */
    /* unsigned char t = root->as_root.tail; */

    // a bit hacky but fast - lets shift entire thing ;)
    unsigned int root_d = root->as_ints[0];
    root_d <<= 8;
    root->as_ints[0] = root_d;
    
    d[3] = d[4];
    d[4] = new;

    return p;
}

static inline unsigned char pop_head_data(node_t* root)
{
    assert(bounds_check(root));
    assert(!is_empty_root(root));
    assert(!is_single_root(root));
    assert(!is_headtail_root(root));

    node_t* head = get_root_head(root);

    unsigned char cnt = root->as_root.cnth;
    assert(cnt > 0 && cnt <= NODE_PAYLOAD);

    unsigned char* d = head->as_node.data;
    unsigned char p = d[0];

    // a bit hacky but fast - lets shift entire thing ;)
    // will have to recover next field though
    unsigned char next = head->as_node.next;
    unsigned long int head_d = head->as_pfree;
    head_d <<= 8;
    head->as_pfree = head_d;
    head->as_node.next = next;
    

    // recover and update data
    root->as_root.cnth = cnt - 1;
    return p;
}

static inline unsigned char pop_tail_data(node_t* root)
{
    assert(bounds_check(root));
    assert(!is_empty_root(root));
    assert(!is_single_root(root));
    assert(is_headtail_root(root));

    node_t* tail = get_root_head(root);

    unsigned char cnt = root->as_root.cntt;
    assert(cnt > 0 && cnt <= TAIL_PAYLOAD);

    unsigned char* d = tail->as_tail.data;
    unsigned char p = d[0];

    unsigned long int tail_d = tail->as_pfree;
    tail_d <<= 8;
    tail->as_pfree = tail_d;

    root->as_root.cntt = cnt - 1;
    root->as_root.cnth = cnt - 1;

    return p;
}

static inline unsigned char pop_tail_last(node_t* root, unsigned char next)
{
    assert(bounds_check(root));
    assert(!is_empty_root(root));
    assert(!is_single_root(root));
    assert(!is_headtail_root(root));

    node_t* tail = get_root_tail(root);
    unsigned char* d = tail->as_node.data;
    unsigned char p = d[0];
}

// Simple node related

static inline node_t* get_node_next(node_t* node)
{
    assert(bounds_check(node));
    return  index_to_node(node->as_node.next);
}



// ========================================================================== //

static node_t* alloc_node()
{

    assert(buffer->as_pfree != 0);

    if (buffer->as_pfree >= NODE_COUNT)
    {
        onOutOfMemory();
        return NULL;
    }

    node_t* ret = index_to_node(buffer->as_pfree);

    if (ret->as_pfree == 0)
    {
        buffer->as_pfree += 1;
    }
    else
    {
        buffer->as_pfree = ret->as_pfree;
        ret->as_pfree = 0;
    }

    return ret;
}

static void free_node(node_t* node)
{
    assert(bounds_check(node));

    node->as_pfree = buffer->as_pfree;
    buffer->as_pfree = node_to_index(node);
}

// ========================================================================== //


int initQueues(unsigned char* buf, unsigned int len)
{
    assert(buf != NULL);
    assert(len == 2048);

    memset(buf, 0, len);

    buffer = (node_t*) buf;
    buffer_len = len;
    buffer->as_pfree = 1;

    return 1343; // return worst case magic constant
}

Q* createQueue()
{
    // create new empty root node and return it as handle
    return (Q*) alloc_node();
}

void destroyQueue(Q* q)
{
    node_t* root = get_queue_root(q);

    if (is_empty_root(root) || is_single_root(root))
    {
        free_node(root);
        return;
    }

    node_t* t = get_root_tail(root);
    node_t* p = get_root_head(root);

    if (t == p)
    {
        free_node(t);
        free_node(root);
        return;
    }

    while (p != t)
    {
        node_t* pp = get_node_next(p);
        free_node(p);
        p = pp;
    }

    free_node(t);
}

void enqueueByte(Q* q, unsigned char b)
{
    node_t* root = get_queue_root(q);

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
    node_t* root = get_queue_root(q);

    if (is_empty_root(root))
    {
        onIllegalOperation();
        return 0;
    }

    if (is_single_root(root))
    {
        return pop_single_root_data(root);
    }

    node_t* h = get_root_head(root); 
    node_t* t = get_root_tail(root);

    if (h == t)
    {

        unsigned char head_ret = pop_tail_data(root); // pops from 8 data field

        free_node(head);
        make_root_single(root);
    }

    unsigned char head_ret = pop_head_data(head); // what if head is also tail!!!!!
    unsigned char ret = pop_root_data(root, head_ret);

    if (!is_empty_head(head)) {
        return ret;
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

    if (is_empty_root(root))
    {
        printf("[empty] - 1 node\n");
        return;
    }

    printf("[non empty =) ] - N nodes\n");
    
    // TODO: count elements, nodes, print nicely

    /*printf("[");

    node_t* p;
    while ((p = get_nxt(root)) != root)
    {
        printf("%d ", p->data);
    }

    printf("%d]\n", root->data );*/
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
