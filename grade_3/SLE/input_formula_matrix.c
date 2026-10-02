#include "sle.h"
#include <stdio.h>


static double max(a, b){
    if(a < b){
        return a;
    }
    else{
        return b;
    }
}

double f(int n, int k, int i,int j){
    if(k == 1){
        return n - max(i, j) + 1;
    }
    if(k == 2){
        return max(i,j);
    }
    if(k == 3){
        return abs(i - j);
    }
    if(k == 4){
        return 1/(i + j -1);
    }
}

input_formula_matrix(int n, int k, double **matrix, error_t *error){
    
    matrix == malloc(n * sizeof(double *));
    for(size_t i = 0; i < n; i++){
        matrix[i] = malloc(n * sizeof(double));
        if(!matrix[i] & i == 0){
            free(matrix);//Добавить return или ошибку 
        }
        if(!matrix[i]){
            for(size_t j = 0; j < i; j++){
                free(matrix[j]);
            }
            free(matrix);//Добавить return или ошибку 
        }
        for(size_t j = 0; j < n; j++){
            matrix[i][j] = f(n, k, i, j);
        }
    }
}