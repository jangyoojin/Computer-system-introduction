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
#define LISTLIMIT 20 //segregated list 시작 주소 관리하는 list 크기
#define CHUNKSIZE (1<<12) //heap을 한번 늘리는 용량, 4kB

//둘 중 더 큰(작은) 값을 고르는 함수
#define MAX(x,y) ((x)>(y)? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y)) 

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

// p에서 시작해서 1 word만큼 읽거나 쓰기
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val)) 

// block의 size, allocated를 합치는 함수
#define PACK(size,alloc) ((size)| (alloc))

//pointer 설정
#define SET_PTR(p, ptr) (*(unsigned int*)(p) = (unsigned int)(ptr))

//block 관련 함수
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

//block의 header, footer로 이동하는 함수
#define HDRP(ptr) ((char *)(ptr) - WSIZE)
#define FTRP(ptr) ((char *)(ptr) + GET_SIZE(HDRP(ptr)) - DSIZE)

//(주소 기준) 다음 또는 이전 block으로 이동하는 함수
#define NEXT_BLKP(ptr) ((char *)(ptr) + GET_SIZE((char *)(ptr) - WSIZE))
#define PREV_BLKP(ptr) ((char *)(ptr) - GET_SIZE((char *)(ptr) - DSIZE))

//(free list) block 내 prev, next pointer로 이동하는 함수
#define PREV_PTR(ptr) ((char *)(ptr))
#define NEXT_PTR(ptr) ((char *)(ptr) + WSIZE)

//(free list에서) 이전, 다음 free blokc으로 이동하는 함수
#define PREV(ptr) (*(char **)(ptr))
#define NEXT(ptr) (*(char**)(NEXT_PTR(ptr)))

static char *heap_listp; // heap의 시작 주소
void *segregated_free_listp[LISTLIMIT]; // segregated list 시작 주소 저장하는 list

//사용자 정의 함수
static void *extend_heap(size_t size);
static void *coalesce(void * ptr);
static void *place(void *ptr, size_t asize);
static void insert_node(void *ptr, size_t size);
static void delete_node(void *ptr);
static void heap_checker();




/* 
 * mm_init - initialize the malloc package.
 * 문제가 있으면 -1을 리턴, 아니면 0을 리턴
 */
int mm_init(void)
{
    int list;

    //segregated_free_listp 초기화
    for(list = 0; list < LISTLIMIT; list++){
        segregated_free_listp[list] = NULL;
    }

    if((long)(heap_listp = mem_sbrk(4*WSIZE))==-1)
        return -1;
    
    PUT(heap_listp, 0); //padding
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE,1)); // prologue header
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE,1)); //prologue footer: block 연결 시 가장자리 조건을 없애기 위한 트릭
    PUT(heap_listp + (3*WSIZE), PACK(0,1)); //epilogue header

    printf("mm_init: extend_heap");
    // heap을 CHUNKSIZE만큼 늘려 사용할 수 있는 용량 확보
    if(extend_heap(CHUNKSIZE/WSIZE)==NULL) 
        return -1;
    printf("Finish init");
    heap_checker();
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    printf("mm_malloc");
    size_t asize;
    size_t extendsize;
    int list = 0;//segregated free listp index
    size_t searchsize;
    char *ptr;

    if(size == 0) return NULL; //할당 받을 크기가 0이므로 할당하지 않고 NULL 리턴
    
    if(size <= DSIZE){ //size가 two words size보다 작으면 
        asize = 2*DSIZE; //header, footer를 포함한 크기
    }
    else {
        asize = DSIZE *((size + (DSIZE) + (DSIZE -1))/DSIZE); //header, footer 크기 포함해 최적화한 크기
    }

    searchsize = asize; //first fit을 수행하기 위한 size 저장
    //size에 맞는 segregated free list를 찾아서 해당 list에서 first fit을 수행한다.
    while(list < LISTLIMIT){
        if(list == (LISTLIMIT - 1) || ((searchsize <= 1) && (segregated_free_listp[list] != NULL))){ //마지막 segregated free list의 index에 도달했거나 asize에 맞는 segregated free list를 찾은 경우
            ptr = segregated_free_listp[list];
            while((ptr != NULL) && ((asize > GET_SIZE(HDRP(ptr))))){ //segregated free list에 크기가 오름차순으로 정렬된 free block 중 자신보다 작은 block를 만날 때까지 list를 따라 앞으로 이동
                ptr = PREV(ptr);
            }
            if(ptr != NULL) break;
        }
        searchsize >>= 1;
        list++;
    }

    if(ptr == NULL){ //asize에 맞는 free block이 없는 상태
        extendsize = MAX(asize, CHUNKSIZE); //heap size 늘려주기
        if((ptr = extend_heap(extendsize/WSIZE)) == NULL)
            return NULL;
    }

    ptr = place(ptr, asize);
    heap_checker();
    return ptr;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    printf("mm_free ");
    size_t size = GET_SIZE(HDRP(ptr)); // ptr의 header에서 size를 읽어오기
    

    PUT(HDRP(ptr), PACK(size, 0)); // ptr의 헤더에 allocated -> free로 변경
    PUT(FTRP(ptr), PACK(size, 0)); // ptr의 footer에 allocated -> free로 변경

    insert_node(ptr, size); //이 수행으로 생긴 free block를 segregated free list에 삽입
    coalesce(ptr); //앞 뒤 block과 비교해서 합치기

    heap_checker();
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 * realloc size가 ptr의 size보다 큰 경우, 뒤에 블럭이 free이면 크기에 맞는 free block를 찾지 말고 뒤 block와 이어서 realloc을 해준다. 아니라면, realloc size만큼으로 해당 block을 줄인다.
 */
