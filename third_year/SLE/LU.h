int solve_lu(int n, double *A, double *b, double *x);
void print_matrix(int l, int n, int m, double *mat);
int read_matrix(const char *filename, int n, double *A);
void init_matrix_formula(int n, int k, double *A);
void build_b(int n, double *A, double *b);
void residual_norms(int n, double *A, double *x, double *b, double *r1, double *r2, double *rinf);
void error_norms(int n, double *x, double *e1, double *e2, double *einf);
