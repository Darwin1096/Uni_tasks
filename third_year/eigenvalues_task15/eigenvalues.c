#include "Eigenvalues.h"
#include <math.h>
#include <stdlib.h>

static int tridiagonalize(int n, double *A, double *d, double *e){
    double *v = calloc((size_t)n, sizeof(double));
    double *w = calloc((size_t)n, sizeof(double));
    if (!v || !w){ free(v); free(w); return -1; }

    for (int k = 0; k < n - 2; ++k){
        for (int i = 0; i < n; ++i) v[i] = 0.0;
        double norm = 0.0;
        for (int i = k + 1; i < n; ++i){
            v[i] = A[i * n + k];
            norm = hypot(norm, v[i]);
        }
        if (norm == 0.0){
            e[k] = 0.0;
            continue;
        }

        double first = v[k + 1];
        v[k + 1] += copysign(norm, first);
        double vnorm = 0.0;
        for (int i = k + 1; i < n; ++i) vnorm = hypot(vnorm, v[i]);
        for (int i = k + 1; i < n; ++i) v[i] /= vnorm;

        for (int i = 0; i < n; ++i){
            w[i] = 0.0;
            for (int j = 0; j < n; ++j) w[i] += A[i * n + j] * v[j];
        }
        double alpha = 0.0;
        for (int i = 0; i < n; ++i) alpha += v[i] * w[i];
        for (int i = 0; i < n; ++i) w[i] -= alpha * v[i];

        for (int i = k + 1; i < n; ++i){
            for (int j = i; j < n; ++j){
                A[i * n + j] -= 2.0 * (v[i] * w[j] + w[i] * v[j]);
                A[j * n + i] = A[i * n + j];
            }
        }
        e[k] = -copysign(norm, first);
        A[(k + 1) * n + k] = e[k];
        A[k * n + k + 1] = e[k];
        for (int i = k + 2; i < n; ++i){
            A[i * n + k] = 0.0;
            A[k * n + i] = 0.0;
        }
    }

    if (n > 1) e[n - 2] = A[(n - 1) * n + n - 2];
    if (n > 0) e[n - 1] = 0.0;
    for (int i = 0; i < n; ++i) d[i] = A[i * n + i];
    free(v);
    free(w);
    return 0;
}

//Считаем перемены знака главных миноров матрицы T - xI через LU-разложение 
static int count_below(int n, const double *d, const double *e, double x){
    double scale = 1.0;

    for (int i = 0; i < n; ++i){
        scale = fmax(scale, fabs(d[i]) + fabs(x));
        if (i + 1 < n) scale = fmax(scale, fabs(e[i]));
    }

    double tiny = 1e-14 * scale;
    double pivot = d[0] - x;

    if (fabs(pivot) < tiny) pivot = -tiny;
    int minor_sign = (pivot < 0.0) ? -1 : 1;
    int previous_minor_sign = 1; //Знак нулевого минора равен знаку единицы
    int changes = (minor_sign != previous_minor_sign);

    for (int i = 1; i < n; ++i){
        double multiplier = e[i - 1] / pivot;
        pivot = d[i] - x - multiplier * e[i - 1];
        if (fabs(pivot) < tiny) pivot = -tiny;
        previous_minor_sign = minor_sign;
        minor_sign *= (pivot < 0.0) ? -1 : 1;
        if (minor_sign != previous_minor_sign) ++changes;
    }
    return changes;
}

int solve_eigenvalues(int n, double *A, double eps, double *values){
    double *d = malloc((size_t)n * sizeof(double));
    double *e = malloc((size_t)n * sizeof(double));
    if (!d || !e){ 
        free(d); 
        free(e); 
        return -1; 
    }

    if (tridiagonalize(n, A, d, e) != 0){
        free(d); 
        free(e); 
        return -1; 
    }

    double left = INFINITY, right = -INFINITY;

    for (int i = 0; i < n; ++i){
        double radius = (i > 0 ? fabs(e[i - 1]) : 0.0) + (i + 1 < n ? fabs(e[i]) : 0.0);
        left = fmin(left, d[i] - radius);
        right = fmax(right, d[i] + radius);
    }
    
    double margin = fmax(1.0, right - left);
    left -= margin;
    right += margin;

    for (int k = 0; k < n; ++k){
        double lo = left, hi = right;
        while (hi - lo > eps){
            double mid = lo + (hi - lo) / 2.0;
            if (count_below(n, d, e, mid) <= k) lo = mid;
            else hi = mid;
        }
        values[k] = lo + (hi - lo) / 2.0;
    }
    free(d);
    free(e);
    return 0;
}
