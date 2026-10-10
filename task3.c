// .\task3.exe -q 0.0001 1 -3 2
// .\task3.exe -m 10 5
// .\task3.exe -t 0.0001 3 4 5

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <limits.h>

typedef enum {
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_INVALID_NUMBER,
    STATUS_OUT_OF_RANGE,
    STATUS_INVALID_EPSILON,
    STATUS_INVALID_VALUE,
    STATUS_OVERFLOW,
    STATUS_PRECISION_LOSS
} Status;

typedef enum {
    ROOTS_NONE,
    ROOTS_ONE,
    ROOTS_TWO,
    ROOTS_INFINITE
} RootsType;

typedef struct {
    double a;
    double b;
    double c;
    Status status;
    RootsType type;
    double x1;
    double x2;
} Equation;

int is_digit_char(const char c) {
    return c >= '0' && c <= '9';
}

int is_valid_epsilon(const double eps) {
    return isfinite(eps) && eps > 0.0;
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

Status parse_long(const char *str, long *result) {
    const char *p;
    const char *digits_start;
    long value = 0;
    int negative = 0;
    int digit;
    if (str == NULL || result == NULL) {
        return STATUS_NULL_POINTER;
    }
    p = str;
    if (*p == '-' || *p == '+') {
        negative = *p == '-';
        p++;
    }
    digits_start = p;
    if (*p == '\0') {
        return STATUS_INVALID_NUMBER;
    }
    for (; *p != '\0'; p++) {
        if (!is_digit_char(*p)) {
            return STATUS_INVALID_NUMBER;
        }
    }
    for (p = digits_start; *p != '\0'; p++) {
        digit = *p - '0';
        if (negative) {
            if (value < (LONG_MIN + digit) / 10) {
                return STATUS_OUT_OF_RANGE;
            }
            value = value * 10 - digit;
        } else {
            if (value > (LONG_MAX - digit) / 10) {
                return STATUS_OUT_OF_RANGE;
            }
            value = value * 10 + digit;
        }
    }
    *result = value;
    return STATUS_OK;
}

Status parse_numbers(char *const strs[], const int count, double numbers[], int *bad_index) {
    int i;
    Status status;
    if (strs == NULL || numbers == NULL || bad_index == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (count < 0) {
        return STATUS_INVALID_VALUE;
    }
    for (i = 0; i < count; i++) {
        status = parse_double(strs[i], &numbers[i]);
        if (status != STATUS_OK) {
            *bad_index = i;
            return status;
        }
    }
    return STATUS_OK;
}

Status solve_quadratic(const double a, const double b, const double c, const double eps,
                       RootsType *type, double *x1, double *x2) {
    double bb;
    double ac4;
    double d;
    double noise;
    double q;
    double root1;
    double root2;
    if (type == NULL || x1 == NULL || x2 == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (!is_valid_epsilon(eps)) {
        return STATUS_INVALID_EPSILON;
    }
    if (!isfinite(a) || !isfinite(b) || !isfinite(c)) {
        return STATUS_INVALID_VALUE;
    }
    if (fabs(a) < eps) {
        if (fabs(b) < eps) {
            *type = fabs(c) < eps ? ROOTS_INFINITE : ROOTS_NONE;
            return STATUS_OK;
        }
        root1 = -c / b;
        if (!isfinite(root1)) {
            return STATUS_OVERFLOW;
        }
        *type = ROOTS_ONE;
        *x1 = root1 + 0.0;
        return STATUS_OK;
    }
    bb = b * b;
    ac4 = 4.0 * a * c;
    d = bb - ac4;
    if (!isfinite(bb) || !isfinite(ac4) || !isfinite(d)) {
        return STATUS_OVERFLOW;
    }
    if (fabs(d) < eps) {
        root1 = -b / 2.0 / a;
        if (!isfinite(root1)) {
            return STATUS_OVERFLOW;
        }
        *type = ROOTS_ONE;
        *x1 = root1 + 0.0;
        return STATUS_OK;
    }
    noise = 4.0 * DBL_EPSILON * bb + 4.0 * DBL_EPSILON * fabs(ac4);
    if (fabs(d) <= noise) {
        return STATUS_PRECISION_LOSS;
    }
    if (d < 0.0) {
        *type = ROOTS_NONE;
        return STATUS_OK;
    }
    if (b < 0.0) {
        q = (-b + sqrt(d)) / 2.0;
        root1 = q / a;
        root2 = c / q;
    } else {
        q = (-b - sqrt(d)) / 2.0;
        root1 = c / q;
        root2 = q / a;
    }
    if (!isfinite(root1) || !isfinite(root2)) {
        return STATUS_OVERFLOW;
    }
    *type = ROOTS_TWO;
    *x1 = root1 + 0.0;
    *x2 = root2 + 0.0;
    return STATUS_OK;
}

int is_same_coefficients(const double first[3], const double second[3], const double eps) {
    if (first == NULL || second == NULL) {
        return 0;
    }
    return fabs(first[0] - second[0]) < eps && fabs(first[1] - second[1]) < eps &&
           fabs(first[2] - second[2]) < eps;
}

Status solve_permutations(const double coefficients[3], const double eps, Equation equations[6], int *count) {
    const int orders[6][3] = {
        {0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}
    };
    double current[3];
    double previous[3];
    int i;
    int j;
    int is_unique;
    if (coefficients == NULL || equations == NULL || count == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (!is_valid_epsilon(eps)) {
        return STATUS_INVALID_EPSILON;
    }
    *count = 0;
    for (i = 0; i < 6; i++) {
        current[0] = coefficients[orders[i][0]];
        current[1] = coefficients[orders[i][1]];
        current[2] = coefficients[orders[i][2]];
        is_unique = 1;
        for (j = 0; j < *count; j++) {
            previous[0] = equations[j].a;
            previous[1] = equations[j].b;
            previous[2] = equations[j].c;
            if (is_same_coefficients(current, previous, eps)) {
                is_unique = 0;
                break;
            }
        }
        if (!is_unique) {
            continue;
        }
        equations[*count].a = current[0];
        equations[*count].b = current[1];
        equations[*count].c = current[2];
        equations[*count].status = solve_quadratic(current[0], current[1], current[2], eps,
                                                   &equations[*count].type, &equations[*count].x1,
                                                   &equations[*count].x2);
        (*count)++;
    }
    return STATUS_OK;
}

Status is_multiple(const long a, const long b, int *result) {
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (a == 0 || b == 0) {
        return STATUS_INVALID_VALUE;
    }
    if (b == -1) {
        *result = 1;
    } else {
        *result = a % b == 0;
    }
    return STATUS_OK;
}

Status is_right_triangle(const double sides[3], const double eps, int *result) {
    double a;
    double b;
    double c;
    double tmp;
    double hypotenuse;
    double difference;
    if (sides == NULL || result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (!is_valid_epsilon(eps)) {
        return STATUS_INVALID_EPSILON;
    }
    a = sides[0];
    b = sides[1];
    c = sides[2];
    if (!isfinite(a) || !isfinite(b) || !isfinite(c)) {
        return STATUS_INVALID_VALUE;
    }
    if (a < eps || b < eps || c < eps) {
        *result = 0;
        return STATUS_OK;
    }
    if (a > c) {
        tmp = a;
        a = c;
        c = tmp;
    }
    if (b > c) {
        tmp = b;
        b = c;
        c = tmp;
    }
    hypotenuse = hypot(a, b);
    if (!isfinite(hypotenuse)) {
        return STATUS_OVERFLOW;
    }
    difference = fabs(hypotenuse - c);
    if (difference < eps) {
        *result = 1;
        return STATUS_OK;
    }
    if (difference <= 4.0 * DBL_EPSILON * c) {
        return STATUS_PRECISION_LOSS;
    }
    *result = 0;
    return STATUS_OK;
}

void print_usage(void) {
    printf("Использование:\n");
    printf("  task3.exe -q <эпсилон> <a> <b> <c>\n");
    printf("  task3.exe -m <a> <b>\n");
    printf("  task3.exe -t <эпсилон> <a> <b> <c>\n");
    printf("Флаг может начинаться с '-' или '/'\n");
}

void print_real_error(const Status status, const char *name, const char *text) {
    if (name == NULL || text == NULL) {
        printf("Ошибка при чтении параметров\n");
        return;
    }
    if (status == STATUS_INVALID_NUMBER) {
        printf("Ошибка: %s = \"%s\" не является вещественным числом\n", name, text);
    } else if (status == STATUS_OUT_OF_RANGE) {
        printf("Ошибка: %s = \"%s\" выходит за допустимый диапазон типа double (модуль от %g до %g или 0)\n",
               name, text, DBL_MIN, DBL_MAX);
    } else {
        printf("Ошибка при чтении параметра %s\n", name);
    }
}

void print_integer_error(const Status status, const char *name, const char *text) {
    if (name == NULL || text == NULL) {
        printf("Ошибка при чтении параметров\n");
        return;
    }
    if (status == STATUS_INVALID_NUMBER) {
        printf("Ошибка: %s = \"%s\" не является целым числом\n", name, text);
    } else if (status == STATUS_OUT_OF_RANGE) {
        printf("Ошибка: %s = \"%s\" выходит за допустимый диапазон [%ld, %ld]\n", name, text, LONG_MIN, LONG_MAX);
    } else {
        printf("Ошибка при чтении параметра %s\n", name);
    }
}

void print_equation(const Equation *equation) {
    if (equation == NULL) {
        return;
    }
    printf("a = %.15g, b = %.15g, c = %.15g: ", equation->a, equation->b, equation->c);
    switch (equation->status) {
        case STATUS_OK:
            break;
        case STATUS_OVERFLOW:
            printf("ошибка: переполнение типа double (дискриминант или корни слишком велики по модулю)\n");
            return;
        case STATUS_PRECISION_LOSS:
            printf("ошибка: точности типа double не хватает, чтобы сравнить дискриминант с нулём при таком эпсилон\n");
            return;
        default:
            printf("ошибка при решении уравнения\n");
            return;
    }
    switch (equation->type) {
        case ROOTS_NONE:
            printf("нет действительных корней\n");
            break;
        case ROOTS_ONE:
            printf("x = %.15g\n", equation->x1);
            break;
        case ROOTS_TWO:
            printf("x1 = %.15g, x2 = %.15g\n", equation->x1, equation->x2);
            break;
        case ROOTS_INFINITE:
            printf("любое x является решением\n");
            break;
    }
}

int main(int argc, char *argv[]) {
    const char *const names[4] = {"эпсилон", "a", "b", "c"};
    const char *flag;
    double numbers[4];
    int bad_index = 0;
    Status status;

    if (argc < 2) {
        print_usage();
        return 1;
    }

    flag = argv[1];
    if ((flag[0] != '-' && flag[0] != '/') || flag[1] == '\0' || flag[2] != '\0') {
        printf("Ошибка: некорректный флаг \"%s\"\n", flag);
        print_usage();
        return 1;
    }

    switch (flag[1]) {
        case 'q': {
            Equation equations[6];
            int count;
            int i;
            int has_errors = 0;
            if (argc != 6) {
                printf("Ошибка: для флага -q нужно 4 числа: эпсилон и три коэффициента (передано %d)\n", argc - 2);
                return 1;
            }
            status = parse_numbers(argv + 2, 4, numbers, &bad_index);
            if (status != STATUS_OK) {
                print_real_error(status, names[bad_index], argv[2 + bad_index]);
                return 1;
            }
            if (numbers[0] <= 0.0) {
                printf("Ошибка: эпсилон должен быть положительным\n");
                return 1;
            }
            status = solve_permutations(numbers + 1, numbers[0], equations, &count);
            if (status != STATUS_OK) {
                printf("Ошибка при решении уравнений\n");
                return 1;
            }
            for (i = 0; i < count; i++) {
                print_equation(&equations[i]);
                if (equations[i].status != STATUS_OK) {
                    has_errors = 1;
                }
            }
            if (has_errors) {
                return 1;
            }
            break;
        }
        case 'm': {
            long a;
            long b;
            int result;
            if (argc != 4) {
                printf("Ошибка: для флага -m нужно 2 целых числа (передано %d)\n", argc - 2);
                return 1;
            }
            status = parse_long(argv[2], &a);
            if (status != STATUS_OK) {
                print_integer_error(status, "a", argv[2]);
                return 1;
            }
            status = parse_long(argv[3], &b);
            if (status != STATUS_OK) {
                print_integer_error(status, "b", argv[3]);
                return 1;
            }
            status = is_multiple(a, b, &result);
            if (status == STATUS_INVALID_VALUE) {
                printf("Ошибка: оба числа должны быть ненулевыми (a = %ld, b = %ld)\n", a, b);
                return 1;
            } else if (status != STATUS_OK) {
                printf("Ошибка при проверке кратности\n");
                return 1;
            }
            if (result) {
                printf("%ld кратно %ld\n", a, b);
            } else {
                printf("%ld не кратно %ld\n", a, b);
            }
            break;
        }
        case 't': {
            int result;
            if (argc != 6) {
                printf("Ошибка: для флага -t нужно 4 числа: эпсилон и три стороны (передано %d)\n", argc - 2);
                return 1;
            }
            status = parse_numbers(argv + 2, 4, numbers, &bad_index);
            if (status != STATUS_OK) {
                print_real_error(status, names[bad_index], argv[2 + bad_index]);
                return 1;
            }
            if (numbers[0] <= 0.0) {
                printf("Ошибка: эпсилон должен быть положительным\n");
                return 1;
            }
            status = is_right_triangle(numbers + 1, numbers[0], &result);
            if (status == STATUS_OVERFLOW) {
                printf("Ошибка: переполнение типа double при вычислении гипотенузы\n");
                return 1;
            } else if (status == STATUS_PRECISION_LOSS) {
                printf("Ошибка: точности типа double не хватает, чтобы сравнить стороны с эпсилон %g; увеличьте эпсилон\n",
                       numbers[0]);
                return 1;
            } else if (status != STATUS_OK) {
                printf("Ошибка при проверке треугольника\n");
                return 1;
            }
            if (result) {
                printf("Числа могут быть сторонами прямоугольного треугольника\n");
            } else {
                printf("Числа не могут быть сторонами прямоугольного треугольника\n");
            }
            break;
        }
        default:
            printf("Ошибка: некорректный флаг \"%s\"\n", flag);
            print_usage();
            return 1;
    }
    return 0;
}
