#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


typedef char align[16];

typedef union chunk {
    struct {
        size_t size;
        unsigned is_free;
        union chunk *next;
    } s;
    align stub;
} chunk_header;

chunk_header *head = NULL, *tail = NULL;
pthread_mutex_t mu;

chunk_header *get_free_chunk(size_t size) 
{
    chunk_header *current = head;
    while (current) {
        // first fit
        if (current->s.is_free && current->s.size >= size) {
            return current;
        }
        current = current->s.next;
    }
    return NULL;
}

void *malloc(size_t size) {
    chunk_header *chunk_h;

    if (size <= 0)
        return NULL;

    pthread_mutex_lock(&mu);
    chunk_h = get_free_chunk(size);
    if (chunk_h) {
        chunk_h->s.is_free = 0;
        pthread_mutex_unlock(&mu);
        return (void*)(chunk_h + 1);
    }

    size_t total_size = sizeof(chunk_header) + size;
    void *block = sbrk(total_size);
    if (block == (void*) -1) {
        pthread_mutex_unlock(&mu);
        return NULL;
    }

    chunk_h = block;
    chunk_h->s.size = size;
    chunk_h->s.is_free = 0;
    chunk_h->s.next = NULL;

    if (!head) {
        head = chunk_h;
    }

    if (tail) {
        tail->s.next = chunk_h;
    }
    tail = chunk_h;
    pthread_mutex_unlock(&mu);
    return (void*)(chunk_h + 1);
}

void free(void *block) 
{
    if (!block) {
        return;
    }

    void *program_break;
    chunk_header *chunk_h, *tmp;
    
    pthread_mutex_lock(&mu);
    chunk_h = (chunk_header*)block - 1;
    program_break = sbrk(0);

    if ((char*)block + chunk_h->s.size == program_break) {
        if (head == tail) {
            head = NULL;
            tail = NULL;
        } else {
            tmp = head;
            while (tmp) {
                if (tmp->s.next == tail) {
                    tmp->s.next = NULL;
                    tail = tmp;
                }
                tmp = tmp->s.next;
            }
        }
        sbrk(0 - sizeof(chunk_header) - chunk_h->s.size);
        pthread_mutex_unlock(&mu);
        return;
    }

    chunk_h->s.is_free = 1;
    pthread_mutex_unlock(&mu);
}

void *realloc(void *block, size_t size) 
{
    if (!block || !size) {
        return malloc(size);
    }

    chunk_header *header = (chunk_header*)block - 1;
    // 不需要再次 realloc
    if (header->s.size == size) {
        return block;
    }

    // new block
    void *nblock = malloc(size);
    if (nblock) {
        memcpy(nblock, block, header->s.size);
        free(block);
    }

    return nblock;
}

// alloc an array with {arr_length} elements, and each elements need {ele_size} bytes.
void *calloc(size_t arr_length, size_t ele_size) 
{
    if (arr_length <= 0 || ele_size <= 0)
        return NULL;

    size_t total_size = arr_length * ele_size;
    // overflow
    if (ele_size != total_size / arr_length) {
        return NULL;
    }

    void *block = malloc(total_size);
    if (!block) {
        return NULL;
    }
    memset(block, 0, total_size);
    return block;
}

// reference 
// https://blog.codinglabs.org/articles/a-malloc-tutorial.html#32-%E6%AD%A3%E5%BC%8F%E5%AE%9E%E7%8E%B0
