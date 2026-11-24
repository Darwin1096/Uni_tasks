#include "Eigenvalues.h"
#include <math.h>

void residual_norms(int n, double *A, double *values, double *r1, double *r2){
    double trace = 0.0, sum_values = 0.0;
    double square_matrix = 0.0, square_values = 0.0;
    for (int i = 0; i < n; ++i){
        trace += A[i * n + i];
        sum_values += values[i];
        square_values += values[i] * values[i];
        for (int j = 0; j < n; ++j) square_matrix += A[i * n + j] * A[i * n + j];
    }
    *r1 = fabs(trace - sum_values);
    *r2 = fabs(square_matrix - square_values);
}
