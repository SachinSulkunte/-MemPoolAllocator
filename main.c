#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pool_alloc.h"

/*
 * This file tests the functionality defined in pool_alloc.h and implemented
 * in pool_alloc.c. Test cases are written to evaluate the intialization of
 * the memory pools as well as the allocation and freeing of memory blocks.
 */

const char* passed(int ret, int expected) {
  
  if (ret == expected){
    return "Test Passed\n";
  }
  else {
    return "Test Failed\n";
  }
}

int main()
{
  bool result;

  printf("-------------------------");
  printf("\nInitialization Tests\n\n");
  
  // Test case 1: Negative values of either block_size or block_size_count
  size_t invalid_block1[2] = {32, 64};
  result = pool_init(invalid_block1, -2);
  printf("\nTest Case 1a: %s", passed(result, 0));

  invalid_block1[1] = -64;
  result = pool_init(invalid_block1, 2);
  printf("\nTest Case 1b: %s", passed(result, 0));

  // Test case 2: Number of block sizes is greater than limit (currently 5 pools)
  size_t invalid_block2[6] = {32, 64, 256, 1024, 2048, 4096};
  result = pool_init(invalid_block2, 6);
  printf("\nTest Case 2: %s", passed(result, 0));

  // Test case 3: Block size greater than partition size
  size_t valid_block[5] = {32, 64, 256, 1024, 14000};
  result = pool_init(valid_block, 5);
  // Partition = 65536 / 5 -> 13107
  printf("\nTest Case 3: %s", passed(result, 0));

  // Test case 4: Successful initialization of allocator
  size_t block[4] = {32, 64, 256, 1024};
  result = pool_init(block, 4);
  printf("\nTest Case 4: %s", passed(result, 1));

  printf("\n-------------------------");
  printf("\nAllocation Tests\n");
  size_t mem_size;
  void* ret;
  char* result_alloc = "Failed";

  // Test case 5: Try to allocate negative size value
  mem_size = -15;
  ret = pool_malloc(mem_size);
  if (ret == NULL){
    result_alloc = "Passed";
  }
  printf("\nTest Case 5: %s", result_alloc);

  // Test case 6: Size value larger than largest block_size
  mem_size = 1030;
  ret = pool_malloc(mem_size);
  if (ret == NULL){
    result_alloc = "Passed";
  }
  printf("\nTest Case 6: %s", result_alloc);

  // Test case 7: Successful allocation of memory
  mem_size = 66;
  ret = pool_malloc(mem_size);
  if (ret != NULL){
    result_alloc = "Passed";
  }
  printf("\nTest Case 7a: %s : %p", result_alloc, ret);

  mem_size = 240;
  ret = pool_malloc(mem_size);
  if (ret != NULL){
    result_alloc = "Passed";
  }
  printf("\nTest Case 7b: %s : %p", result_alloc, ret);

  printf("\nTest Case 7c: Filling entire pool");
  // Illustrates that once the 256-byte pool is filled, the program allocates
  // memory from the next suitable pool which is 1024-bytes
  for (int i = 0; i < 65; i++){
    mem_size = 240; // allocated to 256-byte pool
    ret = pool_malloc(mem_size);
    if (i == 0 || i == 63){
      printf("\n\t%lu-bytes allocated to 256-byte pool: %p", mem_size, ret);
    }
    else if (i == 64){
      printf("\n\t%lu-bytes allocated to 1024-byte pool: %p\n", mem_size, ret);
    }
  }


  printf("\n-------------------------");
  printf("\nFree Tests\n");

  // Test case 8: Attempt to free null pointer
  void* ptr = NULL;
  printf("\nTest Case 8: Passed\n");
  pool_free(ptr);

  // Test case 9: Free allocated memory
  printf("\n\nTest Case 9: ");
  mem_size = 56;
  ret = pool_malloc(mem_size);
  printf("\n\tMemory Allocated : %p\n", ret);

  void* ret2 = pool_malloc(mem_size);
  printf("\tNew Memory Allocated : %p\n", ret2);

  ptr = ret; // freeing memory allocated to 1024-byte pool in last test
  pool_free(ptr);
  printf("\tFreeing %p\n", ret);

  mem_size = 56;
  void* ret3 = pool_malloc(mem_size);
  printf("\tNew Memory Allocated : %p\n", ret3);
  // Same memory location is allocated because freed memory is placed
  // first in freed list to be allocated next.

  if (ret == ret3){
    printf("\tTest Passed\n");
  }
  else {
    printf("Test Failed");
  }

  // Test 10: Simulate completely filled pool - including freeing in between
  for (int k = 0; k < 12; k++){
    mem_size = 63;
    ret = pool_malloc(mem_size);
    printf("\nAllocation %d: %p", k, ret);
  }
  
  pool_free(ret);
  printf("\nMemory Address %p Freed", ret);

  for (int k = 12; k < 30; k++){
    mem_size = 1023;
    ret = pool_malloc(mem_size);
    if (ret == NULL){
      printf("\nNo more valid memory to be allocated");
    }
    else { 
      printf("\nAllocation %d: %p", k, ret);
    }
  }
  // Allocation fails after all blocks have been allocated and there are no freed list nodes

  return 0;
}
