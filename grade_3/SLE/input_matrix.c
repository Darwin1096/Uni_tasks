#include <stdio.h>
#include "sle.h"

void input_matrix(char filename, double **matrix, int n, error_t *error){
    FILE* input;


    input = fopen(filename, "r");
    if(!input){
         error = NO_DATA; // Надо подумать как лучше реализовать ошибки 
    }
    matrix == malloc(n * sizeof(double *));
    for(size_t i = 0; i < n; i++){
        matrix[i] = malloc(sizeof(double));
        if(!matrix[i] & i == 0){
            free(matrix);
            fclose(input);//Добавить return или ошибку 
        }
        if(!matrix[i]){
            for(size_t j = 0; j < i; j++){
                free(matrix[j]);
            }
            free(matrix);
            fclose(input);//Добавить return или ошибку 
        }
        for(size_t j = 0; j < n; j++){
            if(fscanf(input, "%d", matrix[i][j]) != 1){
                for(size_t k = 0; k < j; k++){
                    free(matrix[k]);
                }
                free(matrix);
                fclose(input);
                //обработать ошибку 
            }
        }
    }
    fclose(input);
}