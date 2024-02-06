#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "mst.h"

/**
 * @brief This constant is used to set "infinite" weight for prim's algorithm
 *          FLOAT_MAX caused weird bugs, related to the float datatype
 * 
 */
#define WEIGHTLIMIT 1000

/**
 * @brief This struct wraps the vertex array with information needed to implement prims algorithm
 * 
 */
typedef struct _VertexWrapper
{
    bool inTree; //boolean if already in MST
    int vIndex; //index in vertex/WR array
    int pIndex; //index of parent vertex, for prims
    int hIndex; //index of vertex in heap, needed for decrease key func
    float weight; //weight for prims
    Vertex * vPtr; //pointer to vertex info

} WR;

/**
 * @brief These are the getter and setter funcs for JimR's vertex implmentation
 * 
 */

int getFrom(Edge *a)
{
    return a->from;
}
int getTo(Edge *a)
{
    return a->to;
}
double getWeight(Edge *a)
{
    return a->weight;
}

Edge * getEdge(Adjacency * a)
{
    return a->pEdge;
}
Adjacency * getNext(Adjacency * a)
{
    return a->next;
}

Adjacency * getAdj(Vertex * v)
{
    return v->list;
}

/**
 * @brief Wraps vertices with necessary info for prims. 
 *          Acts as a connection between heap info and vertex array
 *          Array is one based instead of zero to make indexing consistent
 * 
 * @param vertices 
 * @param countVertices 
 * @return WR* array of wrapped vertices
 */

WR * initWrapperArray(Vertex * vertices, int countVertices)
{
    WR * array = (WR*)calloc(countVertices, sizeof(WR));
    array--;
    for(int i = 1 ; i <= countVertices ; ++i)
    {
        array[i].pIndex = -1;
        array[i].weight = WEIGHTLIMIT;
        array[i].vIndex = i;
        array[i].vPtr = &vertices[i-1];
        array[i].inTree = false;
    }
    return array;
}
void freeWArray(WR * a)
{
    free( ++a );
}

/**
 * @brief Heap of pointers to Wrapped vertex pointers
 *          One based instead of zero to make heap arithmetic easier
 * 
 * @param wrapper 
 * @param countVertices 
 * @return WR** heap array
 */
WR ** initHeap(WR * wrapper, int countVertices)
{
    WR ** heap = (WR**)calloc(countVertices, sizeof(WR**));
    heap--;
    for(int i = 1 ; i <= countVertices ; ++i)
    {
        heap[i] = &wrapper[i];
        wrapper[i].hIndex = i;
    }
    return heap;
}
void freeHeap(WR ** a)
{
    free( ++a );
}

/**
 * @brief Helper functions for heap functionality
 *          Heap is based around WR wrapper
 *          Index's are used instead of pointers for ease of writing
 * 
 */

int getParent(int index)
{
    int p;
    p = index/2;
    if( p < 1 ) return -1;
    else return p;
}
int getLeft(int index, int heapSize)
{
    int l;
    l = index*2;
    if( l > heapSize ) return -1;
    else return l;
}
int getRight(int index, int heapSize)
{
    int r;
    r = index*2 + 1;
    if( r > heapSize ) return -1;
    else return r;
}

int compareWeight(WR * a, WR * b)
{
    return a->weight - b->weight;
}
void swap(WR ** heap, int index, int greaterIndex)
{
    WR * hold = heap[index];
    heap[index] = heap[greaterIndex];
    heap[index]->hIndex = index;
    heap[greaterIndex] = hold;
    heap[greaterIndex]->hIndex = greaterIndex;
}

void heapify(WR ** heap, int index, int heapSize)
{
    int greatestIndex = index;
    int holdIndex = getLeft(index, heapSize);
    if( holdIndex != -1 )
    {
        if( compareWeight( heap[greatestIndex], heap[holdIndex]) > 0 )
        {
            greatestIndex = holdIndex;
        }
    }
    holdIndex = getRight(index, heapSize);
    if( holdIndex != -1 )
    {
        if( compareWeight( heap[greatestIndex], heap[holdIndex]) > 0 )
        {
            greatestIndex = holdIndex;
        }
    }
    if( greatestIndex != index )
    {
        swap(heap, index, greatestIndex);
        heapify(heap, greatestIndex, heapSize);
    }
}

