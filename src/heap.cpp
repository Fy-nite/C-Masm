// Masm heap
// ints because they are addresses in masm and not actual addresses
// int malloc(size) - get (size) bytes
// int free(int) - free data

#include <stdlib.h>
#include <string>
#include "heap.h"

struct heap_data metadata;

void delete_chunk(heap_chunk* chunk) {
    if (metadata.first == chunk) {
        if (chunk->next != NULL) {
            metadata.first = chunk->next;
        } else {
            metadata.first = NULL;
        }
    } else {
        chunk->prev->next = chunk->next;
        if (chunk->next != NULL) {
            chunk->next->prev = chunk->prev;
        }
    }
    free(chunk);
}

struct heap_chunk* get_last() {
    struct heap_chunk *last = metadata.first;
    while (last->next != NULL) {
        last = last->next;
    }
    return last;
}

void heap_init() {
    metadata.size = 65436;
    metadata.used = 0;
    metadata.free = 65436;

    metadata.start = 100;
    metadata.end = 100;

    metadata.chunks = 0;
    metadata.first = NULL;
}

int mmalloc(int size) {
    std::cout << "Allocating " << std::to_string(size) << " bytes on the heap" << std::endl;
        // make new chuck of size (size)
    if (size <= 0) 
        return HEAP_ERR_INVALID_ARG
    else if (metadata.free < size) 
        return HEAP_ERR_OUT_OF_SPACE
    
    struct heap_chunk *new_chunk = (struct heap_chunk*)malloc(sizeof(struct heap_chunk));
    new_chunk->free = false;
    new_chunk->size = size;

    struct heap_chunk *c = metadata.first;
    while (c != NULL) {
        if (c->size >= size && c->free) {
            // found free chunk
            // split available chunk so if malloc(10) is called and we have a 20 byte chunk it becomes a two 10 byte chunks
            new_chunk->addr = c->addr;

            // append chunk to metadata.chunks
            if (c->size != size) {
                new_chunk->prev = c->prev;
                new_chunk->next = c;
                c->prev->next = new_chunk;
                c->prev = new_chunk;

                c->size -= size;
                c->addr += size;
                return new_chunk->addr;
            } else {
                c->free = false;
                return c->addr;
            }
        }
        c = c->next;
    }
    // actually do work.
    new_chunk->addr = metadata.end;
    new_chunk->next = NULL;

    metadata.used += size;
    metadata.free -= size;
    metadata.end += size;

    // append chunk to metadata.chunks
    
    if (metadata.first == NULL) {
        new_chunk->prev = NULL;
        metadata.first = new_chunk;
    } else {
        new_chunk->prev = get_last();
        new_chunk->prev->next = new_chunk;
    }

    return new_chunk->addr;
}

int mfree(int ptr) {
    // free chunk ptr
    struct heap_chunk *c = metadata.first;
    while (c != NULL && c->addr <= ptr) {
        if (c->addr == ptr) {
            if (c->free) {return HEAP_ERR_ALREADY_FREE}
            c->free = true;
            defragment();
            return 0;
        }
        c = c->next;
    }
    return HEAP_ERR_NOT_ALLOCATED;
}

void defragment() {
    struct heap_chunk *c = metadata.first;
    while (c != NULL) {
        if (c->free && c->next != NULL && c->next->free) {
            std::cout << "Defragmenting data: Chunk 1 addr: " << std::to_string(c->addr) << " size: " << std::to_string(c->size) << "\nChunk 2 addr" << std::to_string(c->next->addr) << " size: " << std::to_string(c->next->size) << std::endl;
            c->size += c->next->size;
            delete_chunk(c->next);
        }
        c = c->next;
    }
    struct heap_chunk *last = get_last();
    if (last->free) {
        metadata.end -= last->size;
        delete_chunk(last);
    }
}