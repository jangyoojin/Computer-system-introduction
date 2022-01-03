
/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution. //여기 수정 필요
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
//preprocessor macros for encapsulate my pointer arithmetic
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12) //heap을 한번 늘리는 용량, 4kB

//둘 중 더 큰 값을 고르는 함수
#define MAX(x,y) ((x)>(y)? (x) : (y))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

// p에서 시작해서 1 word만큼 읽거나 쓰기
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val)) 

// block의 size, allocated를 합치는 함수
#define PACK(size,alloc) ((size)| (alloc))

//주소 p에 있는 block size, allocated를 불러오는 함수
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

// block의 header, footer의 시작주소를 찾는 함수
#define HDRP(bp) ((char*)(bp)-WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp))-DSIZE)

//bp에서 다음 또는 이전 block 시작 주소를 계산하는 함수
#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE(((char*)(bp)-WSIZE))) // 내 block의 크기만큼 뒤로 더해줌
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE(((char*)(bp) - DSIZE))) // 앞 block의 footer에서 사이즈 읽어와서 현재 주소에 빼줌





#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

static char *heap_listp; // heap의 시작 주소
static char *search_ptr = NULL;//next fit을 시작하는 주소


//연속된 free block을 합쳐주는 함수
static void* coalesce(void* bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); //이전 블럭의 allocated 여부 확인
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); //다음 블럭의 allocated 여부 확인
    size_t size = GET_SIZE(HDRP(bp)); //현재 block의 사이즈

    if(prev_alloc && next_alloc){ // 앞 뒤 블럭이 모두 allocated 된 상태
        search_ptr = bp; // free에 사용한 bp로 search_ptr 업데이트
        return bp;
    }
    else if(prev_alloc && !next_alloc){ //앞 블럭은 allocated, 뒤 블럭은 free 된 상태
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); // 뒤 블럭의 헤더에서 사이즈를 읽어와서 지금 블럭 사이즈에 더하기
        PUT(HDRP(bp),PACK(size,0)); // 헤더의 size update
        PUT(FTRP(bp), PACK(size,0)); // footer 새로 설정
    }
    else if(!prev_alloc && next_alloc){
        size += GET_SIZE(HDRP(PREV_BLKP(bp))); //size에 앞 블럭 사이즈를 더함
        PUT(FTRP(bp), PACK(size, 0)); // 지금 블럭의 footer에서 size와 allocated 내용을 update
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0)); // 앞 블럭의 header에서 size 내용을 update
        bp = PREV_BLKP(bp); //bp는 앞 블럭의 payload 시작 위치를 가리킴
    }
    else{// 앞 뒤 블럭 모두 free 인 상태
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))); //앞 블럭의 header와 뒷 블럭의 footer에서 size를 읽어와 현재 size에 더함
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0)); // 앞 블럭의 header 내용을 update
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0)); //뒷 블럭의 footer 내용을 update
        bp = PREV_BLKP(bp); //bp는 앞 블럭의 payload 시작을 가리킴
    }
    search_ptr = bp; // free에 사용한 bp로 search_ptr 업데이트
    return bp;
}

//words*WSIZE값이 8의 배수가 되도록 올림해 새 free block을 만드는 함수
static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    size = (words%2)? (words+1)*WSIZE : words*WSIZE; // 짝수 개의 word만큼 늘릴 size 설정
    
    //size만큼 heap 사이즈 늘리고 새로 만든 free block의 header, footer 만들기
    if((long)(bp = mem_sbrk(size))==-1) return NULL;
    PUT(HDRP(bp), PACK(size, 0));//bp의 1 word 앞에 size, free를 나타내는 header 설정
    PUT(FTRP(bp), PACK(size, 0)); 
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // epilogue header는 heap의 마지막으로 위치 조정
    
    return coalesce(bp);
}

