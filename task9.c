// drmemory -suppress mingw.supp -- .\task9.exe -10 10
// .\task9.exe -10 10


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>

typedef enum {
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_INVALID_NUMBER,
    STATUS_OUT_OF_RANGE,
    STATUS_INVALID_RANGE,
    STATUS_INVALID_SIZE,
    STATUS_OVERFLOW,
    STATUS_MEMORY_ERROR
} Status;

Status parse_int(const char *str, int *result) {
    const char *digits;
    const char *p;
    int value = 0;
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
    if (*p == '\0') {
        return STATUS_INVALID_NUMBER;
    }
    digits = p;
    for (; *p != '\0'; p++) {
        if (!isdigit((unsigned char)*p)) {
            return STATUS_INVALID_NUMBER;
        }
    }
    for (p = digits; *p != '\0'; p++) {
        digit = *p - '0';
        if (negative) {
            if (value < (INT_MIN + digit) / 10) {
                return STATUS_OUT_OF_RANGE;
            }
            value = value * 10 - digit;
        } else {
            if (value > (INT_MAX - digit) / 10) {
                return STATUS_OUT_OF_RANGE;
            }
            value = value * 10 + digit;
        }
    }
    *result = value;
    return STATUS_OK;
}

Status random_in_range(const int a, const int b, int *result) {
    unsigned long long range;
    unsigned long long random_value = 0;
    int i;
    if (result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (a > b) {
        return STATUS_INVALID_RANGE;
    }
    range = (unsigned long long)((long long)b - (long long)a) + 1ULL;
    for (i = 0; i < 4; i++) {
        random_value = random_value * ((unsigned long long)RAND_MAX + 1ULL) + (unsigned long long)rand();
    }
    *result = (int)((long long)a + (long long)(random_value % range));
    return STATUS_OK;
}

Status fill_random(int array[], const size_t size, const int a, const int b) {
    size_t i;
    Status status;
    if (array == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (a > b) {
        return STATUS_INVALID_RANGE;
    }
    for (i = 0; i < size; i++) {
        status = random_in_range(a, b, &array[i]);
        if (status != STATUS_OK) {
            return status;
        }
    }
    return STATUS_OK;
}

Status swap_min_max(int array[], const size_t size, int *min, int *max) {
    size_t i;
    size_t min_index = 0;
    size_t max_index = 0;
    int tmp;
    if (array == NULL || min == NULL || max == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (size == 0) {
        return STATUS_INVALID_SIZE;
    }
    for (i = 1; i < size; i++) {
        if (array[i] < array[min_index]) {
            min_index = i;
        }
        if (array[i] > array[max_index]) {
            max_index = i;
        }
    }
    *min = array[min_index];
    *max = array[max_index];
    tmp = array[min_index];
    array[min_index] = array[max_index];
    array[max_index] = tmp;
    return STATUS_OK;
}

int compare_ints(const void *first, const void *second) {
    const int a = *(const int *)first;
    const int b = *(const int *)second;
    return (a > b) - (a < b);
}

Status find_closest(const int sorted[], const size_t size, const int value, int *closest) {
    size_t left = 0;
    size_t right;
    size_t mid;
    if (sorted == NULL || closest == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (size == 0) {
        return STATUS_INVALID_SIZE;
    }
    right = size - 1;
    while (left < right) {
        mid = left + (right - left) / 2;
        if (sorted[mid] < value) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }
    *closest = sorted[left];
    if (left > 0 && llabs((long long)value - sorted[left - 1]) < llabs((long long)value - sorted[left])) {
        *closest = sorted[left - 1];
    }
    return STATUS_OK;
}

Status add_ints(const int x, const int y, int *sum) {
    if (sum == NULL) {
        return STATUS_NULL_POINTER;
    }
    if ((y > 0 && x > INT_MAX - y) || (y < 0 && x < INT_MIN - y)) {
        return STATUS_OVERFLOW;
    }
    *sum = x + y;
    return STATUS_OK;
}

Status build_array_c(const int a[], const size_t size_a, const int b[], const size_t size_b, int **c) {
    int *sorted_b;
    int *result;
    int closest;
    size_t i;
    Status status;
    if (a == NULL || b == NULL || c == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (size_a == 0 || size_b == 0 || size_a > SIZE_MAX / sizeof(int) || size_b > SIZE_MAX / sizeof(int)) {
        return STATUS_INVALID_SIZE;
    }
    sorted_b = (int *)malloc(size_b * sizeof(int));
    if (sorted_b == NULL) {
        return STATUS_MEMORY_ERROR;
    }
    result = (int *)malloc(size_a * sizeof(int));
    if (result == NULL) {
        free(sorted_b);
        return STATUS_MEMORY_ERROR;
    }
    for (i = 0; i < size_b; i++) {
        sorted_b[i] = b[i];
    }
    qsort(sorted_b, size_b, sizeof(int), compare_ints);
    for (i = 0; i < size_a; i++) {
        status = find_closest(sorted_b, size_b, a[i], &closest);
        if (status == STATUS_OK) {
            status = add_ints(a[i], closest, &result[i]);
        }
        if (status != STATUS_OK) {
            free(sorted_b);
            free(result);
            return status;
        }
    }
    free(sorted_b);
    *c = result;
    return STATUS_OK;
}

void print_array(const char *name, const int array[], const size_t size, const size_t limit) {
    size_t i;
    if (name == NULL || array == NULL) {
        return;
    }
    printf("%s (размер %lu): ", name, (unsigned long)size);
    for (i = 0; i < size && i < limit; i++) {
        printf("%d ", array[i]);
    }
    if (size > limit) {
        printf("...");
    }
    printf("\n");
}

void print_parse_error(const char *name, const Status status) {
    if (name == NULL) {
        return;
    }
    if (status == STATUS_OUT_OF_RANGE) {
        printf("Ошибка: %s выходит за допустимый диапазон [%d, %d]\n", name, INT_MIN, INT_MAX);
    } else {
        printf("Ошибка: %s должно быть целым числом (например, -10 или 25)\n", name);
    }
}

void print_usage(void) {
    printf("Использование: task9.exe <a> <b>\n");
    printf("a и b - целые числа из диапазона [%d, %d], a <= b\n", INT_MIN, INT_MAX);
}

int main(int argc, char *argv[]) {
    const int min_size = 10;
    const int max_size = 10000;
    const int min_value = -1000;
    const int max_value = 1000;
    const size_t print_limit = 20;
    int fixed[20];
    const size_t fixed_size = sizeof(fixed) / sizeof(fixed[0]);
    int *a = NULL;
    int *b = NULL;
    int *c = NULL;
    int left;
    int right;
    int min;
    int max;
    int size_a;
    int size_b;
    Status status;

    if (argc != 3) {
        printf("Ошибка: неверное количество аргументов (нужно 2: a и b)\n");
        print_usage();
        return 1;
    }
    status = parse_int(argv[1], &left);
    if (status != STATUS_OK) {
        print_parse_error("a", status);
        return 1;
    }
    status = parse_int(argv[2], &right);
    if (status != STATUS_OK) {
        print_parse_error("b", status);
        return 1;
    }
    if (left > right) {
        printf("Ошибка: a не должно быть больше b\n");
        return 1;
    }

    srand((unsigned int)time(NULL));

    printf("Задание 1\n");
    status = fill_random(fixed, fixed_size, left, right);
    if (status != STATUS_OK) {
        printf("Ошибка: не удалось заполнить массив\n");
        return 1;
    }
    print_array("Исходный массив", fixed, fixed_size, print_limit);
    status = swap_min_max(fixed, fixed_size, &min, &max);
    if (status != STATUS_OK) {
        printf("Ошибка: не удалось найти минимум и максимум\n");
        return 1;
    }
    printf("Минимум: %d, максимум: %d\n", min, max);
    print_array("После обмена", fixed, fixed_size, print_limit);

    printf("\nЗадание 2\n");
    if (random_in_range(min_size, max_size, &size_a) != STATUS_OK ||
        random_in_range(min_size, max_size, &size_b) != STATUS_OK) {
        printf("Ошибка: не удалось сгенерировать размеры массивов\n");
        return 1;
    }
    a = (int *)malloc((size_t)size_a * sizeof(int));
    b = (int *)malloc((size_t)size_b * sizeof(int));
    if (a == NULL || b == NULL) {
        printf("Ошибка: не удалось выделить память для массивов A и B\n");
        free(a);
        free(b);
        return 1;
    }
    if (fill_random(a, (size_t)size_a, min_value, max_value) != STATUS_OK ||
        fill_random(b, (size_t)size_b, min_value, max_value) != STATUS_OK) {
        printf("Ошибка: не удалось заполнить массивы\n");
        free(a);
        free(b);
        return 1;
    }

    status = build_array_c(a, (size_t)size_a, b, (size_t)size_b, &c);
    if (status != STATUS_OK) {
        if (status == STATUS_MEMORY_ERROR) {
            printf("Ошибка: не удалось выделить память для массива C\n");
        } else if (status == STATUS_OVERFLOW) {
            printf("Ошибка: сумма элементов выходит за диапазон int\n");
        } else {
            printf("Ошибка: не удалось построить массив C\n");
        }
        free(a);
        free(b);
        return 1;
    }
    print_array("A", a, (size_t)size_a, print_limit);
    print_array("B", b, (size_t)size_b, print_limit);
    print_array("C", c, (size_t)size_a, print_limit);

    free(a);
    free(b);
    free(c);
    return 0;
}
