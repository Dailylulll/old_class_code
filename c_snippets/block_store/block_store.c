#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "bitmap.h"
#include "block_store.h"
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <error.h>

#define UNUSED(x) (void)(x)
//#define DEBUG perror
#define DEBUG UNUSED
//#define DBRANCH if(true)
//#define DBRANCH if(false)

/**
    This is the block_store struct
    I put the bitmap object into the struct
    but the bitmap data points to a block of memory in the data block
    so although parts of the bitmap struct are outside the data, 
    the actual bitmap is at BLOCK_START_BLOCK
*/
struct block_store {
    uint8_t * data;
    bitmap_t * bitmap;
};

/**
    Used bit_map overlay constructor to make the bitmap dependent on the datablock
*/
block_store_t *block_store_create() 
{
    block_store_t *bs = calloc(1, sizeof(block_store_t));
    if(bs){
        bs->data = calloc(BLOCK_STORE_NUM_BLOCKS, BLOCK_SIZE_BYTES);
        if(bs->data) {
            bs->bitmap = bitmap_overlay(BITMAP_SIZE_BITS, ((void*)bs->data + BITMAP_START_BLOCK * BLOCK_SIZE_BYTES));
            if(bs->bitmap) {
                for(int i = 0; i < BLOCK_STORE_NUM_BLOCKS / 32 / 8; ++i) {
                    bitmap_set(bs->bitmap, BITMAP_START_BLOCK + i);
                }
                return bs;
            }
            free(bs->data);
        }
        free(bs);
    }
    return NULL;  
}

/**
    Deconstructor
*/
void block_store_destroy(block_store_t *const bs)
{
    if(bs) {
        free(bs->data);
        bitmap_destroy(bs->bitmap);
        free(bs);
    }
    return;
}
/**
    Finds and returns a free block_id or SIZE_MAX on error
*/
size_t block_store_allocate(block_store_t *const bs)
{
    if(bs) {
        size_t open_spot = bitmap_ffz(bs->bitmap);
        if(open_spot != SIZE_MAX) {
            bitmap_flip(bs->bitmap, open_spot);
            return open_spot;
        }
    }
    return SIZE_MAX;
}

/**
    Requests specific datablock for storage
*/
bool block_store_request(block_store_t *const bs, const size_t block_id)
{
    if(bs && block_id < BLOCK_STORE_NUM_BLOCKS ) {
        if(!bitmap_test(bs->bitmap, block_id)) {
            bitmap_set(bs->bitmap, block_id);
            return true;
        }
    }
    return false;
}

/**
    Releases specific datablock
    I put in a protection to prevent the bitmap itself from being
*/
void block_store_release(block_store_t *const bs, const size_t block_id)
{
    if(bs && block_id <BLOCK_STORE_NUM_BLOCKS) {  
        if(block_id < BITMAP_START_BLOCK || block_id >= BITMAP_START_BLOCK + BITMAP_SIZE_BITS ) {
            bitmap_reset(bs->bitmap, block_id);
        }        
    }
}

size_t block_store_get_used_blocks(const block_store_t *const bs)
{
    if(bs) {
        return bitmap_total_set(bs->bitmap);
    }
    return SIZE_MAX;
}

size_t block_store_get_free_blocks(const block_store_t *const bs)
{
    if(bs) {
        return BITMAP_SIZE_BITS - bitmap_total_set(bs->bitmap);
    }
    return SIZE_MAX;
}

size_t block_store_get_total_blocks()
{
    return BLOCK_STORE_NUM_BLOCKS;
}

// block store read uses memcpy
size_t block_store_read(const block_store_t *const bs, const size_t block_id, void *buffer)
{
    if(bs && buffer && block_id < BLOCK_STORE_NUM_BLOCKS) {
        memcpy(buffer, (void*)(bs->data + block_id * BLOCK_SIZE_BYTES), BLOCK_SIZE_BYTES);
        return BLOCK_SIZE_BYTES;
    }
    return 0;
}

// block store write uses memcpy
size_t block_store_write(block_store_t *const bs, const size_t block_id, const void *buffer)
{  
    if(bs && buffer && block_id < BLOCK_STORE_NUM_BLOCKS) {
        memcpy((void*)(bs->data + block_id * BLOCK_SIZE_BYTES), buffer, BLOCK_SIZE_BYTES);
        return BLOCK_SIZE_BYTES;
    }
    return 0;
}

// block store deserialize, uses debug macro to check for errors
block_store_t *block_store_deserialize(const char *const filename)  
{
    if(filename && !access(filename, R_OK)) {
        struct stat info;
        if(stat(filename, &info) != -1) {
            int fd;
            if((fd = open(filename, O_RDONLY)) != -1) {
                size_t blocks_needed = info.st_size / BLOCK_SIZE_BYTES;
                if(blocks_needed > BLOCK_STORE_NUM_BLOCKS) return NULL;
                block_store_t * bs = block_store_create();
                if(!bs) return NULL;
                if(read(fd, (void*)bs->data, BLOCK_SIZE_BYTES * BLOCK_STORE_NUM_BLOCKS) == -1) {
                    DEBUG("ERROR: At read");
                    close(fd);
                    return  NULL;
                }
                close(fd);
                return bs;
            }
            DEBUG("ERROR: At read");
        }
        DEBUG("ERROR: At read");
    }
    DEBUG("ERROR: At read");
    return NULL;
}

// serialize function
// function checks perror built into DEBUG macro
size_t block_store_serialize(const block_store_t *const bs, const char *const filename)
{
    if(bs && filename) {
        int fd = open(filename, O_RDWR | O_CREAT, S_IRWXU);
        if(fd == -1) {
            DEBUG("ERROR EXIT: Error opening file");
            return 0;
        }
        ssize_t count = write(fd, (void*)bs->data, BLOCK_STORE_NUM_BLOCKS * BLOCK_SIZE_BYTES);
        close(fd);
        if(count == -1) {
            DEBUG("ERROR EXIT: Error on write");
            return 0;
        } else {
            return count;
        }
    }
    DEBUG("ERROR EXIT: Invalid params");
    return 0;
}