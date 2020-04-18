#include <stdio.h>

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
    int kk, jj;
    int bsize = 2;

    for (kk = 0; kk < N; kk += bsize)
    {
        for (jj = 0; jj < M; jj += bsize)
        {
            if (jj != kk){
                B[jj + 0][kk + 0] = A[kk + 0][jj + 0];
            }
            if (jj+1 != kk){
                B[jj + 1][kk + 0] = A[kk + 0][jj + 1];
            }
            if (jj != kk+1){
                B[jj + 0][kk + 1] = A[kk + 1][jj + 0];
            }
            if (jj+1 != kk+1){
                B[jj + 1][kk + 1] = A[kk + 1][jj + 1];
            }
            B[jj + 0][kk + 0] = A[kk + 0][jj + 0];
            B[jj + 1][kk + 1] = A[kk + 1][jj + 1];
        }
    }
}

void printB(int M, int N, int B[M][N])
{
    int i, j;
    for (i = 0; i < M; i++)
    {
        for (j = 0; j < N; j++)
        {
            printf("%d  ", B[i][j]);
        }
        printf("\n");
    }
}

int main()
{
    int A[4][4] = {{1, 3, 5, 7}, {9, 2, 4, 6}, {8, 0, -1, -3}, {-5, -7, -9, -2}};
    int B[4][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
    transpose_submit(4, 4, A, B);
    printB(4, 4, B);
    return 0;
}