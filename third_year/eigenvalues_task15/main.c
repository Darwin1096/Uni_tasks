#include "Eigenvalues.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

void print_matrix(int l, int n, int m, double *mat){
    int rows = (l < m) ? l : m;
    int cols = (n < m) ? n : m;
    for (int i = 0; i < rows; ++i){
        for (int j = 0; j < cols; ++j) printf(" %10.3e", mat[i * n + j]);
        printf("\n");
    }
}

int main(int argc, char **argv){
    if (argc < 5){
        printf("Use like this: %s n m eps k [filename]\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]), m = atoi(argv[2]), k = atoi(argv[4]);
    double eps = atof(argv[3]);
    if (n <= 0 || m <= 0 || eps <= 0.0 || !isfinite(eps) || k < 0 || k > 4 ||
        (k == 0 && argc != 6) || (k != 0 && argc != 5)){
        printf("Error, invalid arguments\n");
        return 1;
    }

    double *A = malloc((size_t)n * n * sizeof(double));
    double *values = malloc((size_t)n * sizeof(double));
    if (!A || !values){
        printf("Error, couldn't allocate memory for matrix\n");
        free(A); free(values);
        return 1;
    }
    if (k == 0){
        if (read_matrix(argv[5], n, A) != 0){
            printf("Error, couldn't read matrix\n");
            free(A); free(values);
            return 1;
        }
    } else init_matrix_formula(n, k, A);

    for (int i = 0; i < n; ++i){
        for (int j = i + 1; j < n; ++j){
            if (fabs(A[i * n + j] - A[j * n + i]) > 1e-12){
                printf("Error, matrix must be symmetric\n");
                free(A); free(values);
                return 1;
            }
        }
    }
    printf("Matrix A:\n");
    print_matrix(n, n, m, A);
    clock_t start = clock();
    int res = solve_eigenvalues(n, A, eps, values);
    clock_t end = clock();
    if (res != 0){
        printf("Error, couldn't solve eigenvalue problem\n");
        free(A); free(values);
        return 1;
    }
    printf("Eigenvalues:\n");
    print_matrix(1, n, m, values);
    double r1, r2;
    residual_norms(n, A, values, &r1, &r2);
    printf("Residual norms:\n");
    printf("|tr(A) - sum(lambda_i)| = %.3e\n", r1);
    printf("| ||A||_F^2 - sum(lambda_i^2) | = %.3e\n", r2);
    printf("Solving_time = %.2f s\n", (double)(end - start) / CLOCKS_PER_SEC);
    free(A);
    free(values);
    return 0;
}