void *mm_realloc(void *ptr, size_t size)
{
    void* newptr = ptr;
    size_t newsize = size;
    int remainder; //남는 free block 크기
    int extendsize;

    if(size == 0)
        return NULL;
    //realloc할 size 구하기
    if(size <= DSIZE){
        newsize = 2 * DSIZE;    
    }
    else{
        newsize = DSIZE *((size + (DSIZE) + (DSIZE -1))/DSIZE); //header, footer 크기 포함해 최적화한 크기
    }

    if(newsize > GET_SIZE(HDRP(ptr))){ //realloc size가 ptr size보다 큰 경우
        //ptr의 다음 block이 free block이거나 epilogue block인지 확인
        if(!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) || !GET_SIZE(HDRP(NEXT_BLKP(ptr)))){
            remainder = GET_SIZE(HDRP(ptr)) + GET_SIZE(HDRP(NEXT_BLKP(ptr))) - newsize;
            if(remainder < 0){
                extendsize = MAX(-remainder, CHUNKSIZE);
                if(extend_heap(extendsize/WSIZE) == NULL)
                    return NULL;
                remainder += extendsize;
            }
            delete_node(NEXT_BLKP(ptr)); //합친 free block을 segregated free list에서 제거
            PUT(HDRP(ptr), PACK(newsize + remainder, 1));
            PUT(FTRP(ptr), PACK(newsize + remainder, 1));
        } 
    }
    else { //일반적인 경우
        newptr = mm_malloc(newsize - DSIZE);
        memcpy(newptr, ptr, MIN(size, newsize));
        mm_free(ptr);
    }
    heap_checker();
    return newptr;
}



//words*WSIZE값이 8의 배수가 되도록 올림해 새 free block을 만드는 함수
static void *extend_heap(size_t words){
    char *ptr;
    size_t size;

    size = (words%2)? (words+1)*WSIZE : words*WSIZE; // 짝수 개의 word만큼 늘릴 size 설정
    
    //size만큼 heap 사이즈 늘리고 새로 만든 free block의 header, footer 만들기
    if((long)(ptr = mem_sbrk(size))==-1) return NULL;

    PUT(HDRP(ptr), PACK(size, 0));//ptr의 1 word 앞에 size, free를 나타내는 header 설정
    PUT(FTRP(ptr), PACK(size, 0)); 
    PUT(HDRP(NEXT_BLKP(ptr)), PACK(0, 1)); // epilogue header는 heap의 마지막으로 위치 조정
    insert_node(ptr, size); //새롭게 만들어진 size 크기의 free block을 segregated list와 연결

    return coalesce(ptr);
}

