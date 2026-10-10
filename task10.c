// drmemory -suppress mingw.supp -- .\task10.exe

// .\task10.exe
// 16
// 1A -FF 0010 7FFF
// Stop


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>

typedef enum {
    STATUS_OK,
    STATUS_END_OF_FILE,
    STATUS_NULL_POINTER,
    STATUS_INVALID_ARGUMENT,
    STATUS_INVALID_BASE,
    STATUS_INVALID_NUMBER,
    STATUS_LOWERCASE_DIGIT,
    STATUS_OVERFLOW,
    STATUS_SUM_OVERFLOW,
    STATUS_MEMORY_ERROR,
    STATUS_READ_ERROR
} Status;

Status read_token(FILE *file, char **buffer, size_t *capacity, size_t *length) {
    const size_t start_capacity = 16;
    char *new_buffer;
    int c;
    if (file == NULL || buffer == NULL || capacity == NULL || length == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (*buffer != NULL && *capacity == 0) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (*buffer == NULL) {
        *buffer = (char *)malloc(start_capacity);
        if (*buffer == NULL) {
            return STATUS_MEMORY_ERROR;
        }
        *capacity = start_capacity;
    }
    *length = 0;
    (*buffer)[0] = '\0';
    if (feof(file)) {
        return STATUS_END_OF_FILE;
    }
    do {
        c = fgetc(file);
    } while (c != EOF && isspace(c));
    while (c != EOF && !isspace(c)) {
        if (*length + 1 >= *capacity) {
            if (*capacity > SIZE_MAX / 2) {
                return STATUS_MEMORY_ERROR;
            }
            new_buffer = (char *)realloc(*buffer, *capacity * 2);
            if (new_buffer == NULL) {
                return STATUS_MEMORY_ERROR;
            }
            *buffer = new_buffer;
            *capacity *= 2;
        }
        (*buffer)[*length] = (char)c;
        (*length)++;
        c = fgetc(file);
    }
    if (ferror(file)) {
        return STATUS_READ_ERROR;
    }
    (*buffer)[*length] = '\0';
    return *length == 0 ? STATUS_END_OF_FILE : STATUS_OK;
}

Status parse_number(const char *str, const size_t length, const int base, long long *result) {
    unsigned long long magnitude = 0;
    unsigned long long limit = (unsigned long long)LLONG_MAX;
    int is_negative = 0;
    int has_lowercase = 0;
    int is_overflow = 0;
    size_t i = 0;
    int digit;
    if (str == NULL || result == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (base < 2 || base > 36) {
        return STATUS_INVALID_BASE;
    }
    if (length > 0 && (str[0] == '-' || str[0] == '+')) {
        is_negative = str[0] == '-';
        i = 1;
    }
    if (is_negative) {
        limit += 1ULL;
    }
    if (i >= length) {
        return STATUS_INVALID_NUMBER;
    }
    for (; i < length; i++) {
        if (str[i] >= '0' && str[i] <= '9') {
            digit = str[i] - '0';
        } else if (str[i] >= 'A' && str[i] <= 'Z') {
            digit = str[i] - 'A' + 10;
        } else if (str[i] >= 'a' && str[i] <= 'z') {
            digit = str[i] - 'a' + 10;
            has_lowercase = 1;
        } else {
            return STATUS_INVALID_NUMBER;
        }
        if (digit >= base) {
            return STATUS_INVALID_NUMBER;
        }
        if (is_overflow || magnitude > (limit - (unsigned long long)digit) / (unsigned long long)base) {
            is_overflow = 1;
        } else {
            magnitude = magnitude * (unsigned long long)base + (unsigned long long)digit;
        }
    }
    if (has_lowercase) {
        return STATUS_LOWERCASE_DIGIT;
    }
    if (is_overflow) {
        return STATUS_OVERFLOW;
    }
    if (is_negative) {
        *result = magnitude == 0 ? 0 : -(long long)(magnitude - 1ULL) - 1LL;
    } else {
        *result = (long long)magnitude;
    }
    return STATUS_OK;
}

Status parse_base(const char *str, const size_t length, int *base) {
    long long value;
    Status status;
    if (str == NULL || base == NULL) {
        return STATUS_NULL_POINTER;
    }
    status = parse_number(str, length, 10, &value);
    if (status == STATUS_INVALID_NUMBER || status == STATUS_LOWERCASE_DIGIT) {
        return STATUS_INVALID_NUMBER;
    }
    if (status == STATUS_OVERFLOW) {
        return STATUS_INVALID_BASE;
    }
    if (status != STATUS_OK) {
        return status;
    }
    if (value < 2 || value > 36) {
        return STATUS_INVALID_BASE;
    }
    *base = (int)value;
    return STATUS_OK;
}

unsigned long long absolute(const long long value) {
    return value < 0 ? 0ULL - (unsigned long long)value : (unsigned long long)value;
}

Status add_number(const long long value, long long *sum, long long *max_abs, size_t *count) {
    if (sum == NULL || max_abs == NULL || count == NULL) {
        return STATUS_NULL_POINTER;
    }
    if ((value > 0 && *sum > LLONG_MAX - value) || (value < 0 && *sum < LLONG_MIN - value)) {
        return STATUS_SUM_OVERFLOW;
    }
    *sum += value;
    if (*count == 0 || absolute(value) > absolute(*max_abs)) {
        *max_abs = value;
    }
    (*count)++;
    return STATUS_OK;
}

Status to_base(const long long value, const int base, char *out, const size_t size) {
    const char alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char digits[70];
    unsigned long long magnitude = absolute(value);
    size_t count = 0;
    size_t position = 0;
    if (out == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (base < 2 || base > 36) {
        return STATUS_INVALID_BASE;
    }
    do {
        digits[count] = alphabet[magnitude % (unsigned long long)base];
        count++;
        magnitude /= (unsigned long long)base;
    } while (magnitude > 0);
    if (count + 2 > size) {
        return STATUS_OVERFLOW;
    }
    if (value < 0) {
        out[position] = '-';
        position++;
    }
    while (count > 0) {
        count--;
        out[position] = digits[count];
        position++;
    }
    out[position] = '\0';
    return STATUS_OK;
}

void print_token(const char *token, const size_t length) {
    const size_t max_shown = 40;
    size_t shown = length;
    size_t i;
    if (token == NULL) {
        return;
    }
    if (shown > max_shown) {
        shown = max_shown;
        while (shown > max_shown - 3 && ((unsigned char)token[shown] & 0xC0) == 0x80) {
            shown--;
        }
    }
    for (i = 0; i < shown; i++) {
        if (token[i] == '\0') {
            printf("\\0");
        } else {
            putchar(token[i]);
        }
    }
    if (shown < length) {
        printf("...");
    }
}

void print_skipped(const Status status, const char *token, const size_t length, const int base) {
    printf("Ошибка: \"");
    print_token(token, length);
    if (status == STATUS_INVALID_NUMBER) {
        printf("\" не является целым числом в системе счисления с основанием %d", base);
    } else if (status == STATUS_LOWERCASE_DIGIT) {
        printf("\" содержит строчные буквы, а цифры больше 9 нужно вводить прописными латинскими буквами");
    } else if (status == STATUS_OVERFLOW) {
        printf("\" выходит за допустимый диапазон [-9223372036854775808, 9223372036854775807]");
    } else if (status == STATUS_SUM_OVERFLOW) {
        printf("\" нельзя прибавить: сумма выйдет за допустимый диапазон [-9223372036854775808, 9223372036854775807]");
    } else {
        printf("\" не обработано");
    }
    printf(", пропущено\n");
}

void print_number(const char *name, const long long value, const int input_base) {
    const int bases[4] = {9, 18, 27, 36};
    char buffer[70];
    int i;
    if (name == NULL) {
        return;
    }
    if (to_base(value, input_base, buffer, sizeof(buffer)) != STATUS_OK) {
        printf("Ошибка перевода числа\n");
        return;
    }
    printf("%s: %s\n", name, buffer);
    for (i = 0; i < 4; i++) {
        if (to_base(value, bases[i], buffer, sizeof(buffer)) != STATUS_OK) {
            printf("Ошибка перевода числа\n");
            return;
        }
        printf("  в системе с основанием %d: %s\n", bases[i], buffer);
    }
}

void print_error(const Status status) {
    if (status == STATUS_MEMORY_ERROR) {
        printf("Ошибка: не удалось выделить память\n");
    } else if (status == STATUS_READ_ERROR) {
        printf("Ошибка чтения ввода\n");
    } else {
        printf("Внутренняя ошибка программы\n");
    }
}

int main(int argc, char *argv[]) {
    char *token = NULL;
    size_t capacity = 0;
    size_t length = 0;
    int base;
    long long value;
    long long sum = 0;
    long long max_abs = 0;
    size_t count = 0;
    int stopped = 0;
    Status status;

    (void)argv;
    if (argc != 1) {
        printf("Ошибка: программа не принимает аргументов командной строки\n");
        printf("Использование: task10.exe, затем ввести основание, числа и строку Stop\n");
        return 1;
    }

    printf("Введите основание системы счисления [2..36]: ");
    status = read_token(stdin, &token, &capacity, &length);
    if (status == STATUS_END_OF_FILE) {
        printf("\nОшибка: основание не введено\n");
        free(token);
        return 1;
    }
    if (status != STATUS_OK) {
        printf("\n");
        print_error(status);
        free(token);
        return 1;
    }
    status = parse_base(token, length, &base);
    if (status != STATUS_OK) {
        if (status == STATUS_INVALID_NUMBER) {
            printf("Ошибка: основание должно быть целым десятичным числом\n");
        } else if (status == STATUS_INVALID_BASE) {
            printf("Ошибка: основание должно быть в диапазоне [2..36]\n");
        } else {
            print_error(status);
        }
        free(token);
        return 1;
    }

    printf("Введите числа (для завершения введите Stop):\n");
    while ((status = read_token(stdin, &token, &capacity, &length)) == STATUS_OK) {
        if (length == 4 && memcmp(token, "Stop", 4) == 0) {
            stopped = 1;
            break;
        }
        status = parse_number(token, length, base, &value);
        if (status == STATUS_OK) {
            status = add_number(value, &sum, &max_abs, &count);
        }
        if (status == STATUS_INVALID_NUMBER || status == STATUS_LOWERCASE_DIGIT ||
            status == STATUS_OVERFLOW || status == STATUS_SUM_OVERFLOW) {
            print_skipped(status, token, length, base);
        } else if (status != STATUS_OK) {
            break;
        }
    }
    free(token);

    if (status != STATUS_OK && status != STATUS_END_OF_FILE) {
        print_error(status);
        return 1;
    }
    if (!stopped) {
        printf("Ввод завершён без строки Stop\n");
    }
    if (count == 0) {
        printf("Не введено ни одного числа\n");
        return 0;
    }

    print_number("Максимальное по модулю", max_abs, base);
    print_number("Сумма", sum, base);
    return 0;
}
