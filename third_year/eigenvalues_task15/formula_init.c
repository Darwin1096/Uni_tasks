#include "Eigenvalues.h"

void init_matrix_formula(int n, int k, double *A){
    for (int i = 0; i < n; ++i){
        for (int j = 0; j < n; ++j){
            double value = 0.0;
            if (k == 1){
                int max_index = (i > j) ? i : j;
                value = n - max_index;
            } else if (k == 2){
                if (i == j) value = 2.0;
                else if (i - j == 1 || j - i == 1) value = -1.0;
            } else if (k == 3){
                if (i == j && i < n - 1) value = 1.0;
                else if (j == n - 1) value = (double)(i + 1);
                else if (i == n - 1) value = (double)(j + 1);
            } else if (k == 4){
                value = 1.0 / (double)(i + j + 1);
            }
            A[i * n + j] = value;
        }
    }
}
