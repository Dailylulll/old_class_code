#include "../include/bitmap.h"
#include <stdio.h>  // get rid of this

// data is an array of uint8_t and needs to be allocated in bitmap_create
//      and used in the remaining bitmap functions. You will use data for any bit operations and bit logic
// bit_count the number of requested bits, set in bitmap_create from n_bits
// byte_count the total number of bytes the data contains, set in bitmap_create
//

const uint8_t bit_array[8] = {1, 2, 4, 8, 16, 32, 64, 128};


bitmap_t *bitmap_create(size_t n_bits)
{
    if(n_bits <= 0) return NULL;
    bitmap_t * map = malloc(sizeof(bitmap_t));
    map->bit_count = n_bits;
    size_t byte_count = n_bits / 8;
    if(n_bits % 8) byte_count++;
    map->byte_count = byte_count;
    map->data = calloc(sizeof(uint8_t), byte_count);
    if(map->data == NULL) {
        free(map);
    }
    return map;
}

bool bitmap_set(bitmap_t *const bitmap, const size_t bit)
{
    if(bitmap == NULL || bit < 0 || bitmap->data == NULL || bitmap->bit_count < bit) return false;
    //uint8_t bit_array[8] = {1, 2, 4, 8, 16, 32, 64, 128};
    int byte = bit / 8;
    int bit_spot = bit % 8;
    bitmap->data[byte] |= bit_array[bit_spot];
    return true;
}  // This depends on endianess, the bit array might have to be backwards

bool bitmap_reset(bitmap_t *const bitmap, const size_t bit)
{
    if(bitmap == NULL || bit < 0 || bitmap->data == NULL || bitmap->bit_count < bit) return false;
    //uint8_t bit_array[8] = {1, 2, 4, 8, 16, 32, 64, 128};
    int byte = bit / 8;
    int bit_spot = bit % 8;
    bitmap->data[byte] ^= bit_array[bit_spot];
    return true;
}  // the bit might have to be tested first, because if 0, xor will set the bit, !or might be better

bool bitmap_test(const bitmap_t *const bitmap, const size_t bit)
{
    if(bitmap == NULL || bit < 0 || bitmap->data == NULL || bitmap->bit_count < bit) return false;
    //uint8_t bit_array[8] = {1, 2, 4, 8, 16, 32, 64, 128};
    int byte = bit / 8;
    int bit_spot = bit % 8;
    return ((bitmap->data[byte] &= bit_array[bit_spot]) == bit_array[bit_spot]);
}

size_t bitmap_ffs(const bitmap_t *const bitmap)
{
    if(bitmap == NULL || bitmap->data == NULL || bitmap->bit_count <= 0 || bitmap->byte_count < 0) return SIZE_MAX;
    //uint8_t bit_array[8] = {1, 2, 4, 8, 16, 32, 64, 128};
    for(size_t i = 0; i < bitmap->byte_count; ++i) {
        for(size_t j = 0; j < 8 && i * 8 + j < bitmap->bit_count; ++j) {
            if((bitmap->data[i] & bit_array[j]) == bit_array[j]) {
                return i * 8 + j;
            }
        }
    }
    return SIZE_MAX;
}  // check set bit, or create bitmap

size_t bitmap_ffz(const bitmap_t *const bitmap)
{
    if(bitmap == NULL || bitmap->data == NULL || bitmap->bit_count <= 0 || bitmap->byte_count < 0) return SIZE_MAX;
    //uint8_t bit_array[8] = {1, 2, 4, 8, 16, 32, 64, 128};
    for(size_t i = 0; i < bitmap->byte_count; ++i) {
        for(size_t j = 0; j < 8 && i * 8 + j < bitmap->bit_count; ++j) {
            if( (~bitmap->data[i] & bit_array[j]) == bit_array[j]) {  // use and to cancel out the bit
                return i * 8 + j;
            }
        }
    }
    return SIZE_MAX;
}  // youre returning the end of the array because youre not checking total bits

bool bitmap_destroy(bitmap_t *bitmap)
{
    if( bitmap == NULL ) return  false;  // is this true
    if( bitmap->data != NULL ) {
        free(bitmap->data);
    }
    free(bitmap);
    return true;
}
