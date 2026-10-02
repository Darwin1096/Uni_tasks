#include "sle.h"
#include <stdio.h>


//вспомогательные функции для подсчета определителя 
static void add_multiple_string(double** matrix, double a, int k, int s, int n){
    for(int j = 0; j < n; j++){
        matrix[k][j] += a * matrix[s][j];
    }
}

static void swap_string(double** matrix, int k, int s, int n){
    double h = 0;
    double* teap = matrix[k];
    matrix[k] = matrix[s];
    matrix[s] = teap;
}

static double find_determinator(double** matrix, int n, double eps){
    double result = 1;

	for (size_t i = 0; i < n-1; i++)
	{
		size_t j = i;

		while (fabs(matrix[j][i]) < eps)
		{
			j += 1;
			// all rest elements in this row are 0.
			if (j == n)
			{
				return 0;
			}
		}
		if (i != j)
		{
			swap_string(matrix, j, i, n);
			result *= -1;
		}
		
		result *= matrix[i][i];
		
		for (size_t k = i+1; k < n; k++)
		{
			add_multiple_string(matrix, -matrix[k][i]/matrix[i][i], i, k, n);
		}
	}

	result *= matrix[n-1][n-1];

	return result;
}

//Проверка применимости метода LU - разложения
int applicability_check(int n, double **matrix){
    double **minor;

    //Делаем единичную матрицу размера n x n
    minor == malloc(n * sizeof(double *));
    for(size_t i = 0; i < n; i++){
        minor[i] = malloc(n * sizeof(double));
        if(!minor[i] & i == 0){
            free(minor);//Добавить return или ошибку 
        }
        if(!minor[i]){
            for(size_t j = 0; j < i; j++){
                free(minor[j]);
            }
            free(minor);//Добавить return или ошибку 
        }
        minor[i][i] = 1;
    }

    for(size_t i = 0; i < n; i++){
        for(size_t j = 0; j <= i; j++){
            minor[i][j] = matrix[i][j];
        }
        if(find_determinator(i, minor, 1.0e-8) == 0){
            return -1;
        }
    }
}

void print_matrix(double **matrix, int m, int n){
    for(size_t i = 0; i < m; i++){
        for(size_t j = 0; j < n; j++){
            printf("%10.3e ", matrix[i][j]);
        }
        printf("\n");
    }
   printf("\n");
}



int main(int argc, char* argv[]){
    int n = argv[1];
    int m = argv[2];
    int k = argv[3];
    char filename = argv[4];
    double **matrix;
    double *x;
    double *b;
    error_t error_code;
    
    if(k != 0 || k != 1 || k != 2 || k != 3 || k != 4){
        printf("WRONG FORMULA VALUE");
        return -1;
    }
    if(n > m){
        printf("INFINITY SOLUTIONS");
        return -1;
    }
    if(n < m){
        printf("NO SOLUTIONS EXCEPT TRIVIAL");
    }
    if(k == 0){
        input_matrix(filename, matrix, n, &error_code);
    }
    else{
        input_formula_matrix(n, k, matrix, &error_code);
    }

    b = malloc(sizeof(n));
    if(!b){
        free(matrix);
    }

    for(size_t i = 0; i < n; i++){
        
    }

    print_matrix(matrix, m, n);

    if(applicability_check(n, matrix) != -1){
        solve_sle(n, matrix, &x, b);
    }
    print_matrix(x, 1, n);

}