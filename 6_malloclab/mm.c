/*
 * mm.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */

#include "mm.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"

team_t team = {
    /* Team name */
    "LiTeam",
    /* First member's full name */
    "Li Li",
    /* First member's email address */
    "lil147@pitt.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE 4             /* Word and header/footer size (bytes) */
#define DSIZE 8             /* Double word size (bytes) */
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* the number of the free list */
#define LIST 20

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val) | GET_TAG(p))
#define PUT_NOTAG(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_TAG(p) (GET(p) & 0x2)
#define SET_RATAG(p) (GET(p) |= 0x2)
#define REMOVE_RATAG(p) (GET(p) &= ~0x2)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-WSIZE)                         // hdrp
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)  // ftrp

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

/* Given the address of the free block's pred and succ */
#define PRED_PTR(ptr) ((char *)(ptr))
#define SUCC_PTR(ptr) ((char *)(ptr) + WSIZE)

/* Given the address of the free block's pred and succ */
#define PRED(ptr) (*(char **)(ptr))
#define SUCC(ptr) (*(char **)(SUCC_PTR(ptr)))

/* Set pred or succ pointer for free blocks */
#define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

/* $end mallocmacros */

/* Global variables */
static char *heap_listp = 0; /* Pointer to first block */
void *free_lists[LIST];      /* store the segregate free list */

/* Function prototypes for internal helper routines */
static void mm_check();
static void *extend_heap(size_t words);
static void insert_node(void *ptr, size_t size);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void printblock(void *bp);
static void checkheap(int verbose);
static void checkblock(void *bp);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    int index;

    /* initialize the segregated free list NULL */
    for (index = 0; index < LIST; index++) free_lists[index] = NULL;

    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1) return -1;
    PUT(heap_listp, 0);                            /* Alignment padding */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */
    heap_listp += (2 * WSIZE);                     // endinit

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) return -1;
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    if (heap_listp == 0) {
        mm_init();
    }

    /* Ignore spurious requests */
    if (size == 0) return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)      // line:vm:mm:sizeadjust1
        asize = 2 * DSIZE;  // line:vm:mm:sizeadjust2
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) /
                         DSIZE);  // line:vm:mm:sizeadjust3

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {  // line:vm:mm:findfitcall
        place(bp, asize);                  // line:vm:mm:findfitplace
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);  // line:vm:mm:growheap1
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;   // line:vm:mm:growheap2
    place(bp, asize);  // line:vm:mm:growheap3
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    /* $end mmfree */
    if (ptr == 0) return;

    /* $begin mmfree */
    size_t size = GET_SIZE(HDRP(ptr));
    /* $end mmfree */
    if (heap_listp == 0) {
        mm_init();
    }
    /* $begin mmfree */

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) { /* Case 1 */
        return bp;
    }

    else if (prev_alloc && !next_alloc) { /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) { /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else { /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    size_t oldsize;
    void *newptr;

    /* If size == 0 then this is just free, and we return NULL. */
    if (size == 0) {
        mm_free(ptr);
        return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if (ptr == NULL) {
        return mm_malloc(size);
    }

    newptr = mm_malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if (!newptr) {
        return 0;
    }

    /* Copy the old data. */
    oldsize = GET_SIZE(HDRP(ptr));
    if (size < oldsize) oldsize = size;
    memcpy(newptr, ptr, oldsize);

    /* Free the old block. */
    mm_free(ptr);

    return newptr;
}

/*
 * mm_check - Minimal check of the heap for consistency
 */
static void mm_check() {
    char *bp = heap_listp;
    printf("Heap (%p):\n", heap_listp);

    if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp)))
        printf("Bad prologue header\n");
    checkblock(heap_listp);

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        printblock(bp);
        checkblock(bp);
    }

    printblock(bp);
    if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp))))
        printf("Bad epilogue header\n");
}

/*
 * extend_heap - Extend heap with free block and return its block pointer
 */
static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    /* if NO ENOUGH SPACE */
    if ((long)(bp = mem_sbrk(size)) == -1) return NULL;

    /* Initialize free block header/footer and the epilogue header */

    /* Free block header */
    PUT(HDRP(bp), PACK(size, 0));

    /* Free block footer */
    PUT(FTRP(bp), PACK(size, 0));

    /* New epilogue header */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    /* Insert extended block to the free list */
    insert_node(bp, size);

    /* Coalesce if the previous block was free */
    return coalesce(bp);  // line:vm:mm:returnblock
}

/*
 * place - Place block of asize bytes at start of free block bp
 *         and split if remainder would be at least minimum block size
 */
static void place(void *bp, size_t asize) {
    size_t csize = GET_SIZE(HDRP(bp));

    if ((csize - asize) >= (2 * DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/*
 * insert_node - Insert current node to the segregated free list
 */
static void insert_node(void *ptr, size_t size) {
    int index;  /* the index of the free list */
    void *pred; /* points to the previous free block node */
    void *succ; /* points to the next free block node */

    /* find the proper index in the free list */
    /* size class: free_list[0]={1}, free_list[1]={2~3}, free_list[2]={4~7}*/
    for (index = 0; index < LIST - 1; index++) {
        if (size > 1) {
            size = size >> 1;
        } else
            break;
    }
    succ = free_lists[index];

    /* traverse to find a position for node inputing */
    while (succ != NULL && size < GET_SIZE(HDRP(succ))) {
        pred = succ;
        succ = PRED(succ);
    }

    /* IF succ is not the end of the list */
    if (succ != NULL) {
        /* ptr is the succ of succ node */
        SET_PTR(PRED_PTR(ptr), succ);
        SET_PTR(SUCC_PTR(succ), ptr);

        /* IF pred is not the root of the list */
        if (pred != NULL) {
            /* insert ptr between the list */
            SET_PTR(PRED_PTR(pred), ptr);
            SET_PTR(SUCC_PTR(ptr), pred);
        }
        /* ELSE pred is the root of the list */
        else {
            /* insert ptr at the begining of the list */
            SET_PTR(SUCC_PTR(ptr), NULL);

            /* update the root of the free list */
            free_lists[index] = ptr;
        }
    } 
    /* ELSE succ is the end of the list */
    else {
        /* the pred of ptr is NULL */
        SET_PTR(PRED_PTR(ptr), NULL);

        /* IF pred is not the root of the list */
        if (pred != NULL) {
            /* insert ptr between the list */
            SET_PTR(SUCC_PTR(ptr), pred);
            SET_PTR(PRED_PTR(pred), ptr);
        }
        /* ELSE the list is empty */
        else {
            /* insert ptr at the begining of the list */
            SET_PTR(SUCC_PTR(ptr), NULL);
            /* update the root of the free list */
            free_lists[index] = ptr;
        }
    }
    return;
}

/*
 * find_fit - Find a fit for a block with asize bytes
 */
static void *find_fit(size_t asize) {
    void *bp;

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            return bp;
        }
    }
    return NULL; /* No fit */
}

static void printblock(void *bp) {
    size_t hsize, halloc, fsize, falloc;

    mm_check();
    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));

    if (hsize == 0) {
        printf("%p: EOL\n", bp);
        return;
    }

    printf("%p: header: [%ld:%c] footer: [%ld:%c]\n", bp, hsize,
           (halloc ? 'a' : 'f'), fsize, (falloc ? 'a' : 'f'));
}

static void checkblock(void *bp) {
    if ((size_t)bp % 8) printf("Error: %p is not doubleword aligned\n", bp);
    if (GET(HDRP(bp)) != GET(FTRP(bp)))
        printf("Error: header does not match footer\n");
}