//next fit을 수행하는 함수
static void* find_fit(size_t asize){
    void* bp;
    for(bp = NEXT_BLKP(search_ptr); GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){//이전 탐색부터 모든 block을 돌면서
        if(!GET_ALLOC(HDRP(bp)) && (asize<=GET_SIZE(HDRP(bp)))){ // free이고 size가 asize보다 큰 block이 있으면 그 블럭의 bp를 리턴
            search_ptr = bp;
            return bp;
        }
    }
    for(bp = NEXT_BLKP(heap_listp); bp <= search_ptr; bp = NEXT_BLKP(bp)){
        if(!GET_ALLOC(HDRP(bp)) && (asize<=GET_SIZE(HDRP(bp)))){ // free이고 size가 asize보다 큰 block이 있으면 그 블럭의 bp를 리턴
            search_ptr = bp;
            return bp;
        }
    }

    return NULL; //없으면 NULL 리턴
}

//
static void place(void * bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp)); //header에서 size 읽어오기

    if((csize - asize) >= (2*DSIZE)){// 현재 block size인 csize에서 넣으려는 asize의 차가 2*DSIZE만큼 나면 남은 공간은 분리해주기
        //asize 넣기
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        //남은 공간을 분리하는 header, footer 설정
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    }
    else { // 아니라면 csize만 넣고 종료
        PUT(HDRP(bp), PACK(csize,1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
    search_ptr = bp;
}


/* 
 * mm_init - initialize the malloc package.
 * 문제가 있으면 -1을 리턴, 아니면 0을 리턴
 */
int mm_init(void)
{
    if((heap_listp = mem_sbrk(4*WSIZE))==(void*)-1)
        return -1;
    PUT(heap_listp, 0); //padding
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE,1)); // prologue header
    PUT(heap_listp+(2*WSIZE), PACK(DSIZE,1)); //prologue footer: block 연결 시 가장자리 조건을 없애기 위한 트릭
    PUT(heap_listp+(3*WSIZE), PACK(0,1)); //epilogue header
    heap_listp += (2*WSIZE);

    // heap을 CHUNKSIZE만큼 늘려 사용할 수 있는 용량 확보
    if(extend_heap(CHUNKSIZE/WSIZE)==NULL) return -1;
    search_ptr = heap_listp;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    if(size == 0) return NULL; //할당 받을 크기가 0이므로 할당하지 않고 NULL 리턴
    
    if(size <= DSIZE){ //size가 two words size보다 작으면 
        asize = 2*DSIZE; //header, footer를 포함한 크기
    }
    else {
        asize = DSIZE *((size + (DSIZE) + (DSIZE -1))/DSIZE); //header, footer 크기 포함해 최적화한 크기
    }

    if((bp = find_fit(asize)) != NULL){ //asize와 fit한 블럭을 찾음
        place(bp, asize);
        search_ptr = bp;
        return bp;
    }

    //위에서 fit한 block를 찾지 못하면
    extendsize = MAX(asize, CHUNKSIZE); // heap 사이즈 늘리기
    //printf("Heap is fulled");
    if((bp = extend_heap(extendsize/WSIZE)) == NULL) 
        return NULL;
    //printf("Success extend_heap");
    place(bp,asize);
    search_ptr = bp;
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp)); // bp의 header에서 size를 읽어오기

    PUT(HDRP(bp), PACK(size, 0)); // bp의 헤더에 allocated -> free로 변경
    PUT(FTRP(bp), PACK(size, 0)); // bp의 footer에 allocated -> free로 변경
    coalesce(bp); //앞 뒤 block과 비교해서 합치기
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size)
{
    void* newbp; // 다시 할당한 결과
    size_t copySize; // 기존 block에서 복사할 크기

    if(size <= 0){ //size가 잘못된 경우
        mm_free(bp);
        return 0;
    }
    if(bp == NULL){ //bp이 NULL인 경우
        return mm_malloc(size);
    }
    //size 만큼을 할당하기
    newbp = mm_malloc(size);
    if(newbp == NULL) return NULL;
    //기존 block에서 불러올 payload 크기
    copySize = GET_SIZE(HDRP(bp));
    if(size < copySize){
        copySize = size;
    }
    memcpy(newbp, bp, copySize);
    mm_free(bp);
    search_bp = newbp; //search_bp update
    return newbp;
}
