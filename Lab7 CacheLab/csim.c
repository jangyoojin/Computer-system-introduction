//name: Jang Yujin, loginID: jangyj2020

#include "cachelab.h"
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>

void accessCache(int set, unsigned int tag);

typedef struct Line
{
    int val;
    unsigned int tag;
    int used;
} Line;

Line **cache;          //cache 동적할당할 포인터
FILE *result;          //cache hit/miss 결과 저장하는 텍스트 파일
int s, E, b;           // commend line에서 입력받는 값
int hitcount = 0;      //hit 개수
int misscount = 0;     //miss 개수
int evictioncount = 0; //eviction 개수
int last_use = 0;      //마지막에 사용된 cache 표시하는 변수

int main(int argc, char *const *argv)
{
    //commend line에서 입력된 옵션 값 저장하는 변수
    int opt;
    int verbose = 0;
    FILE *t;
    //hit/miss 결과 기록하는 txt 파일
    result = fopen("result.txt", "w");

    char instruction;
    unsigned int address;
    int blockbyte;

    int set_index = 1;
    int S = 2;
    int i, j;

    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            break;
        case 'v':
            verbose = 1;
            break;
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            t = fopen(optarg, "r");
            break;
        }
    }

    //S(number of set) 구하기
    for (i = 1; i < s; i++)
    {
        set_index += S;
        S *= 2;
    }

    //E*S개의 line을 가지는 cache 공간 할당
    cache = (Line **)malloc(S * sizeof(Line *));
    for (i = 0; i < S; i++)
        cache[i] = (Line *)malloc(E * sizeof(Line));

    //cache의 valid, tag, used 값 0으로 초기화
    for (i = 0; i < S; i++)
    {
        for (j = 0; j < E; j++)
        {
            cache[i][j].val = 0;
            cache[i][j].tag = 0;
            cache[i][j].used = 0;
        }
    }

    //trace에서 한 줄씩 읽어와서 hit/miss 판단하기
    while (fscanf(t, " %c %x,%d", &instruction, &address, &blockbyte) != EOF)
    {
        int set = (address >> b) & set_index;  //set bit 구하기
        unsigned int tag = address >> (b + s); //tag 비트 구하기

        switch (instruction)
        {
        case 'L':
            fprintf(result, "%c, %11x,%d ", instruction, address, blockbyte);
            accessCache(set, tag);
            fprintf(result, "\n");
            break;
        case 'M':
            fprintf(result, "%c, %11x,%d ", instruction, address, blockbyte);
            accessCache(set, tag);
            accessCache(set, tag);
            fprintf(result, "\n");
            break;
        case 'S':
            fprintf(result, "%c, %11x,%d ", instruction, address, blockbyte);
            accessCache(set, tag);
            fprintf(result, "\n");
            break;
        }
    }

    //verbose option이면 result.txt 내용 모두 출력
    if (verbose)
    {
    }
    printSummary(hitcount, misscount, evictioncount);

    fclose(t);
    fclose(result);

    for (i = 0; i < S; i++)
        free(cache[i]);
    free(cache);

    return 0;
}

void accessCache(int set, unsigned int tag)
{
    int i, evic = 1;
    int lru, evicLine;

    //hit인지 판단
    for (i = 0; i < E; i++)
    {
        if (cache[set][i].val == 1 && cache[set][i].tag == tag)
        {
            last_use++;
            cache[set][i].used = last_use;
            hitcount++;
            fprintf(result, "hit ");
            return;
        }
    }

    //eviction이 발생하는지 확인
    for (i = 0; i < E; i++)
    {
        if (cache[set][i].val == 0)
        {
            evic = 0;
            break;
        }
    }
    //evicton할 line 찾기 = 가장 최근에 쓰이지 않은 line 찾기
    if (evic)
    {
        lru = cache[set][0].used;
        evicLine = 0;
        for (i = 1; i < E; i++)
        {
            if (cache[set][i].used < lru)
            {
                lru = cache[set][i].used;
                evicLine = i;
            }
        }
        cache[set][evicLine].tag = tag;
        last_use++;
        cache[set][evicLine].used = last_use;
        evictioncount++;
        misscount++;
        fprintf(result, "miss eviction ");
        return;
    }
    else
    {
        for (i = 0; i < E; i++)
        {
            if (cache[set][i].val == 0)
            {
                cache[set][i].val = 1;
                cache[set][i].tag = tag;
                last_use++;
                cache[set][i].used = last_use;
                misscount++;
                fprintf(result, "miss ");
                return;
            }
        }
    }
}