// Masm heap

// ints because they are addresses in masm and not actual addresses

// int malloc(size) - get (size) bytes

// int free(int) - free data
#ifndef _MASM_HEAP
#define _MASM_HEAP

#include <stdlib.h>
#include <iostream>

#define HEAP_ERR_ALREADY_FREE -1;
#define HEAP_ERR_NOT_ALLOCATED -2;
#define HEAP_ERR_OUT_OF_SPACE -3;
#define HEAP_ERR_INVALID_ARG -4;

struct heap_data {
    int size;
    int used;
    int free;

    int start;
    int end;

    struct heap_chunk *chunks;
    struct heap_chunk *free_chunks;
    int chunks_count;
    int free_chunks_count;
};

struct heap_chunk {
    int size;
    int addr;
    bool free;
};

void heap_init();

int mmalloc(int size);

int mfree(int ptr);

#endif