/**
 * @brief Priority Q function, extract min
 * 
 * @param heap 
 * @param heapSize 
 * @return WR* 
 */
WR * extractMin(WR ** heap, int heapSize)
{
    WR * vertex = heap[1];
    heap[1] = heap[heapSize];
    heap[heapSize] = vertex;
    heapSize--;
    heapify(heap, 1, heapSize);
    vertex->inTree=true;
    return vertex;
}

/**
 * @brief recursive helper func for decrease key
 * 
 * @param heap 
 * @param hIndex 
 */
void climbHeap( WR ** heap, int hIndex)
{
    int pIndex = getParent(hIndex);
    if( pIndex != -1 )
    {
        if( compareWeight( heap[hIndex], heap[pIndex] ) < 0 )
        {
            swap(heap, pIndex, hIndex);
            climbHeap(heap, pIndex);
        }
    }
}

/**
 * @brief Decrease key function. 
 *          Needed for prims algorithm
 * 
 * @param heap 
 * @param wArray 
 * @param vIndex 
 * @param pIndex 
 * @param newWeight 
 */
void decreaseKey(WR ** heap, WR * wArray, int vIndex, int pIndex, float newWeight)
{
    if( wArray[vIndex].inTree == true ) return;
    if( wArray[vIndex].weight > newWeight  )
    {
        wArray[vIndex].weight = newWeight;
        wArray[vIndex].pIndex = pIndex;
        climbHeap(heap, wArray[vIndex].hIndex);
    }
}

/**
 * @brief Printer functions for display purposes
 * 
 */
void printTree(WR ** heap, WR * wArray, int countVertices, float totalWeight)
{
    printf("\nMINIMUM SPANNING TREE\n\nTotal vertices: %d\nTotal weight: %f\n",countVertices, totalWeight);
    for( int i = countVertices-1 ; i > 0 ; --i )
    {
        printf("%d -> %d, weight = %.2f\n", heap[i]->pIndex,heap[i]->vIndex,heap[i]->weight);
    }
}

void printHeap(WR ** heap, WR * wArray, int countVertices)
{
    printf("\nMINIMUM SPANNING TREE\n\nTotal vertices: %d\n",countVertices);
    for( int i = 1 ; i <= countVertices ; ++i )
    {
        printf("%d -> %d, weight = %.2f\n", heap[i]->pIndex,heap[i]->vIndex,heap[i]->weight);
    }
}

/**
 * @brief Main function for assignment
 *          The tree is stored in reverse order in the heap. 
 *          A empty heap is a full MST
 * 
 * @param vertices 
 * @param startNumber 
 * @param countVertices 
 */
void MST_Prim(Vertex vertices[], int startNumber, int countVertices)
{
    int heapSize = countVertices;
    WR * wArray = initWrapperArray(vertices, countVertices);
    WR ** heap = initHeap(wArray, countVertices);

    int index = startNumber;
    int childIndex;
    float weight = 0;
    float newWeight = 0;
    PAdjacency adj;
    Edge * edge;
    WR * min;

    decreaseKey(heap, wArray, index, -1, newWeight);
    min = extractMin(heap, heapSize);
    heapSize--;

    while (heapSize != 0) 
    {
        adj = getAdj( wArray[index].vPtr );
        while( adj != NULL && ( edge = getEdge(adj) ) != NULL )
        {
            if( getFrom(edge) == index ) childIndex = getTo(edge);
            else childIndex = getFrom(edge);
            newWeight = getWeight(edge);
            decreaseKey( heap, wArray, childIndex, index, newWeight);
            adj = getNext(adj);
        }
        min = extractMin(heap, heapSize);
        heapSize--;
        weight = weight + min->weight;
        index = min->vIndex;
        
    }
    
    printTree(heap, wArray, countVertices, weight);

    freeHeap(heap);
    freeWArray(wArray);
}
