#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pool_alloc.h"

/* 
* Program functionality:
* This file describes the implementation of a block pool memory allocator,
* designed for quicker memory allocation given predefined block sizes. 
* The defined constants can be changed, the POOLS constant in particular
* is used to simplify the creation of a list of structs and imposes a limit
* on the number of block sizes allowed. Users are expected to handle error
* cases. Detailed function-specific comments are provided below.

* This program is NOT thread-safe as the memory footprint must be fixed. 
* Implementing thread-safe functionality could be done using libraries
* such as POSIX thread.
*
* Author: Sachin Sulkunte
*/

#define HEAP_SIZE 65536 // given heap size
#define POOLS 5     // imposed upper limit on number of block sizes (adjustable)

static uint8_t g_pool_heap[HEAP_SIZE]; // for easy modification of heap size if needed

// linked list node for keeping track of free blocks
typedef struct {
    void* next;
} list_node;

typedef struct {
    uint16_t allocated;      // num of contiguous blocks allocated
    uint8_t* pool_start;     // start address of pool
    uint8_t* pool_end;       // end address of pool
    uint16_t max;            // max blocks in partition
    list_node* head;         // pointer to free blocks
    size_t block_size;       // tunable block size given by user
} pool_obj;

static pool_obj pool_list[POOLS]; // defined pools for each block size

/*
 * This function takes in a pointer to an array of block sizes as well
 * as the count of how many block sizes there are. The number of partitions
 * are defined and each pool is intitalized with its parameters.
 * Returns: True - if initialization is successful, else - False
 */
bool pool_init(const size_t* block_sizes, size_t block_size_count)
{
    // upper limit surpassed or negative number of blocks (assumed max of 256 blocks)
    if (block_size_count > POOLS || (int8_t) block_size_count <= 0) {
        //fprintf(stderr, "Err: Invalid parameters\n");
        return false;
    }

    size_t unused = HEAP_SIZE;
    // Assumption - user wants equal-sized partitions for all block sizes
    uint16_t partition = unused / block_size_count;

    uint8_t* heap_start = g_pool_heap;
    uint8_t* heap_end = g_pool_heap + sizeof(g_pool_heap);
    uint8_t* current_addr = heap_start;

    // determine if any block_sizes are invalid
    for (int i = 0; i < block_size_count; ++i) {
        if (block_sizes[i] > partition || current_addr > heap_end
        || (int16_t) block_sizes[i] <= 0) { 
          return false;
        }
        
        // define pool for specific block size
        pool_list[i].pool_start = current_addr;
        pool_list[i].head = NULL;
        pool_list[i].block_size = block_sizes[i];
        pool_list[i].max = partition / block_sizes[i]; // any excess partial block is ignored
        pool_list[i].pool_end = current_addr + (pool_list[i].max * pool_list[i].block_size);
        unused -= partition;
        current_addr += (pool_list[i].max * pool_list[i].block_size);
    }
    // successful initialization of pools
    return true;
}

/*
 * This function is passed an unsigned value corresponding to the desired
 * memory size to be allocated. Algorithm follows a best-fit approach, the
 * smallest block size that can meet the needs of the user is allocated
 * to the request. If all blocks are full, the memory is allocated from the
 * next largest pool.
 *
 * O(n) operation where n = POOLS
 *
 * Returns: Pointer to allocated memory if successful, NULL if failed
 */ 
void* pool_malloc(size_t n)
{

    if ((int64_t)n <= 0) {
      //fprintf(stderr, "Err: Cannot allocate non-positive value\n");
      return NULL; // failure case - cannot allocate negative value
    }

    pool_obj* curr_pool = NULL;

    // determine which pool to allocate from
    size_t optimal_block = UINT16_MAX;
    uint16_t select_partition = POOLS + 1; // invalid partitition number

    for (int i = 0; i < POOLS; i++) {
        // find smallest block size that fits n in a non-full pool
        if (pool_list[i].block_size <= optimal_block && n <= pool_list[i].block_size 
        && (pool_list[i].allocated < pool_list[i].max || pool_list[i].head != NULL)) {
            optimal_block = pool_list[i].block_size;
            select_partition = i;
        }
    }

    if (select_partition == POOLS + 1) {
      //fprintf(stderr, "Err: No suitable memory pool found\n");
      return NULL; // all partitions' blocks are too small or full to hold this data
    }
    curr_pool = &pool_list[select_partition];

    // memory to be allocated
    list_node* current = NULL;

    if (curr_pool->head == NULL) {
      // get position of block to be allocated
      current = (void *)(curr_pool->pool_start + (curr_pool->allocated * curr_pool->block_size)); 
      curr_pool->allocated++;
    }
    else {
      current = curr_pool->head; // first free block is allocated
      curr_pool->head = curr_pool->head->next; // list is updated to remove allocated block
    } 

    return current; // pointer to memory allocated
}

/*
 * This function deallocates memory blocks based on the ptr parameter.
 * The parameter must be a pointer corresponding to a valid place in memory.
 * The newly-freed memory is assigned to the head of the unused memory and
 * will be used next when allocating new memory.
 *
 * O(n) operation where n = POOLS
 *
 * Returns: No return value.
 */
void pool_free(void* ptr)
{

    if (ptr == NULL) {
      fprintf(stderr, "\tErr: NULL pointer");
      return; // no processing to be done
    }

    pool_obj* curr_pool = NULL;

    // determine which partition ptr belongs to
    for (int i = 0; i < POOLS; ++i) {

        // determine whether the ptr corresponds to the correct block_size for partition
        if (ptr >= (void*)pool_list[i].pool_start && ptr < (void*)pool_list[i].pool_end
                && ((uint8_t*)ptr - pool_list[i].pool_start) % pool_list[i].block_size == 0) {

            curr_pool = &pool_list[i];
        }
    }

    if (curr_pool == NULL) {
      //fprintf(stderr, "\tErr: Pointer does not correspond to allocated memory\n");
      return; // ptr not found - fail case
    }

    list_node* ptr_free = (list_node*)ptr;

    ptr_free->next = curr_pool->head; // freed memory becomes new head of list for partition
    curr_pool->head = ptr_free;
}
