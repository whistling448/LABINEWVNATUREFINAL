// drmemory -suppress mingw.supp -- .\task6.exe
// .\task6.exe

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <limits.h>

typedef enum {
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_INVALID_EPSILON,
    STATUS_INVALID_COUNT,
    STATUS_INVALID_BASE,
    STATUS_INVALID_NUMBER,
    STATUS_NOT_FINITE,
    STATUS_NEGATIVE_NUMBER,
    STATUS_OUT_OF_RANGE,
    STATUS_INVALID_INTERVAL,
    STATUS_NO_SIGN_CHANGE,
    STATUS_FUNCTION_ERROR,
    STATUS_DIVISION_BY_ZERO,
    STATUS_OVERFLOW,
    STATUS_UNDERFLOW,
    STATUS_DUPLICATE_POINTS,
    STATUS_DEGENERATE_POLYGON,
    STATUS_MEMORY_ERROR,
    STATUS_ITERATION_LIMIT
} Status;

Status is_convex(int *result, const double eps, const int count, ...) {
    const double full_turn = 2.0 * acos(-1.0);
    const double min_eps = 1e-12;
    const double max_eps = 1e-3;
    double *points;
    double total_angle = 0.0;
    double ex1, ey1, ex2, ey2;
    double length1, length2;
    double cross, dot;
    long turns;
    int sign = 0;
    int current;
    int convex = 1;
    int i, j, k;
    Status status = STATUS_OK;
    va_list args;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (!isfinite(eps) || eps < min_eps || eps > max_eps) {
        return STATUS_INVALID_EPSILON;
    }
    if (count < 3 || count > INT_MAX / 2) {
        return STATUS_INVALID_COUNT;
    }
    points = (double *)malloc(2 * (size_t)count * sizeof(double));
    if (points == NULL) {
        return STATUS_MEMORY_ERROR;
    }
    va_start(args, count);
    for (i = 0; i < 2 * count; i++) {
        points[i] = va_arg(args, double);
        if (!isfinite(points[i])) {
            status = STATUS_NOT_FINITE;
        }
    }
    va_end(args);

    for (i = 0; i < count && status == STATUS_OK; i++) {
        j = (i + 1) % count;
        k = (i + 2) % count;
        ex1 = points[2 * j] - points[2 * i];
        ey1 = points[2 * j + 1] - points[2 * i + 1];
        ex2 = points[2 * k] - points[2 * j];
        ey2 = points[2 * k + 1] - points[2 * j + 1];
        length1 = hypot(ex1, ey1);
        length2 = hypot(ex2, ey2);
        if (!isfinite(length1) || !isfinite(length2)) {
            status = STATUS_OVERFLOW;
        } else if (fpclassify(length1) == FP_ZERO || fpclassify(length2) == FP_ZERO) {
            status = STATUS_DUPLICATE_POINTS;
        } else {
            ex1 /= length1;
            ey1 /= length1;
            ex2 /= length2;
            ey2 /= length2;
            cross = ex1 * ey2 - ey1 * ex2;
            dot = ex1 * ex2 + ey1 * ey2;
            total_angle += atan2(cross, dot);
            if (fabs(cross) < eps) {
                if (dot < -eps) {
                    convex = 0;
                }
            } else {
                current = cross > 0.0 ? 1 : -1;
                if (sign == 0) {
                    sign = current;
                } else if (sign != current) {
                    convex = 0;
                }
            }
        }
    }
    free(points);
    if (status == STATUS_OK && sign == 0) {
        status = STATUS_DEGENERATE_POLYGON;
    }
    if (status != STATUS_OK) {
        return status;
    }
    turns = lround(total_angle / full_turn);
    *result = convex && (turns == 1 || turns == -1);
    return STATUS_OK;
}

