#include "LU.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static void vector_norms(int n, const double *v, double *n1, double *n2, double *ninf){
    double s1 = 0.0, s2 = 0.0, mx = 0.0;
    int has_nan = 0, has_inf = 0;

    for (int i = 0; i < n; ++i) {
        double a = fabs(v[i]);
        if (isnan(a)) {
            has_nan = 1;
            continue;
        }
        if (isinf(a)) {
            has_inf = 1;
            continue;
        }
        s1 += a;
        s2 += a * a;
        if (a > mx) mx = a;
    }

    if (has_nan) {
        *n1 = *n2 = *ninf = NAN;
        return;
    }
    if (has_inf) {
        *n1 = *n2 = *ninf = INFINITY;
        return;
    }

    *n1   = s1;
    *n2   = sqrt(s2);
    *ninf = mx;
}

void residual_norms(int n, double *A, double *x, double *b, double *r1, double *r2, double *rinf){
    double *r = malloc((size_t)n * sizeof(double));
    if (!r){
        *r1 = *r2 = *rinf = NAN;
        return;
    }

    for (int i = 0; i < n; ++i){
        double ax = 0.0;
        for (int j = 0; j < n; ++j)
            ax += A[i * n + j] * x[j];
        r[i] = ax - b[i];
    }

    double rn1, rn2, rninf;
    double bn1, bn2, bninf;
    vector_norms(n, r, &rn1, &rn2, &rninf);
    vector_norms(n, b, &bn1, &bn2, &bninf);

    *r1   = isfinite(bn1)   ? ((bn1   > 0.0) ? rn1   / bn1   : rn1)   : NAN;
    *r2   = isfinite(bn2)   ? ((bn2   > 0.0) ? rn2   / bn2   : rn2)   : NAN;
    *rinf = isfinite(bninf) ? ((bninf > 0.0) ? rninf / bninf : rninf) : NAN;

    free(r);
}

void error_norms(int n, double *x, double *e1, double *e2, double *einf){
    double s1 = 0.0, s2 = 0.0, mx = 0.0;
    int has_nan = 0, has_inf = 0;

    for (int i = 0; i < n; ++i){
        double exact = (i % 2 == 0) ? 1.0 : 0.0;
        double d = fabs(x[i] - exact);
        if (isnan(d)) {
            has_nan = 1;
            continue;
        }
        if (isinf(d)) {
            has_inf = 1;
            continue;
        }
        s1 += d;
        s2 += d * d;
        if (d > mx) mx = d;
    }

    if (has_nan) {
        *e1 = *e2 = *einf = NAN;
        return;
    }
    if (has_inf) {
        *e1 = *e2 = *einf = INFINITY;
        return;
    }

    *e1   = s1;
    *e2   = sqrt(s2);
    *einf = mx;
}
