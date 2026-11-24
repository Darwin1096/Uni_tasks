#ifndef EIGENVALUES_H
#define EIGENVALUES_H

int read_matrix(const char *filename, int n, double *A);
void init_matrix_formula(int n, int k, double *A);
int solve_eigenvalues(int n, double *A, double eps, double *values);
void residual_norms(int n, double *A, double *values, double *r1, double *r2);
void print_matrix(int l, int n, int m, double *mat);

#endif
