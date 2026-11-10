#include "LU.h"

void build_b(int n, double *A, double *b){
    for (int i = 0; i < n; ++i) {
        b[i] = 0.0;
        for (int j = 0; j < n; j += 2){
            b[i] += A[i * n + j];
        }
    }
}