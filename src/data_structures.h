#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include "memory.h"

// Generic single linked list, double linked list, binary heap, double linked list with freelist, pool







#define NEW_SLIST(List, Arena, Type)    {                   \
        Type *Sentinel = (Type *) PUSH_STRUCT(Arena, Type); \
        (List)->Sentinel = Sentinel;                        \
        (List)->Sentinel->Next = Sentinel;    }             \

#define NEW_SLIST_FREE(List, Arena, Type)    {              \
        Type *Sentinel = (Type *) PUSH_STRUCT(Arena, Type); \
        (List)->Sentinel = Sentinel;                        \
        (List)->Free = (Type *) PUSH_STRUCT(Arena, Type);   \
        (List)->Free->Next = (List)->Sentinel;              \
        (List)->Sentinel->Next = Sentinel;    }             \

#define FREE_SLIST(List, Type) {                    \
        Type *First = (List)->Sentinel->Next;       \
        (List)->Free->Next = First;                 \
        (List)->Sentinel->Next = (List)->Sentinel;  \
    }                                               \

#define SLIST_ADDFIRST(List, Node)              \
    (Node)->Next = (List)->Sentinel->Next;      \
    (List)->Sentinel->Next = (Node);            \


#define SLIST_REMOVEFIRST(List)                         \
    List->Sentinel->Next = List->Sentinel->Next->Next;  \


#define NEW_DLIST(List, Arena, Type) {                      \
        Type *Sentinel = (Type *) PUSH_STRUCT(Arena, Type); \
        (List)->Sentinel = Sentinel;                        \
        (List)->Sentinel->Next = Sentinel;                  \
        (List)->Sentinel->Prev = Sentinel;}                 \

#define NEW_DLIST_FREE(List, Arena, Type)                   \
    (List)->Sentinel = (Type *) PUSH_STRUCT(Arena, Type);   \
    (List)->Sentinel->Next = (List)->Sentinel;              \
    (List)->Sentinel->Prev = (List)->Sentinel;              \
    (List)->Free = (Type *) PUSH_STRUCT(Arena, Type);       \
    (List)->Free->Next = (List)->Sentinel;                  \
    (List)->Free->Prev = (List)->Sentinel;                  \

#define DLIST_ADDLAST(List, Node)               \
    Node->Next = List->Sentinel;                \
    Node->Prev = List->Sentinel->Prev;          \
    Node->Next->Prev = Node;                    \
    Node->Prev->Next = Node;                    \

#define DLIST_ADDFIRST(List, Node)              \
    Node->Prev = List->Sentinel;                \
    Node->Next = List->Sentinel->Next;          \
    Node->Next->Prev = Node;                    \
    Node->Prev->Next = Node;                    \


#define FREE_DLIST(List) {                              \
        List->Sentinel->Prev->Next = List->Free->Next;  \
        List->Free->Next = List->Sentinel->Next;        \
        List->Sentinel->Next = List->Sentinel;          \
        List->Sentinel->Prev = List->Sentinel;          \
    }                                                   \




#define DLIST_INSERTBEFORE(List, Node, Before)  \
    Node->Next = Before;                        \
    Node->Prev = Before->Prev;                  \
    Node->Next->Prev = Node;                    \
    Node->Prev->Next = Node;                    \

#define DLIST_INSERTAFTER(List, Node, After)    \
    Node->Prev = After;                         \
    Node->Next = After->Next;                   \
    Node->Next->Prev = Node;                    \
    Node->Prev->Next = Node;                    \

#define DLIST_REMOVE(List, Node)                \
    Node->Next->Prev = Node->Prev;              \
    Node->Prev->Next = Node->Next;              \

#define DLIST_REMOVELAST(List)                                          \
    (List->Sentinel->Prev)->Prev->Next = (List->Sentinel->Prev)->Next;  \
    (List->Sentinel->Prev)->Next->Prev = (List->Sentinel->Prev)->Prev;  \

#define DLIST_REMOVEFIRST(List)                                         \
    (List->Sentinel->Next)->Next->Prev = (List->Sentinel->Next)->Prev;  \
    (List->Sentinel->Next)->Prev->Next = (List->Sentinel->Next)->Next;  \

