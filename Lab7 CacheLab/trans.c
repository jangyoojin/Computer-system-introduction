/* Name: Jang yoojin
   LoginID: jangyj2020
*/

/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int val1, val2, val3, val4, val5, val6, val7, val8;
    int i, j, i0, j0;

    if (M == 32 && N == 32)
    {
        for (i = 0; i < N; i += 8)
        {
            for (j = 0; j < M; j += 8)
            {
                for (i0 = 0; i0 < 8; i0++)
                {
                    val1 = A[i + i0][j];
                    val2 = A[i + i0][j + 1];
                    val3 = A[i + i0][j + 2];
                    val4 = A[i + i0][j + 3];
                    val5 = A[i + i0][j + 4];
                    val6 = A[i + i0][j + 5];
                    val7 = A[i + i0][j + 6];
                    val8 = A[i + i0][j + 7];
                    B[j][i + i0] = val1;
                    B[j + 1][i + i0] = val2;
                    B[j + 2][i + i0] = val3;
                    B[j + 3][i + i0] = val4;
                    B[j + 4][i + i0] = val5;
                    B[j + 5][i + i0] = val6;
                    B[j + 6][i + i0] = val7;
                    B[j + 7][i + i0] = val8;
                }
            }
        }
    }
    else if (M == 64 && N == 64)
    {
        for (i = 0; i < N; i += 8)
        {
            for (j = 0; j < M; j += 8)
            {
                for (i0 = 0; i0 < 4; i0++) //4줄마다 같은 set 사용하므로 4X4 block이 최소 miss 발생
                {
                    //A 원소 한 개 load 해서 conflict miss 안 나는 7개까지 모두 읽어오기
                    val1 = A[j + i0][i];
                    val2 = A[j + i0][i + 1];
                    val3 = A[j + i0][i + 2];
                    val4 = A[j + i0][i + 3];
                    val5 = A[j + i0][i + 4];
                    val6 = A[j + i0][i + 5];
                    val7 = A[j + i0][i + 6];
                    val8 = A[j + i0][i + 7];
                    //B의 4x8 matrix 구역에 저장하기, 세로가 4줄을 넘어가면 conflict miss 많이 발생
                    B[i][j + i0] = val1;
                    B[i + 1][j + i0] = val2;
                    B[i + 2][j + i0] = val3;
                    B[i + 3][j + i0] = val4;
                    B[i][j + i0 + 4] = val5;
                    B[i + 1][j + i0 + 4] = val6;
                    B[i + 2][j + i0 + 4] = val7;
                    B[i + 3][j + i0 + 4] = val8;
                }
                //B 4x8 matrix에 저장했던 값들 원위치로 이동
                for (i0 = 4; i0 < 8; i0++)
                {
                    val1 = B[i + i0 - 4][j + 4];
                    val2 = B[i + i0 - 4][j + 5];
                    val3 = B[i + i0 - 4][j + 6];
                    val4 = B[i + i0 - 4][j + 7];

                    //값을 임시로 저장했던 B 위치에는 올바른 값 채우기
                    B[i + i0 - 4][j + 4] = A[j + 4][i + i0 - 4];
                    B[i + i0 - 4][j + 5] = A[j + 5][i + i0 - 4];
                    B[i + i0 - 4][j + 6] = A[j + 6][i + i0 - 4];
                    B[i + i0 - 4][j + 7] = A[j + 7][i + i0 - 4];

                    B[i + i0][j] = val1;
                    B[i + i0][j + 1] = val2;
                    B[i + i0][j + 2] = val3;
                    B[i + i0][j + 3] = val4;
                }
                //B의 8x8 matrix block에서 마지막 4x4 matrix block 부분 채우기
                for (i0 = 0; i0 < 4; i0++)
                {
                    B[i + 4 + i0][j + 4] = A[j + 4][i + 4 + i0];
                    B[i + 4 + i0][j + 5] = A[j + 5][i + 4 + i0];
                    B[i + 4 + i0][j + 6] = A[j + 6][i + 4 + i0];
                    B[i + 4 + i0][j + 7] = A[j + 7][i + 4 + i0];
                }
            }
        }
    }
    else if (M == 61 && N == 67)
    {
        for (i = 0; i < N; i += 16)
        {
            for (j = 0; j < M; j += 16)
            {
                for (i0 = 0; (i0 < 16) && (i0 + i < N); i0++)
                {
                    for (j0 = 0; (j0 < 16) && (j0 + j < M); j0++)
                    {
                        B[j + j0][i + i0] = A[i + i0][j + j0];
                    }
                }
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; ++j)
        {
            if (A[i][j] != B[j][i])
            {
                return 0;
            }
        }
    }
    return 1;
}
