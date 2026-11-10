#include "LU.h"
#include <stdlib.h>

static double max(double i, double j){
    if(i < j){
        return j;
    }
    else{
        return i;
    }
}

static double f(int k, int n, int i, int j){
    if(k == 1){
        return n - max(i, j) + 1;
    }
    if(k == 2){
        return max(i, j);
    }
    if(k == 3){
        return abs(i - j);
    }
    else{
        return 1.0/(i + j -1);
    }
}

void init_matrix_formula(int n, int k, double *A){
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= n; ++j) {
            A[(i - 1) * n + (j - 1)] = f(k, n, i, j);
        }
    }
}