#define DLIST_PEEKFIRST(List)                   \
    (List->Sentinel->Next)                      \

#define DLIST_PEEKLAST(List)                    \
    (List->Sentinel->Prev)                      \

#define DLIST_EMPTY(List)                       \
    (List->Sentinel->Next = List->Sentinel)    \
    (List->Sentinel->Prev = List->Sentinel)    \


#define SLIST_EMPTY(List)                       \
    (List)->Sentinel->Next = (List)->Sentinel  \

#define SLIST_PEEKFIRST(List) DLIST_PEEKFIRST(List) \

#define NEW_HEAP(Heap, BaseSize, Type, Arena)                   \
    Heap->size = BaseSize;                                      \
    Heap->count = 0;                                            \
    Heap->data = (Type *) PUSH_ARRAY(Arena, BaseSize, Type);    \

#define CMP(a, b) ((a->f) <= (b->f))

#define HEAP_PUSH(Heap, Value)                              \
    {                                                       \
        u32 index, parent ;                                 \
        for (index = Heap->count++; index; index=parent) {  \
            parent = (index-1) >> 1;                        \
            if CMP((Heap->data[parent]), (Value)) break;    \
            Heap->data[index] = Heap->data[parent];         \
        }                                                   \
        Heap->data[index] = Value;                          \
    };                                                      \

#define HEAP_PEEKFIRST(h) (*(h)->data)

// since this is a macro it doenst return the popped item, you can peek at it
// with  HEAP_PEEKFIRST, or write a typed heapPop procedure
#define HEAP_POP(Heap, Type)                                            \
    {                                                                   \
        u32 index, swap, other;                                         \
        Type temp = Heap->data[--Heap->count];                           \
        for(index = 0; 1; index = swap) {                               \
            swap = (index << 1) + 1;                                    \
            if (swap >= Heap->count) break;                             \
            other = swap + 1;                                           \
            if ((other < Heap->count) && CMP(Heap->data[other], Heap->data[swap])) swap = other; \
            if CMP(temp, Heap->data[swap]) break;                       \
            Heap->data[index] = Heap->data[swap];                       \
        }                                                               \
        Heap->data[index] = temp;                                       \
    };                                                                  \

//adapted from https://gist.github.com/martinkunev/1365481
//TODO this code is a bit meh, for some reason the swap index can become huge!
// like 300.000 or something, this overwrites memory out of bounds.
// for now a simple fix is just have a bit larger scratch space, so the memory is atleast not in another Arena.

#define HEAP_UPDATE_AT(Heap, Type, StartIndex)                          \
    {                                                                   \
        u32 count = Heap->count;                                        \
        u32 item=StartIndex;                                            \
        u32 index, swap, other;                                         \
        Type temp;                                                      \
        while(1) {                                                      \
            temp = Heap->data[item];                                    \
            for (index = item; 1; index = swap) {                       \
                swap = (index << 1) + 1;                                \
                if (swap >= count) break;                               \
                other = swap + 1;                                       \
                if ((other < count) && CMP(Heap->data[other], Heap->data[swap])) swap = other; \
                if CMP(temp, Heap->data[swap]) break;                   \
                Heap->data[index] = Heap->data[swap];                   \
                ASSERT(swap <= Heap->size);                              \
            }                                                           \
            if (index != item) Heap->data[index] = temp;                \
            if (!item) break;                                           \
            --item;                                                     \
        }                                                               \
    }                                                                   \

#define HEAP_UPDATE_ITEM(Heap, Type, Value)     \
    {                                           \
        u32 Index = 0;                          \
        int IndexFound = 0;                     \
        for (u32 i = 0; i < Heap->size; i++) {  \
            if (Heap->data[i] == Value) {       \
                IndexFound = 1;                 \
                break;                          \
            }                                   \
            Index++;                            \
        }                                       \
        if (IndexFound) {                       \
            HEAP_UPDATE_AT(Heap, Type, Index)   \
                }                               \
    }                                           \




#endif
