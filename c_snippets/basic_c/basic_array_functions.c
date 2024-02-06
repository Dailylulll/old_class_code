#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../include/arrays.h"

// LOOK INTO MEMCPY, MEMCMP, FREAD, and FWRITE

bool array_copy(const void *src, void *dst, const size_t elem_size, const size_t elem_count)
{
    if(src == NULL || dst == NULL || elem_size <= 0 || elem_count <= 0){
        return false;
    } else {
       if(memcpy(dst, src, (elem_count * elem_size))) return true;
       else return false;
    }
    
}

bool array_is_equal(const void *data_one, void *data_two, const size_t elem_size, const size_t elem_count)
{
    if(data_one == data_two) return true;
    else if(data_one == NULL || data_two == NULL || elem_size <= 0 || elem_count <= 0) {
        return false;
    } else {
        if(memcmp(data_one, data_two, (elem_size * elem_count)) == 0) return true;
        else return false;
    }
}

ssize_t array_locate(const void *data, const void *target, const size_t elem_size, const size_t elem_count)
{
    if(data == NULL || target == NULL || elem_size <= 0 || elem_count <= 0) return -1;
    else {
        for(ssize_t i = 0; i < elem_count; ++i){
            if(memcmp(target, (data + (elem_size * i)), elem_size) == 0) return i;
        }
        return -1;
    }
}

bool array_serialize(const void *src_data, const char *dst_file, const size_t elem_size, const size_t elem_count)
{
    if(src_data == NULL || dst_file == NULL || elem_size <= 0 || elem_count <= 0) return false;
    else if(strstr(dst_file, "\n") == NULL ) {
        FILE * file_pointer = fopen(dst_file, "w");
        if(file_pointer == NULL) return false;
        else {
            size_t check = fwrite(src_data, elem_size, elem_count, file_pointer);
            fclose(file_pointer);
            if(check != elem_count) return false;
            else return true;
        }
    } else return false;
}  // should i check for all escape characters?

bool array_deserialize(const char *src_file, void *dst_data, const size_t elem_size, const size_t elem_count)
{
    if(src_file == NULL || dst_data == NULL || elem_size <= 0 || elem_count <= 0) return false;
    else if(strstr(src_file, "\n") == NULL) {
        FILE * file_pointer = fopen(src_file, "r");
        if(file_pointer == NULL) return false;
        else {
            size_t check = fread(dst_data, elem_size, elem_count, file_pointer);
            fclose(file_pointer);
            if(check != elem_count) return false;
            else return true;
        }
    } else return false;
}

