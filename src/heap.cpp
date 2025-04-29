// Masm heap
// ints because they are addresses in masm and not actual addresses
// int malloc(size) - get (size) bytes
// int free(int) - free data

#include <stdlib.h>
#include "heap.h"

struct heap_data metadata;

void heap_init() {
    metadata.size = 65536;
    metadata.used = 0;
    metadata.free = 65536;

    metadata.start = 0;
    metadata.end = 0;

    metadata.chunks = (struct heap_chunk*)malloc(sizeof(struct heap_chunk)); // ah yes. malloc using malloc
    metadata.free_chunks = (struct heap_chunk*)malloc(sizeof(struct heap_chunk)); // ah yes. malloc using malloc

    metadata.chunks_count = 0;
    metadata.free_chunks_count = 0;
}

int mmalloc(int size) {
    // make new chuck of size (size)
    if (size <= 0) 
        return HEAP_ERR_INVALID_ARG
    else if (metadata.free < size) 
        return HEAP_ERR_OUT_OF_SPACE
    
    struct heap_chunk new_chunk;
    new_chunk.free = false;
    new_chunk.size = size;

    for (int i=0;i<metadata.free_chunks_count;i++) {
        struct heap_chunk *c = &metadata.free_chunks[i];
        if (c->size >= size) {
            // found free chunk
            // split available chunk so if malloc(10) is called and we have a 20 byte chunk it becomes a two 10 byte chunks
            new_chunk.addr = c->addr;

            // append chunk to metadata.chunks
            metadata.chunks = (struct heap_chunk*)realloc(metadata.chunks, metadata.chunks_count+1*sizeof(struct heap_chunk));
            metadata.chunks[metadata.chunks_count++] = new_chunk;

            c->size -= size;
            c->addr += size;
            return new_chunk.addr;
        }
    }
    // actually do work.
    new_chunk.addr = metadata.end;

    metadata.used += size;
    metadata.free -= size;
    metadata.end += size;

    // append chunk to metadata.chunks
    metadata.chunks = (struct heap_chunk*)realloc(metadata.chunks, metadata.chunks_count+1*sizeof(struct heap_chunk));
    metadata.chunks[metadata.chunks_count++] = new_chunk;

    return new_chunk.addr;
}

int mfree(int ptr) {
    // free chunk ptr
    for (int i;i<metadata.chunks_count;i++) {
        struct heap_chunk* c = &metadata.chunks[i];
        if (c->addr == ptr) {
            if (!c->free) return HEAP_ERR_ALREADY_FREE;
            metadata.free+=c->size;
            metadata.used-=c->size;
            c->free = true;

            metadata.free_chunks = (struct heap_chunk*)realloc(metadata.free_chunks, metadata.free_chunks_count+1*sizeof(struct heap_chunk));
            metadata.free_chunks[metadata.free_chunks_count++] = *c;
            return 1;
        }
    }
    return HEAP_ERR_NOT_ALLOCATED;
}