#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "hash.h"

/*
 * Questions:
 *          1. Can i use structs and lists like this
 *          2. or is something like memcmp more preferable?
 *          3. on no collision, do we assume if the value isnt null then the object contained is the correct object, there is no way to compare built into the function prototype
 */

typedef struct ListStruct List;

struct ListStruct
{
    void * data;
    int key;
    List * next;
};

typedef struct HashTableStruct HashTable;

struct HashTableStruct
{
    void ** hashArray;
};

int getKey(List * node);

void setKey(List * node, int key);

void * getData(List * node);

void setData(List * node, void * data);

List * getNext(List * node);

void setNext(List * node, List * nextNode);

List * initList();

List * makeNode(void * data, int key);

void freeChain(List * chain);

void**  getHashArray(HashTable * htable);

void setHashArray(HashTable * htable, void** ptr);


// These are stubs.  That is, you need to implement these functions.

int InsertFailCollision(	void * hashtable, int elementSize, int elementCountMax,
                            int key, void * element, int (*HashFunc)(int key));


void * SearchNoCollision(void * hashtable, int key, int elementSize, int (*HashFunc)(int key));

int DivMethod(int key);

int MultMethod(int key);

void * AllocateChainTable(int elementCountMax);

void FreeChainTable(void * hashtable);

int InsertChain(	void * hashtable, int elementSize, int elementCountMax,
                            int key, void * element, int (*HashFunc)(int key));

void * SearchChain(void * hashtable, int key, int elementSize, int (*HashFunc)(int key));


// End of prototypes
//
// list struct helpers

int getKey(List * node)
{
    return node->key;
}
void setKey(List * node, int key)
{
    node->key = key;
}
void * getData(List * node)
{
    return node->data;
}
void setData(List * node, void * data)
{
    node->data = data;
}
List * getNext(List * node)
{
    return node->next;
}
void setNext(List * node, List * nextNode)
{
    node->next = nextNode;
}
List * initList()
{
    return NULL;
}
List *makeNode(void * data, int key)
{
    List * node = (List*) calloc(1,sizeof(List));
    if( node == NULL ) return NULL;
    setKey(node, key);
    setData(node, data);
    setNext(node, NULL);
    return node;
}


void freeChain(List * chain)
{
    if( chain == NULL ) return;
    List * next = getNext(chain);
    free(chain);
    freeChain(next);
}

//HashTable struct helpers
//

void** getHashArray(HashTable * htable)
{
    return htable->hashArray;
}
void setHashArray(HashTable * htable, void** ptr)
{
    htable->hashArray = ptr;
}

// Function impelementations
//
int InsertFailCollision(	void * hashtable, int elementSize, int elementCountMax,
                            int key, void * element, int (*HashFunc)(int key))
{
    int index = HashFunc(key);
    void * place = hashtable + index*elementSize;
    for( int i = 0 ; i < elementSize; ++i )
    {
        if( ((char*)place)[i] != 0 ) return -1;
    }
    memcpy(place, element, elementSize);
    return 0;
 }

void * SearchNoCollision(void * hashtable, int key, int elementSize, int (*HashFunc)(int key))
{
    // ask for critique of assignment 1
    // matches are confirmed by main, not this search function, is that a problem?
    // solution, call get key function in lib on casted address  and compare
    // similar to what i did with list, but structure of list is known to function
    // do a byte by byte character parsing of void * address + elementSize, and then 
    // try to match key. This doesnt require knowledge of underlying struct, but is 
    // computationally instensive and complex
    int index = HashFunc(key);
    return hashtable+index*elementSize;
}


int DivMethod(int key)
{
	return key % TABLESIZE;
}

int MultMethod(int key)
{
    double knuthNum = (sqrt(5)-1)/2;
    double dKey = (double)key;
    return (int)( TABLESIZE  * ( dKey * knuthNum - (int)( dKey*knuthNum ) ) );
}


void * AllocateChainTable(int elementCountMax)
{
    HashTable * htable = (HashTable*)calloc(1,sizeof(HashTable));
    void**  hashArray =(void**) calloc(elementCountMax,sizeof(void**));
    setHashArray(htable, hashArray);
    return (void*)htable;
}

void FreeChainTable(void * hashtable)
{
    if( hashtable == NULL ) return;
    int tableSize = TABLESIZE;
    void** array = getHashArray( (HashTable*) hashtable);
    if( array != NULL )
    {
        for(int i = 0 ; i < tableSize ; ++i)
        {
             freeChain( array[i] );
        }
    }
    free(array);
    free(hashtable);
    return;
}

int InsertChain(	void * hashtable, int elementSize, int elementCountMax,
                            int key, void * element, int (*HashFunc)(int key))
{
    void** array = getHashArray( (HashTable*)hashtable );

    List * newNode = makeNode(element, key);
    
    int index = HashFunc(key);

    List* head = array[index];

    if( newNode == NULL ) return -1;

    if( head == NULL ) array[index] = newNode;
    else
    {
        List* next=NULL;
        while(1)
        {
            next = getNext(head);
            if(next == NULL) 
            {
                setNext(head,newNode);
                break;
            }
            else
            {
                (head = next);
            }
        }
        
    }
    //puts("Successful insertion");

    return 0;
}

void * SearchChain(void * hashtable, int key, int elementSize, int (*HashFunc)(int key))
{
    void** array = getHashArray( (HashTable*) hashtable);

    List* next = (List*)array[HashFunc(key)];

    while( next != NULL )
    {
        int keyX = getKey(next);
        if(key == keyX)
        {
            //puts("found match");
            return getData(next);
        }
        else
        {
            next = getNext(next);
        }
    }
    //puts("Did not find result");

    return NULL;
}


