// .\task5.exe 0.000001 0.5

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

typedef enum {
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_INVALID_NUMBER,
    STATUS_OUT_OF_RANGE,
    STATUS_INVALID_EPSILON,
    STATUS_INVALID_X,
    STATUS_X_NOT_IN_DOMAIN,
    STATUS_INVALID_ARGUMENT,
    STATUS_INVALID_INTERVAL,
    STATUS_NOT_FINITE,
    STATUS_OVERFLOW,
    STATUS_PRECISION_LOSS,
    STATUS_ITERATION_LIMIT
} Status;

typedef Status (*Series)(double x, double eps, double *result);
typedef double (*Function)(double x);

int is_digit_char(const char c) {
    return c >= '0' && c <= '9';
}

Status parse_double(const char *str, double *result) {
    const char *p;
    char *end;
    double value;
    int digits = 0;
    int has_nonzero = 0;
    if (str == NULL || result == NULL) {
        return STATUS_NULL_POINTER;
    }
    p = str;
    if (*p == '-' || *p == '+') {
        p++;
    }
    for (; is_digit_char(*p); p++) {
        digits++;
        has_nonzero = has_nonzero || *p != '0';
    }
    if (*p == '.') {
        for (p++; is_digit_char(*p); p++) {
            digits++;
            has_nonzero = has_nonzero || *p != '0';
        }
    }
    if (digits == 0) {
        return STATUS_INVALID_NUMBER;
    }
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '-' || *p == '+') {
            p++;
        }
        if (!is_digit_char(*p)) {
            return STATUS_INVALID_NUMBER;
        }
        while (is_digit_char(*p)) {
            p++;
        }
    }
    if (*p != '\0') {
        return STATUS_INVALID_NUMBER;
    }
    value = strtod(str, &end);
    if (*end != '\0') {
        return STATUS_INVALID_NUMBER;
    }
    if (!isfinite(value) || (has_nonzero && fabs(value) < DBL_MIN)) {
        return STATUS_OUT_OF_RANGE;
    }
    *result = value + 0.0;
    return STATUS_OK;
}

Status check_eps(const double eps) {
    if (!isfinite(eps) || eps < DBL_EPSILON || eps >= 1.0) {
        return STATUS_INVALID_EPSILON;
    }
    return STATUS_OK;
}

Status check_rounding(const double abs_sum, const long terms, const double eps) {
    if (check_eps(eps) != STATUS_OK) {
        return STATUS_INVALID_EPSILON;
    }
    if (terms <= 0 || !isfinite(abs_sum)) {
        return STATUS_INVALID_ARGUMENT;
    }
    if ((double)terms * fabs(abs_sum) * DBL_EPSILON < eps / 2.0) {
        return STATUS_OK;
    }
    return STATUS_PRECISION_LOSS;
}

Status sum_a(const double x, const double eps, double *result) {
    double term = 1.0;
    double sum = 1.0;
    double abs_sum = 1.0;
    double ratio;
    long n;
    const long max_terms = 10000000L;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (check_eps(eps) != STATUS_OK) {
        return STATUS_INVALID_EPSILON;
    }
    if (!isfinite(x)) {
        return STATUS_INVALID_X;
    }
    for (n = 1; n < max_terms; n++) {
        term *= x / (double)n;
        sum += term;
        abs_sum += fabs(term);
        if (!isfinite(sum) || !isfinite(abs_sum)) {
            return STATUS_OVERFLOW;
        }
        ratio = fabs(x) / (double)(n + 1);
        if (fabs(term) * ratio < eps / 2.0 * (1.0 - ratio)) {
            *result = sum;
            return check_rounding(abs_sum, n + 1, eps);
        }
    }
    *result = sum;
    return STATUS_ITERATION_LIMIT;
}

Status sum_b(const double x, const double eps, double *result) {
    double term = 1.0;
    double sum = 1.0;
    double abs_sum = 1.0;
    double ratio;
    long n;
    const long max_terms = 10000000L;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (check_eps(eps) != STATUS_OK) {
        return STATUS_INVALID_EPSILON;
    }
    if (!isfinite(x)) {
        return STATUS_INVALID_X;
    }
    for (n = 0; n < max_terms; n++) {
        term *= -x * x / ((2.0 * (double)n + 1.0) * (2.0 * (double)n + 2.0));
        sum += term;
        abs_sum += fabs(term);
        if (!isfinite(sum) || !isfinite(abs_sum)) {
            return STATUS_OVERFLOW;
        }
        ratio = x * x / ((2.0 * (double)n + 3.0) * (2.0 * (double)n + 4.0));
        if (fabs(term) * ratio < eps / 2.0 * (1.0 - ratio)) {
            *result = sum;
            return check_rounding(abs_sum, n + 2, eps);
        }
    }
    *result = sum;
    return STATUS_ITERATION_LIMIT;
}

