#include "LU.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int read_matrix(const char *filename, int n, double *A){
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Error, cannot open file\n");
        return -1;
    }
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (fscanf(fp, "%lf", &A[i * n + j]) != 1) {
                printf("Error, can't read the matrix\n");
                fclose(fp);
                return -1;
            }
        }
    }
    fclose(fp);
    return 0;
}