Status polynomial(double *result, const double x, const int degree, ...) {
    double value = 0.0;
    double coefficient;
    int i;
    Status status = STATUS_OK;
    va_list args;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (degree < 0) {
        return STATUS_INVALID_COUNT;
    }
    if (!isfinite(x)) {
        return STATUS_NOT_FINITE;
    }
    va_start(args, degree);
    for (i = degree; i >= 0; i--) {
        coefficient = va_arg(args, double);
        if (!isfinite(coefficient)) {
            status = STATUS_NOT_FINITE;
        } else if (status == STATUS_OK) {
            value = value * x + coefficient;
            if (!isfinite(value)) {
                status = STATUS_OVERFLOW;
            }
        }
    }
    va_end(args);
    if (status != STATUS_OK) {
        return status;
    }
    *result = value;
    return STATUS_OK;
}

Status char_to_digit(const char c, int *digit) {
    if (digit == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (c >= '0' && c <= '9') {
        *digit = c - '0';
    } else if (c >= 'A' && c <= 'Z') {
        *digit = c - 'A' + 10;
    } else if (c >= 'a' && c <= 'z') {
        *digit = c - 'a' + 10;
    } else {
        return STATUS_INVALID_NUMBER;
    }
    return STATUS_OK;
}

Status parse_in_base(const char *str, const int base, unsigned long long *value) {
    int digit;
    if (str == NULL || value == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (base < 2 || base > 36) {
        return STATUS_INVALID_BASE;
    }
    if (*str == '\0') {
        return STATUS_INVALID_NUMBER;
    }
    *value = 0;
    for (; *str != '\0'; str++) {
        if (char_to_digit(*str, &digit) != STATUS_OK || digit >= base) {
            return STATUS_INVALID_NUMBER;
        }
        if (*value > (ULLONG_MAX - (unsigned long long)digit) / (unsigned long long)base) {
            return STATUS_OUT_OF_RANGE;
        }
        *value = *value * (unsigned long long)base + (unsigned long long)digit;
    }
    return STATUS_OK;
}

Status is_kaprekar(const unsigned long long n, const int base, int *result) {
    unsigned long long square;
    unsigned long long power;
    unsigned long long left;
    unsigned long long right;
    const unsigned long long max_value = 4294967295ULL;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (base < 2 || base > 36) {
        return STATUS_INVALID_BASE;
    }
    if (n > max_value) {
        return STATUS_OUT_OF_RANGE;
    }
    *result = 0;
    square = n * n;
    power = (unsigned long long)base;
    while (1) {
        left = square / power;
        right = square % power;
        if (right > 0 && left + right == n) {
            *result = 1;
            return STATUS_OK;
        }
        if (left == 0 || power > ULLONG_MAX / (unsigned long long)base) {
            return STATUS_OK;
        }
        power *= (unsigned long long)base;
    }
}

Status find_kaprekar(int flags[], const int capacity, int *error_index, const int base, const int count, ...) {
    const char *str;
    unsigned long long value;
    Status status = STATUS_OK;
    int i;
    va_list args;
    if (flags == NULL || error_index == NULL) {
        return STATUS_NULL_POINTER;
    }
    *error_index = -1;
    if (base < 2 || base > 36) {
        return STATUS_INVALID_BASE;
    }
    if (count < 0 || count > capacity) {
        return STATUS_INVALID_COUNT;
    }
    va_start(args, count);
    for (i = 0; i < count && status == STATUS_OK; i++) {
        str = va_arg(args, const char *);
        status = parse_in_base(str, base, &value);
        if (status == STATUS_OK) {
            status = is_kaprekar(value, base, &flags[i]);
        }
        if (status != STATUS_OK) {
            *error_index = i;
        }
    }
    va_end(args);
    return status;
}

Status geometric_mean(double *result, const double eps, const int count, ...) {
    double log_sum = 0.0;
    double value;
    double mean;
    int has_zero = 0;
    int i;
    Status status = STATUS_OK;
    va_list args;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (!isfinite(eps) || eps <= 0.0) {
        return STATUS_INVALID_EPSILON;
    }
    if (count <= 0) {
        return STATUS_INVALID_COUNT;
    }
    va_start(args, count);
    for (i = 0; i < count; i++) {
        value = va_arg(args, double);
        if (!isfinite(value)) {
            status = STATUS_NOT_FINITE;
        } else if (value < -eps) {
            if (status == STATUS_OK) {
                status = STATUS_NEGATIVE_NUMBER;
            }
        } else if (value < eps) {
            has_zero = 1;
        } else {
            log_sum += log(value);
        }
    }
    va_end(args);
    if (status != STATUS_OK) {
        return status;
    }
    if (has_zero) {
        *result = 0.0;
        return STATUS_OK;
    }
    mean = exp(log_sum / count);
    if (!isfinite(mean)) {
        return STATUS_OVERFLOW;
    }
    *result = mean;
    return STATUS_OK;
}

Status power_recursive(const double base, const unsigned int n, double *result) {
    double half;
    double value;
    Status status;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (!isfinite(base)) {
        return STATUS_NOT_FINITE;
    }
    if (n == 0) {
        *result = 1.0;
        return STATUS_OK;
    }
    status = power_recursive(base, n / 2, &half);
    if (status != STATUS_OK) {
        return status;
    }
    value = half * half;
    if (n % 2 == 1) {
        value *= base;
    }
    if (!isfinite(value)) {
        return STATUS_OVERFLOW;
    }
    if (fpclassify(value) != FP_NORMAL && fpclassify(base) != FP_ZERO) {
        return STATUS_UNDERFLOW;
    }
    *result = value;
    return STATUS_OK;
}

Status fast_pow(double *result, const double base, const int exponent, const double eps) {
    double actual_base = base;
    unsigned int n;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (!isfinite(eps) || eps <= 0.0) {
        return STATUS_INVALID_EPSILON;
    }
    if (!isfinite(base)) {
        return STATUS_NOT_FINITE;
    }
    if (exponent >= 0) {
        n = (unsigned int)exponent;
    } else {
        if (fabs(base) < eps) {
            return STATUS_DIVISION_BY_ZERO;
        }
        actual_base = 1.0 / base;
        if (!isfinite(actual_base)) {
            return STATUS_OVERFLOW;
        }
        n = 0U - (unsigned int)exponent;
    }
    return power_recursive(actual_base, n, result);
}

int sign_of(const double value) {
    if (value > 0.0) {
        return 1;
    }
    if (value < 0.0) {
        return -1;
    }
    return 0;
}

Status bisection(double *root, double (*f)(double), const double a, const double b, const double eps) {
    const int max_steps = 2200;
    double left = a;
    double right = b;
    double fa;
    double fb;
    double mid;
    double fmid;
    int sign_left;
    int sign_right;
    int sign_mid;
    int i;
    if (root == NULL || f == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (!isfinite(eps) || eps <= 0.0) {
        return STATUS_INVALID_EPSILON;
    }
    if (!isfinite(a) || !isfinite(b) || b - a < eps) {
        return STATUS_INVALID_INTERVAL;
    }
    fa = f(a);
    fb = f(b);
    if (!isfinite(fa) || !isfinite(fb)) {
        return STATUS_FUNCTION_ERROR;
    }
    sign_left = sign_of(fa);
    sign_right = sign_of(fb);
    if (sign_left == 0) {
        *root = a;
        return STATUS_OK;
    }
    if (sign_right == 0) {
        *root = b;
        return STATUS_OK;
    }
    if (sign_left == sign_right) {
        return STATUS_NO_SIGN_CHANGE;
    }
    for (i = 0; i < max_steps && right - left > eps; i++) {
        mid = left / 2.0 + right / 2.0;
        fmid = f(mid);
        if (!isfinite(fmid)) {
            return STATUS_FUNCTION_ERROR;
        }
        sign_mid = sign_of(fmid);
        if (sign_mid == 0) {
            *root = mid;
            return STATUS_OK;
        }
        if (sign_mid == sign_left) {
            left = mid;
        } else {
            right = mid;
        }
    }
    *root = left / 2.0 + right / 2.0;
    return right - left > eps ? STATUS_ITERATION_LIMIT : STATUS_OK;
}

double square_minus_two(const double x) {
    return x * x - 2.0;
}

double cos_minus_x(const double x) {
    return cos(x) - x;
}

double cubic(const double x) {
    return x * x * x - x - 2.0;
}

double sine(const double x) {
    return sin(x);
}

double x_plus_log(const double x) {
    return x + log(fabs(x));
}

const char *status_message(const Status status) {
    switch (status) {
        case STATUS_OK:
            return "успешно";
        case STATUS_NULL_POINTER:
            return "передан нулевой указатель";
        case STATUS_INVALID_EPSILON:
            return "некорректная точность eps";
        case STATUS_INVALID_COUNT:
            return "некорректное количество аргументов";
        case STATUS_INVALID_BASE:
            return "основание системы счисления должно быть от 2 до 36";
        case STATUS_INVALID_NUMBER:
            return "некорректная запись числа (пустая строка или недопустимая цифра)";
        case STATUS_NOT_FINITE:
            return "передано не конечное число (NaN или бесконечность)";
        case STATUS_NEGATIVE_NUMBER:
            return "передано отрицательное число";
        case STATUS_OUT_OF_RANGE:
            return "число больше допустимого максимума 4294967295";
        case STATUS_INVALID_INTERVAL:
            return "некорректный интервал (нужны конечные границы a < b, b - a не меньше eps)";
        case STATUS_NO_SIGN_CHANGE:
            return "функция не меняет знак на концах интервала";
        case STATUS_FUNCTION_ERROR:
            return "значение функции не является конечным числом";
        case STATUS_DIVISION_BY_ZERO:
            return "деление на ноль";
        case STATUS_OVERFLOW:
            return "переполнение: значение слишком велико для типа double";
        case STATUS_UNDERFLOW:
            return "потеря точности: результат слишком мал для типа double";
        case STATUS_DUPLICATE_POINTS:
            return "соседние вершины совпадают";
        case STATUS_DEGENERATE_POLYGON:
            return "все вершины лежат на одной прямой";
        case STATUS_MEMORY_ERROR:
            return "не удалось выделить память";
        case STATUS_ITERATION_LIMIT:
            return "заданная точность недостижима в типе double";
    }
    return "неизвестная ошибка";
}

void print_convex(const char *name, const Status status, const int result) {
    if (status == STATUS_OK) {
        printf("  %s: %s\n", name, result ? "выпуклый" : "невыпуклый");
    } else {
        printf("  %s: ошибка - %s\n", name, status_message(status));
    }
}

Status demo_convex(void) {
    const double eps = 1e-9;
    int result = 0;
    int memory_failed = 0;
    Status status;

    printf("1. Выпуклость многоугольника (eps = %g)\n", eps);
    status = is_convex(&result, eps, 4, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0);
    print_convex("квадрат", status, result);
    memory_failed = memory_failed || status == STATUS_MEMORY_ERROR;
    status = is_convex(&result, eps, 3, 0.0, 0.0, 0.0, 3.0, 4.0, 0.0);
    print_convex("треугольник, обход по часовой стрелке", status, result);
    memory_failed = memory_failed || status == STATUS_MEMORY_ERROR;
    status = is_convex(&result, eps, 5, 0.0, 0.0, 2.0, 0.0, 3.0, 1.5, 1.0, 3.0, -1.0, 1.5);
    print_convex("пятиугольник", status, result);
    memory_failed = memory_failed || status == STATUS_MEMORY_ERROR;
    status = is_convex(&result, eps, 5, 0.0, 0.0, 1.0, 0.0, 2.0, 0.0, 2.0, 2.0, 0.0, 2.0);
    print_convex("квадрат с вершиной на стороне", status, result);
    memory_failed = memory_failed || status == STATUS_MEMORY_ERROR;
    status = is_convex(&result, eps, 4, 0.0, 0.0, 1e-6, 0.0, 1e-6, 1e-6, 0.0, 1e-6);
    print_convex("квадрат со стороной 1e-6", status, result);
    memory_failed = memory_failed || status == STATUS_MEMORY_ERROR;
    status = is_convex(&result, eps, 4, 0.0, 0.0, 2.0, 0.0, 1.0, 0.5, 1.0, 2.0);
    print_convex("четырёхугольник с вогнутостью", status, result);
    memory_failed = memory_failed || status == STATUS_MEMORY_ERROR;
    status = is_convex(&result, eps, 4, 0.0, 0.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0);
    print_convex("самопересекающийся четырёхугольник", status, result);
    memory_failed = memory_failed || status == STATUS_MEMORY_ERROR;
    status = is_convex(&result, eps, 5, 0.0, 10.0, 5.88, -8.09, -9.51, 3.09, 9.51, 3.09, -5.88, -8.09);
    print_convex("пятиконечная звезда", status, result);
    memory_failed = memory_failed || status == STATUS_MEMORY_ERROR;
    status = is_convex(&result, eps, 2, 0.0, 0.0, 1.0, 1.0);
    print_convex("две точки", status, result);
    memory_failed = memory_failed || status == STATUS_MEMORY_ERROR;
    status = is_convex(&result, eps, 4, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 1.0);
    print_convex("повторяющаяся вершина", status, result);
    memory_failed = memory_failed || status == STATUS_MEMORY_ERROR;
    status = is_convex(&result, eps, 3, 0.0, 0.0, 1.0, 1.0, 2.0, 2.0);
    print_convex("точки на одной прямой", status, result);
    memory_failed = memory_failed || status == STATUS_MEMORY_ERROR;
    status = is_convex(&result, eps, 3, 0.0, 0.0, NAN, 0.0, 0.0, 1.0);
    print_convex("координата NaN", status, result);
    memory_failed = memory_failed || status == STATUS_MEMORY_ERROR;
    status = is_convex(&result, 0.0, 4, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0);
    print_convex("квадрат, eps = 0", status, result);
    memory_failed = memory_failed || status == STATUS_MEMORY_ERROR;
    status = is_convex(&result, 1e-20, 4, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0);
    print_convex("квадрат, eps = 1e-20 (меньше 1e-12)", status, result);
    memory_failed = memory_failed || status == STATUS_MEMORY_ERROR;
    return memory_failed ? STATUS_MEMORY_ERROR : STATUS_OK;
}

void print_polynomial(const char *name, const Status status, const double result) {
    if (status == STATUS_OK) {
        printf("  %s: %g\n", name, result);
    } else {
        printf("  %s: ошибка - %s\n", name, status_message(status));
    }
}

void demo_polynomial(void) {
    double result = 0.0;
    Status status;

    printf("2. Значение многочлена\n");
    status = polynomial(&result, 2.0, 2, 1.0, -3.0, 2.0);
    print_polynomial("x^2 - 3x + 2 при x = 2", status, result);
    status = polynomial(&result, -1.5, 3, 2.0, 0.0, -1.0, 4.0);
    print_polynomial("2x^3 - x + 4 при x = -1.5", status, result);
    status = polynomial(&result, 0.5, 4, 1.0, 1.0, 1.0, 1.0, 1.0);
    print_polynomial("x^4 + x^3 + x^2 + x + 1 при x = 0.5", status, result);
    status = polynomial(&result, 10.0, 0, 7.0);
    print_polynomial("7 при x = 10", status, result);
    status = polynomial(&result, 1e200, 2, 1.0, 0.0, 0.0);
    print_polynomial("x^2 при x = 1e200", status, result);
    status = polynomial(&result, 1.0, 1, NAN, 1.0);
    print_polynomial("коэффициент NaN", status, result);
    status = polynomial(&result, 1.0, -1);
    print_polynomial("степень -1", status, result);
}

void print_kaprekar(const int base, const int count, const char *const numbers[], const int flags[],
                    const Status status, const int error_index) {
    int i;
    int found = 0;
    printf("  основание %d, строки:", base);
    for (i = 0; i < count; i++) {
        printf(" \"%s\"", numbers[i]);
    }
    printf("\n");
    if (status != STATUS_OK) {
        if (error_index >= 0) {
            printf("    ошибка в строке %d - %s\n", error_index + 1, status_message(status));
        } else {
            printf("    ошибка - %s\n", status_message(status));
        }
        return;
    }
    printf("    числа Капрекара:");
    for (i = 0; i < count; i++) {
        if (flags[i]) {
            printf(" %s", numbers[i]);
            found = 1;
        }
    }
    printf("%s\n", found ? "" : " нет");
}

void demo_kaprekar(void) {
    const char *decimal[] = {"0", "1", "9", "10", "45", "55", "100", "297", "703", "998", "999", "2223", "4879"};
    const char *hex[] = {"1", "6", "A", "B", "f", "33", "50", "FF", "15F", "FFFFFFFF"};
    const char *binary[] = {"1", "10", "11", "110", "111", "1000", "1010"};
    const char *wrong_digit[] = {"101", "123"};
    const char *too_big[] = {"4294967295", "4294967296"};
    const char *wrong_base[] = {"1"};
    int decimal_flags[13];
    int hex_flags[10];
    int binary_flags[7];
    int other_flags[2];
    int error_index;
    Status status;

    printf("3. Числа Капрекара\n");
    status = find_kaprekar(decimal_flags, 13, &error_index, 10, 13, decimal[0], decimal[1], decimal[2],
                           decimal[3], decimal[4], decimal[5], decimal[6], decimal[7], decimal[8], decimal[9],
                           decimal[10], decimal[11], decimal[12]);
    print_kaprekar(10, 13, decimal, decimal_flags, status, error_index);
    status = find_kaprekar(hex_flags, 10, &error_index, 16, 10, hex[0], hex[1], hex[2], hex[3], hex[4], hex[5],
                           hex[6], hex[7], hex[8], hex[9]);
    print_kaprekar(16, 10, hex, hex_flags, status, error_index);
    status = find_kaprekar(binary_flags, 7, &error_index, 2, 7, binary[0], binary[1], binary[2], binary[3],
                           binary[4], binary[5], binary[6]);
    print_kaprekar(2, 7, binary, binary_flags, status, error_index);
    status = find_kaprekar(other_flags, 2, &error_index, 2, 2, wrong_digit[0], wrong_digit[1]);
    print_kaprekar(2, 2, wrong_digit, other_flags, status, error_index);
    status = find_kaprekar(other_flags, 2, &error_index, 10, 2, too_big[0], too_big[1]);
    print_kaprekar(10, 2, too_big, other_flags, status, error_index);
    status = find_kaprekar(other_flags, 2, &error_index, 37, 1, wrong_base[0]);
    print_kaprekar(37, 1, wrong_base, other_flags, status, error_index);
}

void print_mean(const char *name, const Status status, const double result) {
    if (status == STATUS_OK) {
        printf("  %s: %g\n", name, result);
    } else {
        printf("  %s: ошибка - %s\n", name, status_message(status));
    }
}

void demo_geometric_mean(void) {
    const double eps = 1e-9;
    double result = 0.0;
    Status status;

    printf("4. Среднее геометрическое (eps = %g)\n", eps);
    status = geometric_mean(&result, eps, 3, 2.0, 8.0, 4.0);
    print_mean("2, 8, 4", status, result);
    status = geometric_mean(&result, eps, 4, 1.0, 3.0, 9.0, 27.0);
    print_mean("1, 3, 9, 27", status, result);
    status = geometric_mean(&result, eps, 1, 7.5);
    print_mean("7.5", status, result);
    status = geometric_mean(&result, eps, 3, 1e300, 1e300, 1e300);
    print_mean("1e300, 1e300, 1e300", status, result);
    status = geometric_mean(&result, eps, 3, 5.0, 0.0, 7.0);
    print_mean("5, 0, 7", status, result);
    status = geometric_mean(&result, eps, 2, 5.0, -5.0);
    print_mean("5, -5", status, result);
    status = geometric_mean(&result, eps, 2, 5.0, INFINITY);
    print_mean("5, бесконечность", status, result);
    status = geometric_mean(&result, eps, 0);
    print_mean("количество 0", status, result);
}

void print_power(const double base, const int exponent, const Status status, const double result) {
    if (status == STATUS_OK) {
        printf("  основание %g, степень %d: %g\n", base, exponent, result);
    } else {
        printf("  основание %g, степень %d: ошибка - %s\n", base, exponent, status_message(status));
    }
}

void demo_fast_pow(void) {
    const double eps = 1e-12;
    const double bases[] = {2.0, 2.0, -3.0, 1.5, 10.0, 0.0, 0.0, -1.0, 0.5, 0.0, 10.0, 10.0, 2.0};
    const int exponents[] = {10, -3, 5, 0, -2, 0, 5, INT_MIN, -1023, -1, 400, -400, -1074};
    double result = 0.0;
    Status status;
    int i;

    printf("5. Быстрое возведение в степень (eps = %g)\n", eps);
    for (i = 0; i < 13; i++) {
        status = fast_pow(&result, bases[i], exponents[i], eps);
        print_power(bases[i], exponents[i], status, result);
    }
    status = fast_pow(&result, NAN, 2, eps);
    if (status == STATUS_OK) {
        printf("  основание NaN, степень 2: %g\n", result);
    } else {
        printf("  основание NaN, степень 2: ошибка - %s\n", status_message(status));
    }
    status = fast_pow(&result, 2.0, 3, 0.0);
    if (status == STATUS_OK) {
        printf("  основание 2, степень 3, eps = 0: %g\n", result);
    } else {
        printf("  основание 2, степень 3, eps = 0: ошибка - %s\n", status_message(status));
    }
}

void demo_bisection(void) {
    double (*functions[])(double) = {square_minus_two, square_minus_two, square_minus_two, cos_minus_x, cubic,
                                     sine, sine, sine, x_plus_log, square_minus_two, square_minus_two,
                                     square_minus_two, square_minus_two, x_plus_log};
    const char *names[] = {"x^2 - 2", "x^2 - 2", "x^2 - 2", "cos(x) - x", "x^3 - x - 2", "sin(x)", "sin(x)",
                           "sin(x)", "x + ln|x|", "x^2 - 2", "x^2 - 2", "x^2 - 2", "x^2 - 2", "x + ln|x|"};
    const double left[] = {0.0, 0.0, -2.0, 0.0, 1.0, 3.0, -1.0, 0.0, 0.1, 2.0, 2.0, 0.0, 0.0, -1.0};
    const double right[] = {2.0, 2.0, 0.0, 1.0, 2.0, 4.0, 1.0, 1.0, 1.0, 3.0, 0.0, 2.0, 2.0, 1.0};
    const double epsilons[] = {1e-3, 1e-10, 1e-6, 1e-6, 1e-8, 1e-12, 1e-6, 1e-6, 1e-9, 1e-6, 1e-6, 0.0, 1e-20,
                               1e-6};
    double root = 0.0;
    Status status;
    int i;

    printf("6. Метод дихотомии\n");
    for (i = 0; i < 14; i++) {
        status = bisection(&root, functions[i], left[i], right[i], epsilons[i]);
        printf("  %s на [%g, %g], eps = %g: ", names[i], left[i], right[i], epsilons[i]);
        if (status == STATUS_OK) {
            printf("x = %.12f\n", root);
        } else if (status == STATUS_ITERATION_LIMIT) {
            printf("ошибка - %s, лучшее приближение x = %.12f\n", status_message(status), root);
        } else {
            printf("ошибка - %s\n", status_message(status));
        }
    }
}

int main(int argc, char *argv[]) {
    Status status;
    if (argc != 1 || argv == NULL) {
        printf("Ошибка: программа не принимает аргументов командной строки\n");
        printf("Использование: task6.exe\n");
        return 1;
    }
    status = demo_convex();
    demo_polynomial();
    demo_kaprekar();
    demo_geometric_mean();
    demo_fast_pow();
    demo_bisection();
    if (status != STATUS_OK) {
        return 1;
    }
    return 0;
}
