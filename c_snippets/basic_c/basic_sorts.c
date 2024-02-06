/**
 * @file sort.c
 * @author Dillon Jackson dzjxb2
 * @brief Assignment 1 cs3050
 * @date 2022-09-26
 * 
 */

#include <cs3050.h>
#include <stdio.h>

void bubblesort(void *array, 
				size_t nitems, 
				size_t size, 
				int (*CompareFunc)(const void *, const void*))
{
	for (int i=0;i<nitems;i++)
	{
		for (int j=0;j<nitems-1;j++)
		{
			void * item1 = array + j*size;
			void * item2 = array + (j+1)*size;
			if (CompareFunc(item1,item2)>0)
			{
				Swap(item1,item2,size);
			}
		}
	}
}


/**
 * My implementation of insertionsort to header specification
 */
void insertionsort(void *array, 
				size_t nitems, 
				size_t size, 
				int (*CompareFunc)(const void *, const void*))
{
    int i = 1;
    int j = 0;
    for(  i = 1 ; i < nitems ; ++i)
    {
        for(  j = i ; j > 0; --j)
        {
            void * item1 = array + j*size;
            void * item2 = array + (j-1)*size;
            if ( CompareFunc( item1, item2 ) < 0 )
            {
                Swap(item1, item2, size);
            }
            else break;
            
        }
    }
}


/*
 * Helper functions for heapify. These funcs find the parents and children of subtrees based on i
 * They return NULL if out of range of array
 * fun pointer arithmetic is used for indexing
 */
void * getP(void * array, size_t nitems, size_t i, size_t size)
{
    return array + (i)*size-size;
}
void * getR(void * array, size_t nitems, size_t i, size_t size)
{
    if( i*2+1 > nitems) return NULL;
    else return array + (2*(i)*size);
}
void * getL(void * array, size_t nitems, size_t i, size_t size)
{
    if( i*2 > nitems) return NULL;
    else return array + 2*(i)*size-size;
}

/**
 * Helper function for buildheap and heapsort
 * Recursive heapify function
 */
void heapify( void * array,
                size_t nitems,
                size_t size,
                size_t i,
                int (*CompareFunc) (const void *, const void* ))
{
    void * r = getR(array, nitems, i, size);
    void * l = getL(array, nitems, i, size);
    void * p = getP(array, nitems, i, size);
    void * largest = p;
    size_t ii = 0;
    if( r != NULL && CompareFunc( largest, r ) < 0 )
    {
        largest = r;
        ii = i*2+1;
    }
    if( l != NULL && CompareFunc( largest, l ) < 0 ) 
    {
        largest = l;
        ii = i*2;
    }
    if( p != largest )
    {
        Swap( p, largest, size );
        heapify( array, nitems, size, ii, CompareFunc );
    }
}


/**
 * Helper function for heapsort
 * Builds a heap structure
 */
void buildHeap(void * array,
                size_t  nitems,
                size_t  size,
                int (*CompareFunc) (const void *, const void *))
{
    for( size_t i = nitems/2; i>=1 ; --i )
    {
        heapify(array, nitems, size, i, CompareFunc);
    }
}

/*
 * heapsort function built to header spec
 */
void heapsort(void *array, 
				size_t nitems, 
				size_t size, 
				int (*CompareFunc)(const void *, const void*))
{
    buildHeap( array, nitems, size, CompareFunc );
    
    for( size_t i = nitems; i >=2 ; )
    {
        Swap( array, array + (i)*size-size, size );
        --i;
        heapify(array, i, size, 1, CompareFunc);
    }
    
}
