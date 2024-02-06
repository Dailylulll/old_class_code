#include "../include/structures.h"
#include <stdio.h>
#include <string.h>

int compare_structs(sample_t* a, sample_t* b)
{
    if(a == NULL || b == NULL) return 0;
    if(a->a == b->a && a->b == b->b && a->c == b->c) return 1;
    else return 0;
}

void print_alignments()
{
	printf("Alignment of int is %zu bytes\n",__alignof__(int));
	printf("Alignment of double is %zu bytes\n",__alignof__(double));
	printf("Alignment of float is %zu bytes\n",__alignof__(float));
	printf("Alignment of char is %zu bytes\n",__alignof__(char));
	printf("Alignment of long long is %zu bytes\n",__alignof__(long long));
	printf("Alignment of short is %zu bytes\n",__alignof__(short));
    printf("Alignment apple is %zu bytes\n", __alignof__(apple_t));
    printf("Alignment orange is %zu bytes\n",__alignof__(orange_t));
	printf("Alignment of structs are %zu bytes\n",__alignof__(fruit_t));
}

int sort_fruit(const fruit_t* a,int* apples,int* oranges, const size_t size)
{
    if(a == NULL || apples == NULL || oranges == NULL || size <= 0) return-1;
    if(*apples != 0 || *oranges != 0) return -1;
    
    for(size_t i = 0; i < size; ++i) {
        if(&a[i] == NULL) return -1;
        if(a[i].type == 1) *apples = *apples + 1;
        else *oranges = *oranges + 1;
    }
    return size;
}

int initialize_array(fruit_t* a, int apples, int oranges)
{  
    if(a == NULL || apples < 0 || oranges < 0 || apples + oranges < 1) return -1;
    for(int i = 0; i < apples; ++i) {
        initialize_apple((apple_t*)&a[i]);
    }
    for(int i = 0 + apples; i < oranges + apples; ++i) {
        initialize_orange((orange_t*)&a[i]);
    }
    
    return 0;
}

int initialize_orange(orange_t* a)
{
    memset(a, 0, sizeof(fruit_t));
    a->type = 2;
	return 0;
}

int initialize_apple(apple_t* a)
{
    memset(a, 0, sizeof(fruit_t));
    a->type = 1;
	return 0;
}
