// Masm heap
// ints because they are addresses in masm and not actual addresses
// int malloc(size) - get (size) bytes
// int free(int) - free data

#include <stdlib.h>
#include <string>
#include "heap.h"

struct heap_data metadata;

void heap_init() {
    metadata.size = 65436;
    metadata.used = 0;
    metadata.free = 65436;

    metadata.start = 100;
    metadata.end = 100;

    metadata.chunks = (struct heap_chunk*)malloc(sizeof(struct heap_chunk)); // ah yes. malloc using malloc
    metadata.chunks_count = 0;
}

int mmalloc(int size) {
    std::cout << "Allocating " << std::to_string(size) << " bytes on the heap\n";
    // make new chuck of size (size)
    if (size <= 0) 
        return HEAP_ERR_INVALID_ARG
    else if (metadata.free < size) 
        return HEAP_ERR_OUT_OF_SPACE
    
    struct heap_chunk new_chunk;
    new_chunk.free = false;
    new_chunk.size = size;

    for (int i=0;i<metadata.chunks_count;i++) {
        struct heap_chunk *c = &metadata.chunks[i];
        std::cout << std::to_string(c->size) << std::endl;
        if (c->size >= size && c->free) {
            // found free chunk
            // split available chunk so if malloc(10) is called and we have a 20 byte chunk it becomes a two 10 byte chunks
            new_chunk.addr = c->addr;

            // append chunk to metadata.chunks
            if (c->size != size) {
                metadata.chunks = (struct heap_chunk*)realloc(metadata.chunks, metadata.chunks_count+1*sizeof(struct heap_chunk));
                metadata.chunks[metadata.chunks_count++] = new_chunk;

                c->size -= size;
                c->addr += size;
            } else {
                c->free = false;
                return c->addr;
            }
            return new_chunk.addr;
        }
    }
    // actually do work.
    new_chunk.addr = metadata.end;

    metadata.used += size;
    metadata.free -= size;
    metadata.end += size;

    // append chunk to metadata.chunks
    metadata.chunks = (struct heap_chunk*)realloc(metadata.chunks, (metadata.chunks_count+1)*sizeof(struct heap_chunk));
    metadata.chunks[metadata.chunks_count++] = new_chunk;

    return new_chunk.addr;
}

int mfree(int ptr) {
    // free chunk ptr
    for (int i;i<metadata.chunks_count;i++) {
        struct heap_chunk* c = &metadata.chunks[i];
        if (c->addr == ptr) {
            if (c->free) return HEAP_ERR_ALREADY_FREE;
            metadata.free+=c->size;
            metadata.used-=c->size;
            c->free = true;

            return 1;
        }
    }
    return HEAP_ERR_NOT_ALLOCATED;
}

void combine(int chunk1, int chunk2) {
    struct heap_chunk* chunk1_data = &metadata.chunks[chunk1];
    struct heap_chunk* chunk2_data = &metadata.chunks[chunk2];

    chunk1_data->size += chunk2_data->size;
    for (int i=chunk2;i<metadata.chunks_count;i++) {
        metadata.chunks[i] = metadata.chunks[i+1];
    }
    metadata.chunks_count--;
    metadata.chunks = (struct heap_chunk*)realloc(metadata.chunks, metadata.chunks_count*sizeof(struct heap_chunk));
}

void defragment() {
    for (int i=0;i<metadata.chunks_count;i++) {
        struct heap_chunk c = metadata.chunks[i];
        struct heap_chunk next_c = metadata.chunks[i+1];

        if (c.size + c.addr == next_c->addr && c.free) { // if c is right before ch

        }
    }
    // metadata.chunks_count-=removed_chunks;
    // metadata.chunks = (struct heap_chunk*)realloc(metadata.chunks, metadata.chunks_count*sizeof(struct heap_chunk));
}