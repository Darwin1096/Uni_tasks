#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static double rand_uniform(double lo, double hi) {
    return lo + (hi - lo) * ((double)rand() / RAND_MAX);
}

/* Случайное число: знак * мантисса[1,10) * 10^exp, exp ~ U(log_lo, log_hi) */
static double rand_scale(double log_lo, double log_hi) {
    double e = rand_uniform(log_lo, log_hi);
    double m = rand_uniform(1.0, 9.99);
    double s = (rand() & 1) ? 1.0 : -1.0;
    return s * m * pow(10.0, e);
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s n mode seed [outfile]\n", argv[0]);
        fprintf(stderr, "  mode 0: pure random, log10 in [-12,12], NOT tuned, LU likely fails\n");
        fprintf(stderr, "  mode 1: row-equilibrated, each row own scale in [1e-8,1e8], entries U(-1,1)\n");
        fprintf(stderr, "  mode 2: diagonally dominant, off-diag in [1e-8,1e8], diag = row sum * (1+U(0,1))\n");
        fprintf(stderr, "  mode 3: symmetric positive definite, mixed scales, diag = row sum * (1+U(0,1))\n");
        return 1;
    }
    int n = atoi(argv[1]);
    int mode = atoi(argv[2]);
    unsigned seed = (unsigned)atoi(argv[3]);
    srand(seed);

    FILE *fp = stdout;
    if (argc >= 5) {
        fp = fopen(argv[4], "w");
        if (!fp) { perror("fopen"); return 1; }
    }

    double *A = malloc((size_t)n * n * sizeof(double));
    if (!A) { fprintf(stderr, "out of memory\n"); return 1; }

    if (mode == 0) {
        /* Pure random, log-uniform in [-12, 12]. Без подгонки. */
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
                A[i * n + j] = rand_scale(-12.0, 12.0);
    }
    else if (mode == 1) {
        /* Каждая строка имеет свой масштаб, внутри строки — U(-1,1).
           Это типичный «смешанный» сценарий: большие строки, средние, маленькие. */
        for (int i = 0; i < n; ++i) {
            double s = pow(10.0, rand_uniform(-8.0, 8.0));
            for (int j = 0; j < n; ++j)
                A[i * n + j] = s * rand_uniform(-1.0, 1.0);
        }
    }
    else if (mode == 2) {
        /* Вне диагонали — разброс в 16 порядков, диагональ доминирует.
           Это «не подгонка» в смысле пивотов: они большие просто потому,
           что матрица диagonalно доминантна по построению. */
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j)
                A[i * n + j] = (i == j) ? 0.0 : rand_scale(-8.0, 8.0);
            double sum = 0.0;
            for (int j = 0; j < n; ++j) if (j != i) sum += fabs(A[i * n + j]);
            double sign = (rand() & 1) ? 1.0 : -1.0;
            A[i * n + i] = sign * sum * (1.0 + rand_uniform(0.0, 1.0));
        }
    }
    else if (mode == 3) {
        /* Симметричная: заполняем верхний треугольник в широком диапазоне,
           зеркалим, диагональ = сумма модулей по строке * (1+U(0,1)).
           Матрица SPD, разброс масштабов большой. */
        for (int i = 0; i < n; ++i) {
            for (int j = i; j < n; ++j) {
                double v = rand_scale(-8.0, 8.0);
                A[i * n + j] = v;
                A[j * n + i] = v;
            }
        }
        for (int i = 0; i < n; ++i) {
            double sum = 0.0;
            for (int j = 0; j < n; ++j) sum += fabs(A[i * n + j]);
            A[i * n + i] = sum * (1.0 + rand_uniform(0.0, 1.0));
        }
    }
    else {
        fprintf(stderr, "unknown mode %d\n", mode);
        return 1;
    }

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            fprintf(fp, "%.17g", A[i * n + j]);
            if (j < n - 1) fputc(' ', fp);
        }
        fputc('\n', fp);
    }
    if (fp != stdout) fclose(fp);
    free(A);
    return 0;
}