//free block을 크기에 맞는 segregated list에 연결해주는 함수
static void insert_node(void *ptr, size_t size){
    int list = 0;
    void *search_ptr = ptr;
    void *insert_ptr = NULL;

    if(size <= 2 * DSIZE){
        return;
    }

    //size에 맞는 segregated list index 찾기
    while((list < LISTLIMIT - 1) && (size > 1)){
        size >>= 1;
        list++;
    }

    //segregated_free_list[list]는 2^(list)이상 2^(list+1)미만인 size를 가진 free_block list의 마지막 block을 가리킨다.
    //따라서 마지막 block부터 앞으로 탐색하면서 자신보다 작거나 같은 크기의 block을 찾는다.
    //segregated_free_list의 free list들을 크기가 큰 block부터 내림차순으로 정렬하며 insert
    search_ptr = segregated_free_listp[list];
    while((search_ptr != NULL) && (size > GET_SIZE(HDRP(search_ptr)))) {
        insert_ptr = search_ptr;
        search_ptr = PREV(search_ptr);
    }

    if(search_ptr != NULL) {
        if(insert_ptr != NULL){ //segregated list 중간에 삽입하는 경우
            SET_PTR(PREV_PTR(ptr), search_ptr);
            SET_PTR(NEXT_PTR(search_ptr), ptr);
            SET_PTR(NEXT_PTR(ptr), insert_ptr);
            SET_PTR(PREV_PTR(insert_ptr), ptr);
        }
        else { //segregated list의 마지막에 삽입하는 경우
            SET_PTR(PREV_PTR(ptr), search_ptr);
            SET_PTR(NEXT_PTR(search_ptr), ptr);
            SET_PTR(NEXT_PTR(ptr), NULL);
            segregated_free_listp[list] = ptr;
        }
    }
    else {
        if(insert_ptr != NULL) { //segregated list의 첫번째 node로 삽입하는 경우
            SET_PTR(PREV_PTR(ptr), NULL);
            SET_PTR(NEXT_PTR(ptr), insert_ptr);
            SET_PTR(PREV_PTR(insert_ptr), ptr);
        }
        else { //segregated list에 아무것도 없을 때 node 삽입하는 경우
            SET_PTR(PREV_PTR(ptr), NULL);
            SET_PTR(NEXT_PTR(ptr), NULL);
            segregated_free_listp[list] = ptr;
        }
    }

    return;
}

//segregated_free_listp의 list에서 free blokc 삭제하는 함수
static void delete_node(void *ptr) {
    int list = 0;
    size_t size = GET_SIZE(HDRP(ptr));

    //ptr이 속한 segregated list 찾기
    while((list < LISTLIMIT - 1) && (size > 1)){
        size >>= 1;
        list++;
    }

    if(PREV(ptr) != NULL){
        if(NEXT(ptr) != NULL) { //segregated list 중간에 있는 node 삭제
            SET_PTR(NEXT_PTR(PREV(ptr)), NEXT(ptr));
            SET_PTR(PREV_PTR(NEXT(ptr)), PREV(ptr));
        }
        else { //segregated list 마지막에 있는 node 삭제
            SET_PTR(NEXT_PTR(PREV(ptr)), NULL);
            segregated_free_listp[list] = PREV(ptr);
        }
    }
    else {
        if(NEXT(ptr) != NULL) { //segregated list의 첫번째 node 삭제
            SET_PTR(PREV_PTR(NEXT(ptr)), NULL);
        }
        else { //segregated list에 node가 1개 남은 경우 node 삭제
            segregated_free_listp[list] = NULL;
        }
    }

    return;
}

