#include "my_malloc.h"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#define MAGIC_NUMBER 757111

FreeListNode find_chunk(uint32_t sz);
FreeListNode split_chunk(FreeListNode chunk, uint32_t sz);
void insert_chunk(FreeListNode node);
void remove_chunk(FreeListNode chunk);

MyErrorNo my_errno=MYNOERROR; // 0
FreeListNode head = NULL;

FreeListNode find_chunk(uint32_t sz)
{
    FreeListNode current = head;

    if (current == NULL) { // If the free list is empty, return false
        printf("Free list is empty\n");
        return 0;
    } 
    if (current -> size >= sz) { // If the first chunk is big enough, return it
        printf("First chunk is big enough, chunk size %d, implement sz: %d \n", current->size, sz);
        return current;
    } 
    if (current -> flink == NULL) { // If the first chunk is not big enough and there are no more chunks to check, return false
        printf("First chunk not big enough and there are no more to check\n");
        return 0;
    }
    while (current -> flink != NULL) { // Iterate through the free list
        if (current -> flink -> size >= sz) { // If the next chunk is big enough, return it
            printf("Next chunk is big enough\n");
            return current -> flink;
        }
        current = current -> flink;
        if (current -> flink == NULL) { // If the next chunk is not big enough and there are no more chunks to check, return false
            printf("Next chunk not big enough and there are no more to check\n");
            return 0;
        }
    }

}

FreeListNode split_chunk(FreeListNode chunk, uint32_t sz)
{
    if (chunk->size - sz < 16) { // If the chunk is not big enough to split, return it
        printf("Chunk not big enough to split, size: %d\n", chunk->size);
        return chunk;
    }
    uint32_t oldsize = chunk -> size;
    printf("Begin split_chunk, oldsize: %d\n", oldsize);
    FreeListNode new_chunk = (FreeListNode)((uint8_t *)chunk + sz + CHUNKHEADERSIZE); // Create a new chunk with the remaining size
    // Change the current size stored in chunk to the new size
    chunk -> size = sz;
    printf("Chunk size changed to: %d\n", chunk->size);
    // Create a new chunk with the remaining size
    new_chunk -> size = oldsize - sz - CHUNKHEADERSIZE;
    printf("New chunk size: %d\n", new_chunk->size);
    // Link the flink in the new chunk to the flink in the old chunk
    new_chunk -> flink = chunk -> flink;
    // Link the flink in the old chunk to the new chunk
    chunk -> flink = new_chunk;
    // Return the old chunk
    printf("End split_chunk, returned chunk: %p\n", chunk);
    return chunk;
    
}

void insert_chunk(FreeListNode node)
{
    if (head == NULL) { // If the free list is empty, make the new node the head
        head = node;
        printf("Inserted chunk at head\n");
        return;
    }
    if (node < head) { // If the new node is smaller than the head, make the new node the head
        node -> flink = head;
        head = node;
        printf("Inserted smaller chunk at head and put old head at flink\n");
        return;
    }
    FreeListNode current = head;
    while (current -> flink != NULL) { // Iterate through the free list
        if (node < current -> flink) { // If the new node is smaller than the next node, but larger than the current, insert the new node
            node -> flink = current -> flink;
            current -> flink = node;
            printf("Inserted chunk in the middle, %p\n", node);
            return;
        }
        current = current -> flink;
    }
    current -> flink = node; // If the new node is the largest, insert it at the end
}

void remove_chunk(FreeListNode chunk)
{
    if (head == NULL) {
        printf("Free list empty: return null\n"); // If the free list is empty, return
        return;
    }
    if (head == chunk) { // If the chunk is the head, remove it
        head = chunk -> flink;
        printf("Chunk == head, removed head\n");
        return;
    }

    FreeListNode current = head;
    while (current -> flink != NULL) { // Iterate through the free list
        if (current -> flink == chunk) { // If the next chunk is the one to remove, remove it
            current -> flink = chunk -> flink;
            printf("Removed chunk: %p\n", chunk);
            return;
        }
        current = current -> flink;
    }
}

