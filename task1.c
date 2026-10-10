// .\task1.exe 15 -h

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

typedef enum {
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_INVALID_NUMBER,
    STATUS_OUT_OF_RANGE,
    STATUS_OVERFLOW,
    STATUS_INVALID_ARGUMENT,
    STATUS_BUFFER_TOO_SMALL
} Status;

Status split_integer(const char *str, int *is_negative, const char **digits) {
    const char *p;
    const char *q;
    if (str == NULL || is_negative == NULL || digits == NULL) {
        return STATUS_NULL_POINTER;
    }
    p = str;
    if (*p == '-' || *p == '+') {
        p++;
    }
    if (*p == '\0') {
        return STATUS_INVALID_NUMBER;
    }
    for (q = p; *q != '\0'; q++) {
        if (!isdigit((unsigned char)*q)) {
            return STATUS_INVALID_NUMBER;
        }
    }
    while (*p == '0' && p[1] != '\0') {
        p++;
    }
    *is_negative = str[0] == '-' && *p != '0';
    *digits = p;
    return STATUS_OK;
}

Status parse_long(const char *str, const long min_value, const long max_value, long *result) {
    const char *p;
    long value = 0;
    int negative;
    int digit;
    Status status;
    if (str == NULL || result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (min_value > 0 || max_value < 0) {
        return STATUS_INVALID_ARGUMENT;
    }
    status = split_integer(str, &negative, &p);
    if (status != STATUS_OK) {
        return status;
    }
    for (; *p != '\0'; p++) {
        digit = *p - '0';
        if (negative) {
            if (value < min_value / 10 || value * 10 < min_value + digit) {
                return STATUS_OVERFLOW;
            }
            value = value * 10 - digit;
        } else {
            if (value > max_value / 10 || value * 10 > max_value - digit) {
                return STATUS_OVERFLOW;
            }
            value = value * 10 + digit;
        }
    }
    *result = value;
    return STATUS_OK;
}

int is_valid_flag(const char *flag) {
    if (flag == NULL) {
        return 0;
    }
    if ((flag[0] != '-' && flag[0] != '/') || flag[1] == '\0' || flag[2] != '\0') {
        return 0;
    }
    return flag[1] == 'h' || flag[1] == 'p' || flag[1] == 's' ||
           flag[1] == 'e' || flag[1] == 'a' || flag[1] == 'f';
}

Status find_multiples(const long x, const long limit, long multiples[], const int capacity, int *count) {
    long step;
    long total;
    long i;
    if (multiples == NULL || count == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (limit < 1 || capacity < 0) {
        return STATUS_INVALID_ARGUMENT;
    }
    *count = 0;
    if (x == 0 || x < -limit || x > limit) {
        return STATUS_OK;
    }
    step = x < 0 ? -x : x;
    total = limit / step;
    if (total > capacity) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    for (i = 1; i <= total; i++) {
        multiples[i - 1] = i * step;
    }
    *count = (int)total;
    return STATUS_OK;
}

Status check_prime(const long x, int *is_prime, int *is_composite) {
    long i;
    if (is_prime == NULL || is_composite == NULL) {
        return STATUS_NULL_POINTER;
    }
    *is_prime = 0;
    *is_composite = 0;
    if (x < 2) {
        return STATUS_OK;
    }
    for (i = 2; i <= x / i; i++) {
        if (x % i == 0) {
            *is_composite = 1;
            return STATUS_OK;
        }
    }
    *is_prime = 1;
    return STATUS_OK;
}

Status split_hex(const long x, char digits[], const int capacity, int *count, int *is_negative) {
    const char alphabet[] = "0123456789ABCDEF";
    unsigned long value;
    int i;
    char tmp;
    if (digits == NULL || count == NULL || is_negative == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (capacity < 1) {
        return STATUS_INVALID_ARGUMENT;
    }
    *is_negative = x < 0;
    value = x < 0 ? 0UL - (unsigned long)x : (unsigned long)x;
    *count = 0;
    do {
        if (*count >= capacity) {
            return STATUS_BUFFER_TOO_SMALL;
        }
        digits[*count] = alphabet[value % 16];
        (*count)++;
        value /= 16;
    } while (value > 0);
    for (i = 0; i < *count / 2; i++) {
        tmp = digits[i];
        digits[i] = digits[*count - 1 - i];
        digits[*count - 1 - i] = tmp;
    }
    return STATUS_OK;
}

Status power_table(const long x, const int max_power, long long table[10][10]) {
    int base;
    int exponent;
    if (table == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (max_power < 1 || max_power > 10) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (x < 1 || x > max_power) {
        return STATUS_OUT_OF_RANGE;
    }
    for (base = 1; base <= max_power; base++) {
        table[base - 1][0] = base;
        for (exponent = 2; exponent <= x; exponent++) {
            table[base - 1][exponent - 1] = table[base - 1][exponent - 2] * base;
        }
    }
    return STATUS_OK;
}

Status sum_to(const long x, unsigned long long *sum) {
    unsigned long long a;
    unsigned long long b;
    if (sum == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (x < 1) {
        return STATUS_OUT_OF_RANGE;
    }
    a = (unsigned long long)x;
    b = a + 1;
    if (a % 2 == 0) {
        a /= 2;
    } else {
        b /= 2;
    }
    if (a > ULLONG_MAX / b) {
        return STATUS_OVERFLOW;
    }
    *sum = a * b;
    return STATUS_OK;
}

Status factorial(const long x, unsigned long long *result) {
    unsigned long long value = 1;
    long i;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (x < 0) {
        return STATUS_OUT_OF_RANGE;
    }
    for (i = 2; i <= x; i++) {
        if (value > ULLONG_MAX / (unsigned long long)i) {
            return STATUS_OVERFLOW;
        }
        value *= (unsigned long long)i;
    }
    *result = value;
    return STATUS_OK;
}

void print_status(const Status status) {
    switch (status) {
        case STATUS_NULL_POINTER:
            printf("Ошибка: передан нулевой указатель\n");
            break;
        case STATUS_INVALID_NUMBER:
            printf("Ошибка: x должен быть целым числом\n");
            break;
        case STATUS_OUT_OF_RANGE:
            printf("Ошибка: значение x недопустимо для этого флага\n");
            break;
        case STATUS_OVERFLOW:
            printf("Ошибка: переполнение\n");
            break;
        case STATUS_INVALID_ARGUMENT:
            printf("Ошибка: функции передан некорректный параметр\n");
            break;
        case STATUS_BUFFER_TOO_SMALL:
            printf("Ошибка: недостаточный размер буфера\n");
            break;
        default:
            break;
    }
}

void print_usage(void) {
    printf("Использование: task1.exe <x> <флаг>\n");
    printf("Флаги: -h -p -s -e -a -f (или /h /p /s /e /a /f)\n");
}

void print_range_error(const long min_value, const long max_value) {
    printf("Ошибка: x выходит за допустимый диапазон [%ld, %ld]\n", min_value, max_value);
}

int main(int argc, char *argv[]) {
    const long max_multiple = 100;
    const int max_power = 10;
    const long min_x = -2147483647L - 1;
    const long max_x = 2147483647L;
    long x = 0;
    int in_range;
    int is_negative;
    const char *digits;
    const char *sign;
    Status status;
    const char *flag;

    if (argc != 3) {
        printf("Ошибка: неверное количество аргументов\n");
        print_usage();
        return 1;
    }

    flag = argv[2];
    if (!is_valid_flag(flag)) {
        printf("Ошибка: некорректный флаг \"%s\"\n", flag);
        print_usage();
        return 1;
    }

    status = parse_long(argv[1], min_x, max_x, &x);
    if (status == STATUS_INVALID_NUMBER) {
        printf("Ошибка: x должен быть целым числом, получено \"%s\"\n", argv[1]);
        return 1;
    }
    if (status != STATUS_OK && status != STATUS_OVERFLOW) {
        print_status(status);
        return 1;
    }
    in_range = status == STATUS_OK;
    status = split_integer(argv[1], &is_negative, &digits);
    if (status != STATUS_OK) {
        print_status(status);
        return 1;
    }
    sign = is_negative ? "-" : "";

    switch (flag[1]) {
        case 'h': {
            long multiples[100];
            int count = 0;
            int i;
            if (in_range) {
                status = find_multiples(x, max_multiple, multiples, (int)(sizeof(multiples) / sizeof(multiples[0])), &count);
                if (status != STATUS_OK) {
                    print_status(status);
                    return 1;
                }
            }
            if (count == 0) {
                printf("Нет натуральных чисел до %ld включительно, кратных %s%s\n", max_multiple, sign, digits);
            } else {
                printf("Натуральные числа до %ld включительно, кратные %s%s:", max_multiple, sign, digits);
                for (i = 0; i < count; i++) {
                    printf(" %ld", multiples[i]);
                }
                printf("\n");
            }
            break;
        }
        case 'p': {
            int is_prime = 0;
            int is_composite = 0;
            if (in_range) {
                status = check_prime(x, &is_prime, &is_composite);
                if (status != STATUS_OK) {
                    print_status(status);
                    return 1;
                }
            } else if (!is_negative) {
                print_range_error(min_x, max_x);
                return 1;
            }
            if (is_prime) {
                printf("%s%s - простое число\n", sign, digits);
            } else if (is_composite) {
                printf("%s%s - составное число\n", sign, digits);
            } else {
                printf("%s%s - не является ни простым, ни составным (простыми и составными бывают только натуральные числа больше 1)\n", sign, digits);
            }
            break;
        }
        case 's': {
            char hex_digits[16];
            int count;
            int hex_negative;
            int i;
            if (!in_range) {
                print_range_error(min_x, max_x);
                return 1;
            }
            status = split_hex(x, hex_digits, (int)sizeof(hex_digits), &count, &hex_negative);
            if (status != STATUS_OK) {
                print_status(status);
                return 1;
            }
            if (hex_negative) {
                printf("- ");
            }
            for (i = 0; i < count; i++) {
                if (i > 0) {
                    printf(" ");
                }
                printf("%c", hex_digits[i]);
            }
            printf("\n");
            break;
        }
        case 'e': {
            long long table[10][10];
            int base;
            int exponent;
            if (in_range) {
                status = power_table(x, max_power, table);
            } else {
                status = STATUS_OUT_OF_RANGE;
            }
            if (status == STATUS_OUT_OF_RANGE) {
                printf("Ошибка: для флага -e x должен быть от 1 до %d\n", max_power);
                return 1;
            }
            if (status != STATUS_OK) {
                print_status(status);
                return 1;
            }
            printf("  осн.");
            for (exponent = 1; exponent <= x; exponent++) {
                printf("%12d", exponent);
            }
            printf("\n");
            for (base = 1; base <= max_power; base++) {
                printf("%6d", base);
                for (exponent = 1; exponent <= x; exponent++) {
                    printf("%12lld", table[base - 1][exponent - 1]);
                }
                printf("\n");
            }
            break;
        }
        case 'a': {
            unsigned long long sum;
            if (in_range) {
                status = sum_to(x, &sum);
            } else if (is_negative) {
                status = STATUS_OUT_OF_RANGE;
            } else {
                print_range_error(min_x, max_x);
                return 1;
            }
            if (status == STATUS_OUT_OF_RANGE) {
                printf("Ошибка: для флага -a x должен быть натуральным числом (x >= 1)\n");
                return 1;
            }
            if (status == STATUS_OVERFLOW) {
                printf("Ошибка: для флага -a сумма не помещается в unsigned long long\n");
                return 1;
            }
            if (status != STATUS_OK) {
                print_status(status);
                return 1;
            }
            printf("Сумма чисел от 1 до %ld = %llu\n", x, sum);
            break;
        }
        case 'f': {
            unsigned long long result;
            if (in_range) {
                status = factorial(x, &result);
            } else if (is_negative) {
                status = STATUS_OUT_OF_RANGE;
            } else {
                status = STATUS_OVERFLOW;
            }
            if (status == STATUS_OUT_OF_RANGE) {
                printf("Ошибка: для флага -f факториал определён только для x >= 0\n");
                return 1;
            }
            if (status == STATUS_OVERFLOW) {
                printf("Ошибка: для флага -f x! не помещается в unsigned long long (максимум x = 20)\n");
                return 1;
            }
            if (status != STATUS_OK) {
                print_status(status);
                return 1;
            }
            printf("%ld! = %llu\n", x, result);
            break;
        }
        default:
            printf("Ошибка: некорректный флаг \"%s\"\n", flag);
            print_usage();
            return 1;
    }

    return 0;
}
