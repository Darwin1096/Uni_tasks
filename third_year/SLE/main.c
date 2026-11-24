#include "LU.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>



void print_matrix(int l, int n, int m, double *mat){
    int rows = (l < m) ? l : m;
    int cols = (n < m) ? n : m;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            printf(" %10.3e", mat[i * n + j]);
        }
        printf("\n");
    }
}



int main(int argc, char **argv){
    if (argc < 4) {
        printf("Use like this: %s n m k [filename]\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    int k = atoi(argv[3]);
    double e1, e2, einf;
    double r1, r2, rinf;
    

    if (n <= 0 || m <= 0 || k < 0 || k > 4 || (k == 0 && argc < 5)) {
        printf("Error, invalid arguments\n");
        return 1;
    }
    if (k == 0 && argc != 5) {
    printf("Error, k=0 requires filename\n");
    return 1;
    }

    if (k != 0 && argc != 4) {
        printf("Error, k!=0 does not accept filename\n");
        return 1;
    }

    const char *filename = (k == 0) ? argv[4] : NULL;

    double *A = malloc((size_t)n * n * sizeof(double));
    double *b = malloc((size_t)n * sizeof(double));
    double *x = malloc((size_t)n * sizeof(double));
    if (!A || !b || !x) {
        printf("Error, couldn't allocate memory for matrix\n");
        free(A); 
        free(b);
        free(x);
        return 1;
    }

    if (k == 0) {
        if (read_matrix(filename, n, A) != 0) {
            printf("Error, couldn't read matrix \n");
            free(A); free(b); free(x);
            return 1;
        }
    } else {
        init_matrix_formula(n, k, A);
    }
    build_b(n, A, b);

    printf("Matrix A:\n");
    print_matrix(n, n, m, A);

    printf("Vector b:\n");
    print_matrix(1, n, m, b);

    clock_t start = clock();
    int res = solve_lu(n, A, b, x);
    clock_t end = clock();

    if (res != 0){
        printf("Error, decomposition failed, maybe matrix is singular or LU-decomposition can't be applied \n");
        free(A); free(b); free(x);
        return 1;
    }

    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Solution x:\n");
    print_matrix(1, n, m, x);


    int finite_x = 1;
    for (int i = 0; i < n; ++i) {
        if (!isfinite(x[i])) {
            finite_x = 0;
            break;
        }
    }

    if (!finite_x) {
        printf("Solution contains NaN or Inf\n");
    } else {
        //нормы погрешности
        error_norms(n, x, &e1, &e2, &einf);
        printf("Error norms: \n");
        printf("||x-x*||_1     = %.3e\n", e1);
        printf("||x-x*||_2     = %.3e\n", e2);
        printf("||x-x*||_inf   = %.3e\n", einf);
        printf("\n");

        if (k == 0) {
            if (read_matrix(filename, n, A) != 0) {
                printf("Error, failed to re-read matrix ");
                free(A);
                free(b);
                free(x);
                return 1;
            }
        } else {
            init_matrix_formula(n, k, A);
        }
        build_b(n, A, b);

        //нормы невязки
        residual_norms(n, A, x, b, &r1, &r2, &rinf);
        printf("Residual norms: \n");
        printf("||Ax-b||_1  / ||b||_1    = %.3e\n", r1);
        printf("||Ax-b||_2  / ||b||_2    = %.3e\n", r2);
        printf("||Ax-b||_inf / ||b||_inf = %.3e\n", rinf);
    }

    printf("Solving_time = %.2f s\n", time_spent);

    free(A);
    free(b);
    free(x);

    return 0;
}