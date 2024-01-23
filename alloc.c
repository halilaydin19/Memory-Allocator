#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#define ALIGNMENT sizeof(long)
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))  

struct FreeBlock {
    size_t size;
    struct FreeBlock* next;
}; //in bound data to use representing a block of free memory
static struct FreeBlock* free_list = NULL; //Head of free blocks linklist
static void* batch_start = NULL; //To control batch start 
static size_t batch_remaining = 0; //To control batch remaining
#define BATCH_SIZE 4096 //Each size of batch is 4kb

void* get_batch(size_t size) { //To allacote new memory batch with using sbrk.Each batch's size is 4096 bytes. 
    
    size_t batch_size = (size / BATCH_SIZE + 1) * BATCH_SIZE; 
    void* batch_ptr = sbrk(batch_size);

    if (batch_ptr == (void*)-1) {
        return NULL; //If sbrk failed

    }

    batch_start = batch_ptr;
    batch_remaining = batch_size;


    return batch_ptr;
}

void *kumalloc(size_t size) {
    
    
    size_t allocated_size = ALIGN(size + SIZE_T_SIZE); //Calculates the size that we should allocate

    if (free_list != NULL && free_list->size >= allocated_size) {
        //Checks our free list and determines whether it has enough size to satisfy requested allocation.
        
        struct FreeBlock* free_block = free_list; 
        free_list = free_list->next;
        size_t remaining_size = free_block->size - allocated_size; //Removes free block from freelist and calculates remaining size.
                                                            //If remaining size is enough to use again. It will be added back to freelist.

        if (remaining_size >= SIZE_T_SIZE) {//Add back to freelist.
            struct FreeBlock* new_free_block = (struct FreeBlock*)((char*)free_block + allocated_size);
            new_free_block->size = remaining_size;
            new_free_block->next = free_list;
            free_list = new_free_block;
        }
        size_t* header = (size_t*)free_block;//to return head of freelist
        *header = allocated_size | 1; 
        return (char*)header + SIZE_T_SIZE;
    }

    if (batch_remaining < allocated_size) {//If there is no enough memory in current batch that we use 
                                     //We call get_batch to allocate new memory with using new batch.
        if (get_batch(allocated_size) == NULL) {
            return NULL;
        }
    }
    //If there is enough memory in both freelist and current bacth.
    size_t* header = (size_t*)batch_start;
    *header = allocated_size | 1; 
    batch_start = (char*)batch_start + allocated_size;
    batch_remaining -= allocated_size;
    return (char*)header + SIZE_T_SIZE;
}




void *kucalloc(size_t nmemb, size_t size) 
{

    if(nmemb == 0 || size == 0 ){  //Checks wheter nmemb and size is not zero
        return NULL;
    }

    size_t total_size = nmemb * size;
    void *ptr = kumalloc(total_size);

    if (ptr != NULL) { //If allocation is succesfull we can use allocated memory, So it iterates over allocated memory and initializes to 0.
        char *byte_ptr = ptr;
        for (size_t i = 0; i < total_size; ++i) {
            byte_ptr[i] = 0;
        }
    }

    return ptr;
}

void kufree(void *ptr) {

    if (!ptr) {// If there is nothing to free
        return;
    }
    size_t *header = (size_t *)((char *)ptr - SIZE_T_SIZE);
    *header = *header & ~1L; //marks least significant bit free to mark the block as free

    struct FreeBlock* free_block = (struct FreeBlock*)header;//Adds freeblock to free list to reuse for future allocations
    free_block->size = *header & ~1L;
    free_block->next = free_list;
    free_list = free_block;

    
}

void *kurealloc(void *ptr, size_t size)
{
    if (ptr == NULL) {//If null here is no memory so just behaves like malloc.
        return kumalloc(size);
    }

    size_t *header = (size_t *)((char *)ptr - SIZE_T_SIZE);
    size_t old_size = *header & ~1L; //to retrive old size of data

    if (size <= old_size - SIZE_T_SIZE) {//If requested size is smaller there is no need to allocation.
        return ptr;
    }
//Else reacllocates memory with requested size and then copies the content of data.
    void *new_ptr = kumalloc(size);
    if (new_ptr != NULL) {
        char *src = (char *)ptr;
        char *dest = (char *)new_ptr;
        for (size_t i = 0; i < old_size - SIZE_T_SIZE; ++i) {
            dest[i] = src[i];
        }
        kufree(ptr);
    }

    return new_ptr;
}
/*
 * Enable the code below to enable system allocator support for your allocator.
 * Doing so will make debugging much harder (e.g., using printf may result in
 * infinite loops).
 */
#if 0
void *malloc(size_t size) { return kumalloc(size); }
void *calloc(size_t nmemb, size_t size) { return kucalloc(nmemb, size); }
void *realloc(void *ptr, size_t size) { return kurealloc(ptr, size); }
void free(void *ptr) { kufree(ptr); }
#endif
