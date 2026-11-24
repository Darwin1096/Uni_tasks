#include "LU.h"
#include <math.h>

int solve_lu(int n, double *A, double *b, double *x){
    for (int k = 0; k < n; ++k){
        double U = A[k * n + k];
        
        if (fabs(U) <= 0.0){
            return -1;
        }
        for (int i = k + 1; i < n; ++i){
            double L = A[i * n + k] / U;
            A[i * n + k] = L;
            for (int j = k + 1; j < n; ++j){
                A[i * n + j] -= L * A[k * n + j];
            }
        }
    }

    // Прямая подстановка: L * y = b
    for (int i = 0; i < n; ++i){
        double sum = b[i];
        for (int j = i - 1; j >= 0; --j){
            sum -= A[i * n + j] * b[j];
        }
        b[i] = sum;
    }

    // Обратная подстановка: U * x = y
    for (int i = n - 1; i >= 0; --i){
        double sum = b[i];
        for (int j = n - 1; j > i; --j){
            sum -= A[i * n + j] * x[j];
        }

        double d = 1.0 / A[i * n + i];
        x[i] = sum * d;
    }

    return 0;
}
