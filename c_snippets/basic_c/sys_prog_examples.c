#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../include/sys_prog.h"

// LOOK INTO OPEN, READ, WRITE, CLOSE, FSTAT/STAT, LSEEK
// GOOGLE FOR ENDIANESS HELP

bool bulk_read(const char *input_filename, void *dst, const size_t offset, const size_t dst_size)
{
    if(input_filename == NULL || dst == NULL || offset < 0 || dst_size <= 0) return false;
    if(access(input_filename, R_OK)) return false;
    struct stat info;
    struct stat * info_ptr = &info;
    if(stat(input_filename, info_ptr) == -1) return false;
    if(info.st_size <= offset) return false;
    int file_descriptor = open(input_filename, O_RDONLY);
    lseek(file_descriptor, offset, SEEK_SET);
    read(file_descriptor, dst, dst_size);
    close(file_descriptor);
    return true;
}  // you need to close files too yo

bool bulk_write(const void *src, const char *output_filename, const size_t offset, const size_t src_size)
{
    if(src == NULL || output_filename == NULL || offset < 0 || src_size <= 0) return false;
    int file_descriptor = open(output_filename, O_WRONLY, O_CREAT);
    if(file_descriptor < 0) return false;
    lseek(file_descriptor, offset, SEEK_SET);
    write(file_descriptor, src, src_size);
    close(file_descriptor);
    return true;
}


bool file_stat(const char *query_filename, struct stat *metadata)
{
    if(query_filename == NULL || metadata == NULL) return false;
    if(stat(query_filename, metadata) == -1) return false;
    return true;
}

bool endianess_converter(uint32_t *src_data, uint32_t *dst_data, const size_t src_count)
{
    if(src_data == NULL || dst_data == NULL || src_count <= 0) return false;
    for(size_t i = 0; i < src_count; ++i) {
        if(&src_data[i] == NULL || &dst_data[i] == NULL) return false;
        else {
            uint8_t * back = (uint8_t*)&src_data[i];
            uint8_t * forward = (uint8_t*)&dst_data[i];
            for(size_t k = 0; k < sizeof(uint32_t); ++k) {
                forward[k] = back[sizeof(uint32_t) - k -1];
            }
        }
    }
    return true;
}  // you need to switch bytes, get a zero, 

