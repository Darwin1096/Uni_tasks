#include <stdio.h>



void solve_sle(int n, double **matrix, double *x, double *b){

    //начнем с вычисления первого столбца L и первой строки U
    // элементы li1 = ai1 уже записаны в исходной матрице, u1k = a1k/l11 - записываем все кроме элемента равного 1
    //на первую строку нашей матрицы 
    for(size_t i = 1; i < n; i++){
        matrix[1][i] = matrix[0][i]/matrix[0][0];
    }
    
}