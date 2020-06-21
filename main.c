#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#define HEADER_SIZE 9
/*  4 bytes - size of allocated array
    4 bytes - capacity of allocated block
    1 byte - is_reserved byte (0x00 if free, 0xFF if reserved)  */
#define HEAP_SIZE 8000 // 1000 64-bit numbers


void* memory;

typedef struct header{
    size_t block_size;
    size_t capacity;
    uint8_t is_reserved;
}header_t;

void set_header(uint8_t* ptr, size_t size, size_t capacity, uint8_t is_reserved){

    // 4 bytes - size of allocated array
    for (int i = 0; i < 4; i++){
        ((uint8_t*)ptr)[i] = size & 0xFF;
        size >>= 8; 
    }

    // 4 bytes - capacity of allocated block
    for (int i = 4; i < 8; i++){
        ((uint8_t*)ptr)[i] = capacity & 0xFF;
        capacity >>= 8; 
    }
    // 1 byte - is_reserved byte (0x00 if free, 0xFF if reserved)
    ((uint8_t*)ptr)[HEADER_SIZE - 1] = is_reserved;
}

header_t get_header(uint8_t* ptr){
    header_t h;

    h.capacity = 0;
    for (int i = 7; i >= 4; i--){
        h.capacity <<= 8; 
        h.capacity |= ((uint8_t*)ptr)[i];
    }

    h.block_size = 0;
    for (int i = 3; i >= 0; i--){
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
        set_header(memory, HEAP_SIZE - HEADER_SIZE, HEAP_SIZE - HEADER_SIZE, 0x00);
    }
    // find first available block of memory
    // with enough capacity
    /*
            ***IMPORTANT***
     capacity and block_size are not the same
    */
    uint8_t* current_block = (uint8_t*) memory;
    header_t current_header = get_header(current_block);

    header_t next_header;
        
    while (current_header.capacity < size 
            || current_header.is_reserved == 0xFF){
        // if current block is not reserved
        // and next block is not reserved
        // we can merge them
        next_header = get_header(current_block + HEADER_SIZE + current_header.capacity);
        if (current_header.is_reserved == 0x00
            && next_header.is_reserved == 0x00 ){
                set_header(current_block, current_header.capacity + next_header.capacity + HEADER_SIZE,
                                current_header.capacity + next_header.capacity + HEADER_SIZE, 0x00);
            } else{
                // else move to next header
                current_block += current_header.capacity  + HEADER_SIZE;
            }
        current_header = get_header(current_block);
    }

    if (current_header.capacity <= size + HEADER_SIZE){
        /*
                                ***NOTE***
        When we try to allocate memory, we find
        first available block with enough size and split it into two.
        But there can be situation, when second part of splitted
        block will have size < HEADER_SIZE, so we even can't put there
        header with information about new block, so we add this memory
        to our just allocated memory's capacity, but it's size doesn't change.  
        So, block_size and capacity are not the same.
                           
        */
        // just reserve current block and return it
        set_header(current_block, size,
                        current_header.capacity, 0xFF);
        return current_block + HEADER_SIZE;
    }
    
    size_t memory_for_left_block = current_header.capacity - (size + HEADER_SIZE);

    // reserve current_block
    set_header(current_block, size, size, 0xFF); 

    // reserve next block
    set_header(current_block + HEADER_SIZE + size, memory_for_left_block, 
                    memory_for_left_block, 0x00);


    return current_block + HEADER_SIZE;
}

void e_free(void* ptr){
    ptr -= HEADER_SIZE;
    header_t cur = get_header(ptr);
    set_header(ptr, cur.capacity, cur.capacity, 0x00);
}

void memory_check(void){
    printf("***MEMORY CHECKING***\n");
    uint8_t* current_block = (uint8_t*) memory;
    int64_t current_block_pos = 0;
    header_t current_header = get_header(current_block);

    while (current_block_pos + current_header.block_size + HEADER_SIZE <  HEAP_SIZE){
        printf("detected block with size=%lu, cap=%lu, is_reserved=0x%x\n", 
                    current_header.block_size, current_header.capacity, current_header.is_reserved);
        current_block_pos += current_header.capacity + HEADER_SIZE;
        current_block = memory + current_block_pos; 
        current_header = get_header(current_block);
    }

     // print last block
     printf("detected block with size=%lu, cap=%lu, is_reserved=0x%x\n\n", 
                    current_header.block_size, current_header.capacity, current_header.is_reserved);
}

void test1(){
    char* a = e_malloc(30);
    char* b = e_malloc(40);
    char* c = e_malloc(50);
    e_free(a);
    e_free(b);
    e_free(c);
    memory_check();
    char* d = e_malloc(130);
    memory_check();
    e_free(d);
}

void test2(){
    char* a = e_malloc(30);
    for (int i = 0; i < 10; i++){
        a[i] = 'a' + i;
    }
    for (int i = 0; i < 10; i++){
        printf("%c", a[i]);
    }
    printf("\n\n");

    char* b = e_malloc(30);
    e_free(a);
    e_free(b);
    memory_check();
    char* d = e_malloc(29);
    memory_check();
    e_free(d);
    
}

int main(){
    test1();

    // this to lines to run
    // 2 independent tests
    free(memory);
    memory = NULL;
    
    test2();
    free(memory);
}