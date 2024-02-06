#include "../include/allocation.h"
#include <stdlib.h>
#include <stdio.h>

#define BUFFER_SIZE 255


void* allocate_array(size_t member_size, size_t nmember,bool clear)
{
    if(member_size <= 0 || nmember <= 0) return NULL;
    void * array = NULL;
    if(clear) {
        array = calloc(member_size, nmember);
    }
    else{
        array = malloc(member_size * nmember);
    }
    return array;

}

void* reallocate_array(void* ptr, size_t size)
{
    if(ptr == NULL || size <= 0) return NULL;
    void * new_array = realloc(ptr, size);
    return new_array;
}

void deallocate_array(void** ptr)
{
    if(ptr == NULL || *ptr == NULL) return;
    else {
        free(*ptr);
        *ptr = NULL;
        return;
    }
}

char* read_line_to_buffer(char* filename)
{
    if(filename == NULL) return NULL;
    FILE * file = fopen(filename, "r");
    if(file == NULL) return NULL;
    char * buffer = (char*)calloc(BUFFER_SIZE, 1);
    buffer = fgets(buffer, BUFFER_SIZE, file);
    fclose(file);
    return buffer;
}
