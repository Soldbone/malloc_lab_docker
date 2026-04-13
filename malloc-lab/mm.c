/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "JiSeob Lee",
    /* First member's email address */
    "@",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc)) // header/footer에 저장될 크기 및 할당 정보가 담긴 형태의 바이트 데이터를 반환한다. (크기와 할당 비트를 통합해서 헤더와 풋터에 저장할 수 있는 값을 리턴)

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))              // 인자 p가 참조하는 워드를 읽어서 리턴한다.
#define PUT(p, val) (*(unsigned int *)(p) = (val)) // 인자 p가 가리키는 워드에 val을 저장한다.

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7) // header 또는 footer에 있는 하위 3비트를 제외한 size 정보를 반환한다.
#define GET_ALLOC(p) (GET(p) & 0x1) // header 또는 footer에 있는 할당 여부 비트를 반환한다.

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)                      // payload 시작 주소에서 WSIZE(4bytes) 전으로 이동하여 header 영역에 접근한다.
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) // 현재 블록의 footer에 접근한다.(payload 시작 주소에서 header의 크기를 읽어서 현재 주소에서 더한다. 그 위치는 다음 블록의 payload이므로, 현재 블록의 footer에 접근하기 위해서 DSIZE(8바이트)를 빼준다.)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) // 다음 블록의 payload 시작 주소를 반환한다.(현재 블록 헤더에서 크기를 가져와서 현재 payload 시작 주소에 더해준다.)
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) // 이전 블록의 payload 시작 주소를 반환한다.(현재 payload 시작 위치에서 이전 블록의 footer를 통해 블록 size를 파악하여 빼준다.)

static char *heap_listp = 0;

static void *extend_heap(size_t words);
static void *coalesce(void *p);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    heap_listp += (2 * WSIZE);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    // NOTE: sbrk()의 반환값은 이전 brk 위치임에 유의
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* 이전 에필로그 위치에 새로운 블록 정보 덮어쓰기 */
    PUT(FTRP(bp), PACK(size, 0));         /* 현재 블록 크기만큼 이동하여 푸터 생성 */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* 새로운 에필로그 생성 */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

static void *coalesce(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    size_t prev_alloc = GET_ALLOC(bp - DSIZE);
    size_t next_alloc = GET_ALLOC(bp + size - WSIZE);
    /* GET_SIZE(HDRP(PREV_BLKP(bp))) == 이전 블록 헤더 == 이전 블록 푸터 == GET_SIZE(bp - DSIZE) */
    /* GET_SIZE(FTRP(NEXT_BLKP(bp))) == 다음 블록 푸터 == 다음 블록 헤더 == GET_SIZE(bp + size - WSIZE) */

    if (prev_alloc && next_alloc)
    {
        return bp;
    }

    else if (prev_alloc && !next_alloc)
    {
        size += GET_SIZE(bp + size - WSIZE);

        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc)
    {
        size_t prev_size = GET_SIZE(bp - DSIZE);
        size += prev_size;

        bp = bp - prev_size;
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else
    {
        size_t prev_size = GET_SIZE(bp - DSIZE);
        size_t next_size = GET_SIZE(bp + size - WSIZE);
        size += prev_size + next_size;

        bp = bp - prev_size;
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    return bp;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit*/
    char *bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;

    // int newsize = ALIGN(size + SIZE_T_SIZE);
    // void *p = mem_sbrk(newsize);
    // if (p == (void *)-1)
    //     return NULL;
    // else
    // {
    //     *(size_t *)p = size;
    //     return (void *)((char *)p + SIZE_T_SIZE);
    // }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}