Status sum_c(const double x, const double eps, double *result) {
    double term = 1.0;
    double sum = 1.0;
    double ratio;
    long n;
    const long max_terms = 10000000L;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (check_eps(eps) != STATUS_OK) {
        return STATUS_INVALID_EPSILON;
    }
    if (!isfinite(x)) {
        return STATUS_INVALID_X;
    }
    if (fabs(x) > 1.0 - eps) {
        return STATUS_X_NOT_IN_DOMAIN;
    }
    for (n = 0; n < max_terms; n++) {
        ratio = 9.0 * ((double)n + 1.0) * ((double)n + 1.0) * x * x /
                ((3.0 * (double)n + 1.0) * (3.0 * (double)n + 2.0));
        if (term * ratio < eps / 2.0 * (1.0 - ratio)) {
            *result = sum;
            return check_rounding(sum, n + 1, eps);
        }
        term *= ratio;
        sum += term;
    }
    *result = sum;
    return STATUS_ITERATION_LIMIT;
}

Status sum_d(const double x, const double eps, double *result) {
    double term;
    double sum = 0.0;
    double abs_sum = 0.0;
    long n;
    const long max_terms = 10000000L;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (check_eps(eps) != STATUS_OK) {
        return STATUS_INVALID_EPSILON;
    }
    if (!isfinite(x)) {
        return STATUS_INVALID_X;
    }
    if (fabs(x) > 1.0 - eps) {
        return STATUS_X_NOT_IN_DOMAIN;
    }
    term = -x * x / 2.0;
    for (n = 1; n < max_terms; n++) {
        sum += term;
        abs_sum += fabs(term);
        if (fabs(term) < eps / 2.0) {
            *result = sum;
            return check_rounding(abs_sum, n, eps);
        }
        term *= -(2.0 * (double)n + 1.0) * x * x / (2.0 * (double)n + 2.0);
    }
    *result = sum;
    return STATUS_ITERATION_LIMIT;
}

double function_a(const double x) {
    return log(1.0 + x) / x;
}

double function_b(const double x) {
    return exp(-x * x / 2.0);
}

double function_c(const double x) {
    return -log(1.0 - x);
}

double function_d(const double x) {
    return pow(x, x);
}

