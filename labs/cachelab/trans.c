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
	void trans_32(int M, int N,int A[N][M],int B[M][N]);
	void trans_64(int M, int N,int A[N][M],int B[M][N]);
	void trans_61(int M, int N,int A[N][M],int B[M][N]);
	switch(M){
	case 32:trans_32(M, N, A, B); break;
	case 64:trans_64(M, N, A, B); break;
	case 61:trans_61(M, N, A, B); break;
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

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}
void trans_32(int M, int N, int A[N][M], int B[M][N]){
	for(int i=0;i<N;i+=8)
		for(int j=0;j<M;j+=8){
			/* 8*8分块内部 */
			for(int i1=i;i1<i+8;i1++)
			{
				int t0 = A[i1][j];
				int t1 = A[i1][j+1];
				int t2 = A[i1][j+2];
				int t3 = A[i1][j+3];
				int t4 = A[i1][j+4];
				int t5 = A[i1][j+5];
				int t6 = A[i1][j+6];
				int t7 = A[i1][j+7];
				
				B[j][i1] = t0;
				B[j+1][i1] = t1;
				B[j+2][i1] = t2;
				B[j+3][i1] = t3;
				B[j+4][i1] = t4;
				B[j+5][i1] = t5;
				B[j+6][i1] = t6;
				B[j+7][i1] = t7;				
			}
		}
}

void trans_61(int M, int N, int A[N][M], int B[M][N]){
	int block_size = 23;
	for(int i=0;i<N;i+=block_size)
		for(int j=0;j<M;j+=block_size)
			/* block内部 */
		{
			for(int i1=i;i1<i+block_size && i1<N;i1++)
				for(int j1=j;j1<j+block_size && j1<M;j1++)
					B[j1][i1] = A[i1][j1];
		}
}

void trans_64(int M, int N, int A[N][M], int B[M][N]){
	int t0,t1,t2,t3,t4,t5,t6,t7;
	for(int i=0;i<N;i+=8)
		for(int j=0;j<M;j+=8){
			/* 8*8分块内部 */
			for(int i1=i;i1<i+4;i1++){
				t0 = A[i1][j];
				t1 = A[i1][j+1];
				t2 = A[i1][j+2];
				t3 = A[i1][j+3];
				t4 = A[i1][j+4];
				t5 = A[i1][j+5];
				t6 = A[i1][j+6];
				t7 = A[i1][j+7];
				
				B[j][i1] = t0;
				B[j+1][i1] = t1;
				B[j+2][i1] = t2;
				B[j+3][i1] = t3;
				B[j][i1+4] = t4;
				B[j+1][i1+4] = t5;
				B[j+2][i1+4] = t6;
				B[j+3][i1+4] = t7;
				// 先将A左上翻转存储到B左上,A右上反转存储到B右上
			}
			for(int j1=j;j1<j+4;j1++){
				t0 = B[j1][i+4];
				t1 = B[j1][i+5];
				t2 = B[j1][i+6];
				t3 = B[j1][i+7];
				// 保存B的右上元素
				t4 = A[i+4][j1];
				t5 = A[i+5][j1];
				t6 = A[i+6][j1];
				t7 = A[i+7][j1];
				// A的左下元素
				B[j1][i+4] = t4;
				B[j1][i+5] = t5;
				B[j1][i+6] = t6;
				B[j1][i+7] = t7;
				// 将A左下翻转赋值给B右上
				B[j1+4][i] = t0;
				B[j1+4][i+1] = t1;
				B[j1+4][i+2] = t2;
				B[j1+4][i+3] = t3;
				// B的左下元素
				t0 = A[i+4][j1+4];
				t1 = A[i+5][j1+4];
				t2 = A[i+6][j1+4];
				t3 = A[i+7][j1+4];
				// A的右下元素
				B[j1+4][i+4] = t0;
				B[j1+4][i+5] = t1;
				B[j1+4][i+6] = t2;
				B[j1+4][i+7] = t3;
				// 翻转A的右下元素
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
//    registerTransFunction(trans_withoutOptimization, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

