#include "Eigenvalues.h"
#include <stdio.h>

int read_matrix(const char *filename, int n, double *A){
    FILE *file = fopen(filename, "r");
    if (!file) return -1;

    for (int i = 0; i < n * n; ++i){
        if (fscanf(file, "%lf", &A[i]) != 1){
            fclose(file);
            return -1;
        }
    }

    double extra;
    if (fscanf(file, "%lf", &extra) == 1){
        fclose(file);
        return -1;
    }
    fclose(file);
    return 0;
}
