#include "my_malloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stddef.h>

int main() { // TEst Implementation -> Check for Segmentation Faults
    char *cp, *oldcp=NULL;
    
    // Seed the random number generator
    srand((unsigned int)time(NULL));

    for( int i=0; i<10; i++, oldcp=cp ){
        // Generate a random size between 0 and 10000
        uint32_t sz = rand() % 10001;
        
        cp = (char *)my_malloc(sz);
        printf("%d: malloc(%u) -> %p\n", i, sz, cp);
        
        if( oldcp != NULL ){
            printf("\t%ld bytes from last allocation\n", (ptrdiff_t)(cp-oldcp));
        }
    }
    
    return 0;
}

int main() {
    uint32_t sz;
    char *cp, *oldcp=NULL;
    
    printf("Enter allocation size: ");
    scanf("%u", &sz);
    
    for( int i=0; i<20; i++, oldcp=cp ){
        cp = (char *)my_malloc(sz);
        printf("%d: malloc(%d) -> %p\n", i, sz, cp);
        if (i % 2 == 0) {
            my_free(oldcp);
            printf("%d: free(%p)\n", i, oldcp);
        }
    }
    coalesce_free_list();
    
    return 0;
}

int main() {
    // Seed the random number generator
    srand((unsigned int)time(NULL));
    uint32_t itr = 50;
    const int itr = 50; // Define a constant value for itr
    char *ptrs[itr]; // Keep track of allocated pointers to free them later
    
    // First, allocate 10 blocks of random sizes.
    for (int i = 0; i < itr; i++) {
        uint32_t sz = rand() % 10001; // Random size between 0 and 10000
        ptrs[i] = (char *)my_malloc(sz);
        printf("%d: malloc(%u) -> %p\n", i, sz, ptrs[i]);
    }
    
    // Now, free every other block to create opportunities for coalescing.
    for (int i = 0; i < itr; i += 2) {
        my_free(ptrs[i]);
        printf("%d: free(%p)\n", i, ptrs[i]);
        ptrs[i] = NULL; // Avoid dangling pointer
    }
    
    // Print out the free list to see the effect of coalescing
    FreeListNode node = free_list_begin();
    while (node != NULL) {
        printf("Free block at %p, size %u\n", (void*)node, node->size);
        node = node->flink;
    }
    
    // Free the rest of the blocks
    for (int i = 1; i < itr; i += 2) {
        my_free(ptrs[i]);
        printf("%d: free(%p)\n", i, ptrs[i]);
    }
    
    printf("Called coalesce_free_list()\n");
    coalesce_free_list();
    
    return 0;
}