//연속된 free block을 합쳐주는 함수
static void* coalesce(void* ptr){
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(ptr))); //이전 블럭의 allocated 여부 확인
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr))); //다음 블럭의 allocated 여부 확인
    size_t size = GET_SIZE(HDRP(ptr)); //현재 block의 사이즈

    if(prev_alloc && next_alloc){
        return ptr;
    }
    else if(prev_alloc && !next_alloc){ //앞 블럭은 allocated, 뒤 블럭은 free 된 상태
        //기존에 저장된 두 free block를 삭제
        delete_node(NEXT_BLKP(ptr));
        delete_node(ptr);
        size += GET_SIZE(HDRP(NEXT_BLKP(ptr))); // 뒤 블럭의 헤더에서 사이즈를 읽어와서 지금 블럭 사이즈에 더하기
        PUT(HDRP(ptr),PACK(size,0)); // 헤더의 size update
        PUT(FTRP(ptr), PACK(size,0)); // footer 새로 설정
    }
    else if(!prev_alloc && next_alloc){
        //기존에 저장된 두 free block를 삭제
        delete_node(PREV_BLKP(ptr));
        delete_node(ptr);
        size += GET_SIZE(HDRP(PREV_BLKP(ptr))); //size에 앞 블럭 사이즈를 더함
        PUT(FTRP(ptr), PACK(size, 0)); // 지금 블럭의 footer에서 size와 allocated 내용을 update
        PUT(HDRP(PREV_BLKP(ptr)),PACK(size,0)); // 앞 블럭의 header에서 size 내용을 update
        ptr = PREV_BLKP(ptr); //ptr는 앞 블럭의 payload 시작 위치를 가리킴
    }
    else{// 앞 뒤 블럭 모두 free 인 상태
        delete_node(PREV_BLKP(ptr));
        delete_node(ptr);
        delete_node(NEXT_BLKP(ptr));
        size += GET_SIZE(HDRP(PREV_BLKP(ptr))) + GET_SIZE(HDRP(NEXT_BLKP(ptr))); //앞 블럭의 header와 뒷 블럭의 footer에서 size를 읽어와 현재 size에 더함   
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size,0)); // 앞 블럭의 header 내용을 update
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(size,0)); //뒷 블럭의 footer 내용을 update
        ptr = PREV_BLKP(ptr); //ptr는 앞 블럭의 payload 시작을 가리킴
    }

    // 합친 새로운 크기의 free block을 segregated list에 추가
    insert_node(ptr, size);


    return ptr;
}

// bp에 asize만큼 allocate 하기
static void *place(void * ptr, size_t asize){
    size_t csize = GET_SIZE(HDRP(ptr)); //header에서 size 읽어오기
    size_t remainder = csize - asize;

    //segregated list에 있는 ptr node를 삭제
    delete_node(ptr);

    if(remainder < 0){
        printf("ptr size is smaller than asize\n");
        return NULL;
    }

    if((remainder) >= 2 * DSIZE){// 현재 block size인 csize에서 넣으려는 asize의 차가 4*DSIZE만큼 나면 남은 공간은 분리해주기
        //asize 넣기
        PUT(HDRP(ptr), PACK(asize, 1));
        PUT(FTRP(ptr), PACK(asize, 1));
        //남은 공간 분리하기
        ptr = NEXT_BLKP(ptr);
        
        //분리한 bp에 header, footer 설정
        PUT(HDRP(ptr), PACK(remainder, 0));
        PUT(FTRP(ptr), PACK(remainder, 0));
        insert_node(ptr, remainder);
    }
    else { // 아니라면 asize만 넣고 종료
        PUT(HDRP(ptr), PACK(asize, 1));
        PUT(FTRP(ptr), PACK(asize, 1));
    }

    return ptr;
}


static void heap_checker(){
    void* ptr;
    void *next;
    int list = 0;
    for(list = 0; list < LISTLIMIT; list++){
        ptr = segregated_free_listp[list];
        for (next = ptr; next != NULL; next = PREV(next)) {
            if (GET_ALLOC(HDRP(next))) {
              printf("Consistency error: block %p in free list but marked allocated! list: %d", next, list);
              return 1;
            }
        }
    }
}