Status midpoint_rule(Function f, const double a, const double b, const long n, const double eps,
                     double *result, double *abs_result) {
    double h;
    double value;
    double sum = 0.0;
    double abs_sum = 0.0;
    long i;
    if (f == NULL || result == NULL || abs_result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (check_eps(eps) != STATUS_OK) {
        return STATUS_INVALID_EPSILON;
    }
    if (n <= 0) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (!isfinite(a) || !isfinite(b) || a > b - eps) {
        return STATUS_INVALID_INTERVAL;
    }
    h = (b - a) / (double)n;
    if (!isfinite(h)) {
        return STATUS_OVERFLOW;
    }
    for (i = 0; i < n; i++) {
        value = f(a + h * ((double)i + 0.5));
        if (!isfinite(value)) {
            return STATUS_NOT_FINITE;
        }
        sum += value;
        abs_sum += fabs(value);
    }
    if (!isfinite(sum * h) || !isfinite(abs_sum * h)) {
        return STATUS_OVERFLOW;
    }
    *result = sum * h;
    *abs_result = abs_sum * h;
    return STATUS_OK;
}

Status integrate(Function f, const double a, const double b, const double eps, double *result) {
    double prev;
    double cur;
    double abs_value;
    const long max_splits = 1L << 20;
    long n;
    Status status;
    if (f == NULL || result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (check_eps(eps) != STATUS_OK) {
        return STATUS_INVALID_EPSILON;
    }
    if (!isfinite(a) || !isfinite(b) || a > b - eps) {
        return STATUS_INVALID_INTERVAL;
    }
    status = midpoint_rule(f, a, b, 1, eps, &prev, &abs_value);
    if (status != STATUS_OK) {
        return status;
    }
    for (n = 2; n <= max_splits; n *= 2) {
        status = midpoint_rule(f, a, b, n, eps, &cur, &abs_value);
        if (status != STATUS_OK) {
            return status;
        }
        if (fabs(cur - prev) < eps / 2.0) {
            *result = cur;
            return check_rounding(abs_value, n, eps);
        }
        prev = cur;
    }
    *result = prev;
    return STATUS_ITERATION_LIMIT;
}

int is_reported_status(const Status status) {
    return status == STATUS_OK || status == STATUS_PRECISION_LOSS || status == STATUS_ITERATION_LIMIT ||
           status == STATUS_X_NOT_IN_DOMAIN || status == STATUS_OVERFLOW;
}

void print_usage(void) {
    printf("Использование: task5.exe <эпсилон> <x>\n");
    printf("  эпсилон - точность вычислений, вещественное число: %.3g <= эпсилон < 1\n", DBL_EPSILON);
    printf("  x - вещественный аргумент рядов (интегралы от x не зависят)\n");
}

void print_result(const char *name, const Status status, const double value) {
    if (name == NULL) {
        return;
    }
    switch (status) {
        case STATUS_OK:
            printf("  %s = %.17g\n", name, value);
            break;
        case STATUS_PRECISION_LOSS:
            printf("  %s = %.17g (предупреждение: из-за ошибок округления double точность эпсилон не гарантирована)\n",
                   name, value);
            break;
        case STATUS_ITERATION_LIMIT:
            printf("  %s = %.17g (предупреждение: достигнут предел итераций, точность эпсилон не достигнута)\n",
                   name, value);
            break;
        case STATUS_X_NOT_IN_DOMAIN:
            printf("  %s: ряд вычисляется только при |x| < 1 (с учётом эпсилон: |x| <= 1 - эпсилон)\n", name);
            break;
        case STATUS_OVERFLOW:
            printf("  %s: переполнение типа double при вычислении (слишком большое |x|)\n", name);
            break;
        case STATUS_NOT_FINITE:
            printf("  %s: функция не определена или бесконечна на отрезке интегрирования\n", name);
            break;
        default:
            printf("  %s: внутренняя ошибка вычисления\n", name);
            break;
    }
}

int main(int argc, char *argv[]) {
    const char *names[4] = {"a", "b", "c", "d"};
    Series sums[4] = {sum_a, sum_b, sum_c, sum_d};
    Function functions[4] = {function_a, function_b, function_c, function_d};
    double eps;
    double x;
    double value;
    Status status;
    int exit_code = 0;
    int i;

    if (argc != 3) {
        printf("Ошибка: неверное количество аргументов\n");
        print_usage();
        return 1;
    }
    status = parse_double(argv[1], &eps);
    if (status != STATUS_OK && status != STATUS_OUT_OF_RANGE) {
        printf("Ошибка: эпсилон должен быть вещественным числом\n");
        print_usage();
        return 1;
    }
    if (status == STATUS_OUT_OF_RANGE || check_eps(eps) != STATUS_OK) {
        printf("Ошибка: эпсилон должен удовлетворять %.3g <= эпсилон < 1 (меньшая точность недостижима в типе double)\n",
               DBL_EPSILON);
        return 1;
    }
    status = parse_double(argv[2], &x);
    if (status == STATUS_OUT_OF_RANGE) {
        printf("Ошибка: x выходит за пределы типа double (допустимо x = 0 или %.3g <= |x| <= %.4g)\n", DBL_MIN, DBL_MAX);
        return 1;
    }
    if (status != STATUS_OK) {
        printf("Ошибка: x должен быть вещественным числом\n");
        print_usage();
        return 1;
    }

    printf("Суммы рядов при x = %.15g:\n", x);
    for (i = 0; i < 4; i++) {
        value = 0.0;
        status = sums[i](x, eps, &value);
        print_result(names[i], status, value);
        if (!is_reported_status(status)) {
            exit_code = 1;
        }
    }

    printf("Интегралы от 0 до 1:\n");
    for (i = 0; i < 4; i++) {
        value = 0.0;
        status = integrate(functions[i], 0.0, 1.0, eps, &value);
        print_result(names[i], status, value);
        if (!is_reported_status(status)) {
            exit_code = 1;
        }
    }
    return exit_code;
}
