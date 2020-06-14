#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#define HEADER_SIZE 9
/*  8 bytes - how much free space in current block
    1 byte - is_reserved byte (0x00 if free, 0xFF if reserved)  */
#define HEAP_SIZE 8000 // 1000 64-bit numbers


void* memory;

typedef struct header{
    size_t block_size;
    uint8_t is_reserved;
}header_t;

void set_header(uint8_t* ptr, size_t size, uint8_t is_reserved){
    // set first 8 bytes as int64_t, which means
    // how much space this block takes (in bytes)
    // split int64_t into 8 uint8_t and write them separatelly
    for (int i = 0; i < 8; i++){
       ((uint8_t*)ptr)[i] = size & 0xFF;
       size >>= 8; 
    }
    // set is_reserved byte
    ((uint8_t*)ptr)[HEADER_SIZE - 1] = is_reserved;
}

header_t get_header(uint8_t* ptr){
    header_t h;
    h.block_size = 0;
    for (int i = 7; i >= 0; i--){
        h.block_size <<= 8; 
        h.block_size |= ((uint8_t*)ptr)[i];
    }
    h.is_reserved = ptr[HEADER_SIZE - 1];
    return h;
} 

void* e_malloc(size_t size){
    if (memory == NULL){
        // initialization of memory
        memory = malloc(HEAP_SIZE);
        // in the beginning it is single
        // free block of memory
        set_header(memory, HEAP_SIZE - HEADER_SIZE, 0x00);
    }
    // find first available block of memory
    // with enough size
    uint8_t* current_block = (uint8_t*) memory;
    header_t current_header = get_header(current_block);

    header_t next_header;
        
    while (current_header.block_size < size 
            || current_header.is_reserved == 0xFF){
        // if current block is not reserved
        // and next block is not reserved
        // we can merge them
        next_header = get_header(current_block + HEADER_SIZE + current_header.block_size);
        if (current_header.is_reserved == 0x00
            && next_header.is_reserved == 0x00 ){
                set_header(current_block, current_header.block_size + next_header.block_size + HEADER_SIZE, 0x00);
            } else{
                // else move to next header
                current_block += current_header.block_size  + HEADER_SIZE;
            }
        current_header = get_header(current_block);
    }

    
    // make current block reserved
    // set_header(current_block, size, 0xFF);
    
    // printf("%lu\n", current_header.block_size);

    // create next memory block
    // to call it next time

    if (current_header.block_size <= size + HEADER_SIZE){
        /*
                                ***NOTE***
            Remember, that when we try to get memory from current block,
            we split it into two, so first part of current block becomes
            our just allocated memory and second part becomes separate
            memory block, so we need to initialize it by header.
            And if second block is so small that we can't put here
            even header, we just add it's size to first block and
            second block doesn't exist any more.                       
        */
        // just reserve current block and return it
        set_header(current_block, current_header.block_size, 0xFF);
        return current_block + HEADER_SIZE;
    }
    
    size_t memory_for_left_block = current_header.block_size - (size + HEADER_SIZE);
    // printf("left %ld bytes\n", memory_for_left_block);

    // reserve current_block
    set_header(current_block, size, 0xFF); 

    // reserve next block
    set_header(current_block + HEADER_SIZE + size, memory_for_left_block, 0x00);


    return current_block + HEADER_SIZE;
}

void e_free(void* ptr){
    ptr -= HEADER_SIZE;
    header_t cur = get_header(ptr);
    set_header(ptr, cur.block_size, 0x00);
}

void memory_check(void){
    printf("***MEMORY CHECKING***\n");
    uint8_t* current_block = (uint8_t*) memory;
    int64_t current_block_pos = 0;
    header_t current_header = get_header(current_block);

    while (current_block_pos + current_header.block_size + HEADER_SIZE <  HEAP_SIZE){
        printf("detected block with size=%lu, is_reserved=0x%x\n", 
                    current_header.block_size, current_header.is_reserved);
        current_block_pos += current_header.block_size + HEADER_SIZE;
        current_block = memory + current_block_pos; 
        current_header = get_header(current_block);
    }

     // print last block
     printf("detected block with size=%lu, is_reserved=0x%x\n\n", 
                    current_header.block_size, current_header.is_reserved);
}

void test(){
    char* a = e_malloc(30);
    char* b = e_malloc(30);
    char* c = e_malloc(30);
    e_free(a);
    e_free(b);
    e_free(c);
    memory_check();

    /*
        Instead of block with size=100 we will see block
    with size=108, because when we merge freed blocks of 'a', 'b' and 'c'
    also we have extra 18 bytes, which was headers of 'b' and 'c' and now
    we do not need them and they become a part of 'a'.
        As we allocated 100 bytes from 108-byte block,
    there are 8 bytes left. We can't put there even header (which size is 9 bytes),
    so we just add this 8 bytes to currently created 'd', so it will have size 100+8=108
    */
    char* d = e_malloc(100);
    memory_check();
    
}

int main(){
    test();
    free(memory);
}


/*
                            TODO
    -create block after current, to find it in next e_malloc          | [+]
    -check memory limits                                              | []
    -merge nearby free blocks                                         | [+]
    -memory_check                                                     | [+]
*/