void *my_malloc(uint32_t size) {
    if (size == 0) {
        printf("Size is 0, return NULL\n");
        return NULL;
    }
    printf("Size: %d Bytes, Before Adjustments\n", size);
    size = (size % 8 != 0) ? size + (8 - (size % 8)) : size;
    size += CHUNKHEADERSIZE;
    if (size < 16) {
        size = 16; 
    }
    printf("Size: %d Bytes, After Adjustments\n", size);

    void* chunk = NULL; 

    FreeListNode freelist_chunk = find_chunk(size);
    if (freelist_chunk == NULL) {
        printf("No chunk was found -> call sbrk()\n"); // If no chunk found, sbrk()
        size_t allocate_size = (size <= 8192) ? 8192 : size; 
        chunk = sbrk(allocate_size);
        if (chunk == (void*)-1) {
            perror("sbrk() failed after trying to allocate_size 8192 bytes\n");
            my_errno = MYENOMEM;
            return NULL;
        }
        
        if (allocate_size == 8192) {
            printf("Allocated 8192 bytes, create new node and insert it into the free list\n");
            FreeListNode new_node = (FreeListNode)chunk;
            new_node->size = 8192;
            new_node->flink = NULL;
            printf("Call insert_chunk()\n");
            insert_chunk(new_node);
            printf("inset_chunk() called and returned\n");

            freelist_chunk = find_chunk(size);
            printf("freelist_chunk: %p \n", freelist_chunk);
            if (freelist_chunk == NULL) {
                perror("freelist_chunk is NULL after sbrk()\n");
                my_errno = MYENOMEM;
                return NULL;
            }
            chunk = split_chunk(freelist_chunk, size);
            printf("split_chunk called on sbrk(8192), Chunk: %p\n", chunk);
            printf("remove_chunk() called on sbrk() chunk: %p\n", chunk);
            remove_chunk(chunk);
            
            
        }
    } else {
        printf("Chunk was found, call split_chunk()\n");
        chunk = split_chunk(freelist_chunk, size);
        printf("split_chunk() called when chunk found on FreeLL, Chunk: %p\n", chunk);
        remove_chunk(chunk);
        printf("remove_chunk() called on freelist chunk: %p\n", chunk);
    }

    if (chunk != NULL) {
        uint32_t* header = (uint32_t*)chunk;
        *header = size;
        *(header + 1) = MAGIC_NUMBER;
        printf("Header: %p, Size: %d, Magic Number: %d\n", header, *header, *(header + 1));
        return (void*)(header + 2); 
    } else {
        printf("Chunk is NULL at the end of malloc, return NULL\n");
        my_errno = MYENOMEM;
        return NULL;
    }
}
      
void my_free(void *ptr) 
{
    if (ptr == NULL) {
        printf("Pointer is NULL, return\n");
        my_errno = MYBADFREEPTR;
        return;
    }
    uint32_t* header = (uint32_t*)(ptr - CHUNKHEADERSIZE);
    if (*(header + 1) != MAGIC_NUMBER) {
        printf("Magic number is not correct, return\n");
        my_errno = MYBADFREEPTR;
        return;
    }
    FreeListNode new_node = (FreeListNode)header;
    new_node->size = *header;
    new_node->flink = NULL;
    insert_chunk(new_node);
    printf("insert_chunk() of size: %d\n", new_node->size);

}

FreeListNode free_list_begin()
{
    if (head == NULL) {
        return NULL;
    }
    return head;
}

void coalesce_free_list() // If two addresses of the nodes are adjacent, merge them (in the memory address space). Look for corner cases
{
    FreeListNode current = head;
    printf("Begin coalesce_free_list() current: %p\n", current);
    while (current -> flink != NULL) {
        printf("Current: %p, Size: %d, Flink: %p\n", current, current->size, current->flink);
        if ((uint8_t*)current + current->size  == (uint8_t*)current->flink) {
            printf("Coalescing because current + size: %p == flink: %p\n", (uint8_t*)current + current->size, (uint8_t*)current->flink);
            current->size += current->flink->size;
            current->flink = current->flink->flink;
        }
        current = current->flink;
    }
}
