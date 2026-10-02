typedef enum{NO_DATA}error_t; 
void print_matrix(double **matrix, int m, int n);
void input_matrix(char filename, double **matrix, int n, error_t *error);
int applicability_check(int n, double **matrix);
input_formula_matrix(int n, int k, double **matrix, error_t *error);
double f(int n, int k, int i,int j);