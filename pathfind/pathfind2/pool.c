#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef __MACH__
#include <sys/time.h>
//clock_gettime is not implemented on OSX
int clock_gettime(struct timespec* t) {
    struct timeval now;
    int rv = gettimeofday(&now, NULL);
    if (rv) return rv;
    t->tv_sec  = now.tv_sec;
    t->tv_nsec = now.tv_usec * 1000;
    return 0;
}
#endif

//             FreeStart
//             |
// +--+--+--+--+--+--+--+
// |U |U |U |U |F |F |F |
// +--+--+--+--+--+--+--+
//             |
//
// Used items are in the front, free ones are at the back
// When you want a new Item, you take the Free one at FreeStart and do a FreeStart++
// When you return an item (and make it free) you swap it with the Used one at FreeStart-1 and do a FreeStart--
//
// Initially all items are Free so FreeStart = 0



typedef struct entity {
    int isFree;
    int x;
} entity;

typedef struct entity_pool {
    entity * data;
    int Size;
    int FreeStart;
} entity_pool;

void swap(int index1, int index2, entity_pool *P) {
    entity temp = P->data[index1];
    P->data[index1] =  P->data[index2];
    P->data[index2] = temp;
}

entity* get_item(entity_pool *P) {
    assert(P->FreeStart < P->Size);
    entity *Result = &P->data[P->FreeStart];
    assert(Result->isFree);
    Result->isFree = 0;
    P->FreeStart++;
    return Result;
}

void return_item_at_index( entity_pool *P, int Index) {
    assert(Index > -1);
    assert(Index < P->Size);
    assert(!P->data[Index].isFree && "oops, dont forget if you want to free many items from a pool, either free all at once or loop backwards");
    assert(P->FreeStart > 0);

    entity *ToFree = &P->data[Index];
    ToFree->isFree = 1;
    P->FreeStart--;
    // the endresult seems correct, but I believe something is not good here.
    // P->B and the Index are the same, only though when walking the loop backwards, which actualluy makes sense
    //printf("%d, %d\n",P->B, Index);
    if (P->FreeStart != Index) swap(Index, P->FreeStart, P);
}


int main() {
    entity_pool Pool;
    Pool.Size = 5000;
    Pool.FreeStart = 0;
    entity_pool *P = &Pool;
    Pool.data = malloc(Pool.Size * sizeof(entity));

    //struct timespec before;
    //clock_gettime(&before);

    for (int i = 0; i <  Pool.Size; i++) {
        Pool.data[i].isFree = 1;
        Pool.data[i].x = i * 10;
    }
    for (int i = 0; i <  Pool.Size; i++) {
        entity * E = get_item(P);
    }

    /* printf("\n"); */
    /* for (int i = P->FreeStart-1; i >= 0; i--) { */
    /*    return_item_at_index(P, i); */
    /* } */
    /* for (int i = 0; i < 3; i++) { */
    /*     return_item_at_index(P, i); */
    /* } */

    struct timespec before;
    clock_gettime(&before);

    for (int i = 0; i < Pool.FreeStart; i++) {
        Pool.data[i].x+=1;
    }

    struct timespec after;
    clock_gettime(&after);




    for (int i = 0; i <  Pool.Size; i++) {
        //    printf("item at %d is %d and is free = %d \n", i, Pool.data[i].x, Pool.data[i].isFree);
    }
    printf("time  %f ms.\n",  ((after.tv_sec - before.tv_sec)*1000) +  (after.tv_nsec - before.tv_nsec)/1000000.0f  );

}
