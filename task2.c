// .\task2.exe 0.0001
// drmemory -suppress mingw.supp -- .\task2.exe 0.001
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
    STATUS_INVALID_INTERVAL,
    STATUS_ITERATION_LIMIT,
    STATUS_MEMORY_ERROR
} Status;

typedef double (*Equation)(double x, double param);
typedef Status (*Method)(double eps, double *result);

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
    const double min_eps = 1e-14;
    if (!isfinite(eps) || eps < min_eps || eps >= 1.0) {
        return STATUS_INVALID_EPSILON;
    }
    return STATUS_OK;
}

int is_negative(const double x) {
    return x < 0.0;
}

Status bisection(Equation f, const double param, double a, double b, const double eps, double *root) {
    double fa;
    double fb;
    double mid;
    double fmid;
    const int max_steps = 200;
    int i;
    if (f == NULL || root == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (check_eps(eps) != STATUS_OK) {
        return STATUS_INVALID_EPSILON;
    }
    if (!isfinite(param)) {
        return STATUS_INVALID_NUMBER;
    }
    if (!isfinite(a) || !isfinite(b) || a > b - eps) {
        return STATUS_INVALID_INTERVAL;
    }
    fa = f(a, param);
    fb = f(b, param);
    if (!isfinite(fa) || !isfinite(fb) || is_negative(fa) == is_negative(fb)) {
        return STATUS_INVALID_INTERVAL;
    }
    for (i = 0; i < max_steps && b - a > eps; i++) {
        mid = (a + b) / 2.0;
        fmid = f(mid, param);
        if (!isfinite(fmid)) {
            return STATUS_INVALID_INTERVAL;
        }
        if (is_negative(fa) != is_negative(fmid)) {
            b = mid;
        } else {
            a = mid;
            fa = fmid;
        }
    }
    *root = (a + b) / 2.0;
    return b - a > eps ? STATUS_ITERATION_LIMIT : STATUS_OK;
}

double equation_e(const double x, const double param) {
    (void)param;
    return log(x) - 1.0;
}

double equation_pi(const double x, const double param) {
    (void)param;
    return cos(x / 2.0);
}

double equation_ln2(const double x, const double param) {
    (void)param;
    return exp(x) - 2.0;
}

double equation_sqrt2(const double x, const double param) {
    (void)param;
    return x * x - 2.0;
}

double equation_gamma(const double x, const double param) {
    return exp(-x) - param;
}

Status e_limit(const double eps, double *result) {
    double n = 1.0;
    double prev = 2.0;
    double cur;
    const int max_doublings = 52;
    int i;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (check_eps(eps) != STATUS_OK) {
        return STATUS_INVALID_EPSILON;
    }
    for (i = 0; i < max_doublings; i++) {
        n *= 2.0;
        cur = pow(1.0 + 1.0 / n, n);
        if (fabs(cur - prev) < eps / 2.0) {
            *result = cur;
            return STATUS_OK;
        }
        prev = cur;
    }
    *result = prev;
    return STATUS_ITERATION_LIMIT;
}

Status e_series(const double eps, double *result) {
    double sum = 1.0;
    double term = 1.0;
    long n;
    const long max_terms = 100;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (check_eps(eps) != STATUS_OK) {
        return STATUS_INVALID_EPSILON;
    }
    for (n = 1; n < max_terms; n++) {
        term /= (double)n;
        sum += term;
        if (term < eps) {
            *result = sum;
            return STATUS_OK;
        }
    }
    *result = sum;
    return STATUS_ITERATION_LIMIT;
}

Status e_equation(const double eps, double *result) {
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    return bisection(equation_e, 0.0, 2.0, 3.0, eps, result);
}

Status pi_limit(const double eps, double *result) {
    double value = 4.0;
    double prev = value;
    long n;
    long next_check = 2;
    const long max_n = 67108864L;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (check_eps(eps) != STATUS_OK) {
        return STATUS_INVALID_EPSILON;
    }
    for (n = 1; n < max_n; n++) {
        value = value * (2.0 * (double)n) / (2.0 * (double)n + 1.0) * (2.0 * (double)n + 2.0) / (2.0 * (double)n + 1.0);
        if (n + 1 == next_check) {
            if (fabs(value - prev) < eps / 2.0) {
                *result = value;
                return STATUS_OK;
            }
            prev = value;
            next_check *= 2;
        }
    }
    *result = value;
    return STATUS_ITERATION_LIMIT;
}

Status pi_series(const double eps, double *result) {
    double sum = 0.0;
    double term;
    long n;
    const long max_terms = 100000000L;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (check_eps(eps) != STATUS_OK) {
        return STATUS_INVALID_EPSILON;
    }
    for (n = 1; n < max_terms; n++) {
        term = 4.0 / (2.0 * (double)n - 1.0);
        if (term < eps) {
            *result = sum;
            return STATUS_OK;
        }
        sum += n % 2 == 1 ? term : -term;
    }
    *result = sum;
    return STATUS_ITERATION_LIMIT;
}

Status pi_equation(const double eps, double *result) {
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    return bisection(equation_pi, 0.0, 2.0, 4.0, eps, result);
}

Status ln2_limit(const double eps, double *result) {
    double n = 1.0;
    double y = 1.0;
    double prev = 1.0;
    double cur;
    const int max_doublings = 60;
    int i;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (check_eps(eps) != STATUS_OK) {
        return STATUS_INVALID_EPSILON;
    }
    for (i = 0; i < max_doublings; i++) {
        n *= 2.0;
        y = y / (sqrt(1.0 + y) + 1.0);
        cur = n * y;
        if (fabs(cur - prev) < eps / 2.0) {
            *result = cur;
            return STATUS_OK;
        }
        prev = cur;
    }
    *result = prev;
    return STATUS_ITERATION_LIMIT;
}

Status ln2_series(const double eps, double *result) {
    double sum = 0.0;
    double term;
    long n;
    const long max_terms = 100000000L;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (check_eps(eps) != STATUS_OK) {
        return STATUS_INVALID_EPSILON;
    }
    for (n = 1; n < max_terms; n++) {
        term = 1.0 / (double)n;
        if (term < eps) {
            *result = sum;
            return STATUS_OK;
        }
        sum += n % 2 == 1 ? term : -term;
    }
    *result = sum;
    return STATUS_ITERATION_LIMIT;
}

Status ln2_equation(const double eps, double *result) {
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    return bisection(equation_ln2, 0.0, 0.0, 1.0, eps, result);
}

Status sqrt2_limit(const double eps, double *result) {
    double x = -0.5;
    double next;
    int i;
    const int max_terms = 1000;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (check_eps(eps) != STATUS_OK) {
        return STATUS_INVALID_EPSILON;
    }
    for (i = 0; i < max_terms; i++) {
        next = x - x * x / 2.0 + 1.0;
        if (fabs(next - x) < eps / 2.0) {
            *result = next;
            return STATUS_OK;
        }
        x = next;
    }
    *result = x;
    return STATUS_ITERATION_LIMIT;
}

Status sqrt2_product(const double eps, double *result) {
    double product = 1.0;
    double exponent = 0.25;
    double term;
    int k;
    const int max_terms = 1000;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (check_eps(eps) != STATUS_OK) {
        return STATUS_INVALID_EPSILON;
    }
    for (k = 2; k < max_terms; k++) {
        term = pow(2.0, exponent);
        product *= term;
        if (product * (term - 1.0) < eps / 2.0) {
            *result = product;
            return STATUS_OK;
        }
        exponent /= 2.0;
    }
    *result = product;
    return STATUS_ITERATION_LIMIT;
}

Status sqrt2_equation(const double eps, double *result) {
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    return bisection(equation_sqrt2, 0.0, 1.0, 2.0, eps, result);
}

Status gamma_limit(const double eps, double *result) {
    long double prev = 0.0L;
    long double sum = 0.0L;
    long double binomial;
    long double log_factorial;
    const int max_m = 40;
    int m;
    int k;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (check_eps(eps) != STATUS_OK) {
        return STATUS_INVALID_EPSILON;
    }
    for (m = 1; m <= max_m; m++) {
        sum = 0.0L;
        binomial = 1.0L;
        log_factorial = 0.0L;
        for (k = 1; k <= m; k++) {
            binomial = binomial * (long double)(m - k + 1) / (long double)k;
            log_factorial += logl((long double)k);
            sum += (k % 2 == 0 ? 1.0L : -1.0L) * binomial / (long double)k * log_factorial;
        }
        if (m > 1 && (long double)m * fabsl(sum - prev) < (long double)eps) {
            *result = (double)sum;
            return STATUS_OK;
        }
        prev = sum;
    }
    *result = (double)sum;
    return STATUS_ITERATION_LIMIT;
}

Status gamma_series(const double eps, double *result) {
    double pi = acos(-1.0);
    double sum = -pi * pi / 6.0;
    double block;
    double inverse_square;
    const long max_blocks = 10000;
    long n;
    long k;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (check_eps(eps) != STATUS_OK) {
        return STATUS_INVALID_EPSILON;
    }
    for (n = 1; n <= max_blocks; n++) {
        block = 0.0;
        inverse_square = 1.0 / ((double)n * (double)n);
        for (k = n * n > 2 ? n * n : 2; k < (n + 1) * (n + 1); k++) {
            block += inverse_square - 1.0 / (double)k;
        }
        sum += block;
        if (inverse_square < eps) {
            *result = sum + 2.0 / ((double)n + 0.5);
            return STATUS_OK;
        }
    }
    *result = sum + 2.0 / ((double)max_blocks + 0.5);
    return STATUS_ITERATION_LIMIT;
}

Status mertens_limit(const double eps, double *result) {
    char *is_composite;
    double product = 1.0;
    const long max_prime_limit = 20000000L;
    long limit;
    long t;
    long j;
    Status status = STATUS_OK;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (check_eps(eps) != STATUS_OK) {
        return STATUS_INVALID_EPSILON;
    }
    if (eps < 2.0 / sqrt((double)max_prime_limit)) {
        limit = max_prime_limit;
        status = STATUS_ITERATION_LIMIT;
    } else {
        limit = (long)(4.0 / (eps * eps)) + 1;
    }
    is_composite = (char *)calloc((size_t)limit + 1, sizeof(char));
    if (is_composite == NULL) {
        return STATUS_MEMORY_ERROR;
    }
    for (t = 2; t <= limit; t++) {
        if (!is_composite[t]) {
            product *= (double)(t - 1) / (double)t;
            if (t <= limit / t) {
                for (j = t * t; j <= limit; j += t) {
                    is_composite[j] = 1;
                }
            }
        }
    }
    free(is_composite);
    *result = log((double)limit) * product;
    return status;
}

Status gamma_equation(const double eps, double *result) {
    double limit;
    Status limit_status;
    Status status;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (check_eps(eps) != STATUS_OK) {
        return STATUS_INVALID_EPSILON;
    }
    limit_status = mertens_limit(eps, &limit);
    if (limit_status != STATUS_OK && limit_status != STATUS_ITERATION_LIMIT) {
        return limit_status;
    }
    status = bisection(equation_gamma, limit, 0.0, 1.0, eps, result);
    if (status == STATUS_OK) {
        return limit_status;
    }
    return status;
}

int main(int argc, char *argv[]) {
    const char *names[5] = {"e", "pi", "ln 2", "sqrt 2", "gamma"};
    const char *method_names[3] = {"предел", "ряд", "уравнение"};
    Method methods[5][3] = {
        {e_limit, e_series, e_equation},
        {pi_limit, pi_series, pi_equation},
        {ln2_limit, ln2_series, ln2_equation},
        {sqrt2_limit, sqrt2_product, sqrt2_equation},
        {gamma_limit, gamma_series, gamma_equation}
    };
    double eps;
    double value;
    int i;
    int j;
    int has_errors = 0;
    Status status;

    if (argc != 2) {
        printf("Использование: task2.exe <эпсилон>\n");
        printf("Эпсилон - вещественное число от 1e-14 до 1 (1 не включается), например 0.0001\n");
        return 1;
    }
    status = parse_double(argv[1], &eps);
    if (status == STATUS_INVALID_NUMBER) {
        printf("Ошибка: эпсилон должен быть вещественным числом (например, 0.0001 или 1e-6)\n");
        return 1;
    }
    if (status != STATUS_OK || check_eps(eps) != STATUS_OK) {
        printf("Ошибка: эпсилон должен быть от 1e-14 до 1 (1 не включается); точность меньше 1e-14 недостижима в типе double\n");
        return 1;
    }

    for (i = 0; i < 5; i++) {
        printf("%s:\n", names[i]);
        for (j = 0; j < 3; j++) {
            status = methods[i][j](eps, &value);
            switch (status) {
                case STATUS_OK:
                    printf("  %s: %.15f\n", method_names[j], value);
                    break;
                case STATUS_ITERATION_LIMIT:
                    printf("  %s: %.15f (достигнут предел итераций, точность не гарантируется)\n", method_names[j], value);
                    break;
                case STATUS_MEMORY_ERROR:
                    printf("  %s: ошибка: не удалось выделить память\n", method_names[j]);
                    has_errors = 1;
                    break;
                case STATUS_INVALID_INTERVAL:
                    printf("  %s: ошибка: на интервале нет корня\n", method_names[j]);
                    has_errors = 1;
                    break;
                case STATUS_INVALID_EPSILON:
                    printf("  %s: ошибка: некорректный эпсилон\n", method_names[j]);
                    has_errors = 1;
                    break;
                default:
                    printf("  %s: ошибка вычисления\n", method_names[j]);
                    has_errors = 1;
                    break;
            }
        }
    }
    return has_errors ? 1 : 0;
}
