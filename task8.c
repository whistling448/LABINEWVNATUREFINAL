//drmemory -suppress mingw.supp -- .\task8.exe tests\in8.txt tests\o8.txt
// drmemory -suppress mingw.supp -- .\task8.exe tests\bad8.txt tests\o8b.txt

// .\task8.exe tests\in8.txt tests\o8.txt


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <limits.h>

typedef enum {
    STATUS_OK,
    STATUS_END_OF_FILE,
    STATUS_NULL_POINTER,
    STATUS_INVALID_ARGUMENT,
    STATUS_NO_DIGITS,
    STATUS_INVALID_CHARACTER,
    STATUS_OVERFLOW,
    STATUS_MEMORY_ERROR,
    STATUS_READ_ERROR,
    STATUS_WRITE_ERROR,
    STATUS_FILE_INFO_ERROR
} Status;

typedef struct {
    char *text;
    size_t length;
    size_t capacity;
    unsigned long long line;
} Token;

typedef struct {
    int is_negative;
    const char *digits;
    size_t digit_count;
    int base;
    unsigned long long value;
    size_t bad_position;
} Number;

int is_same_text_ignore_case(const char *first, const char *second) {
    while (*first != '\0' && tolower((unsigned char)*first) == tolower((unsigned char)*second)) {
        first++;
        second++;
    }
    return tolower((unsigned char)*first) == tolower((unsigned char)*second);
}

Status is_same_path(const char *first, const char *second, int *result) {
    const size_t max_length = 32768;
    char *full_first;
    char *full_second;
    Status status = STATUS_OK;
    if (first == NULL || second == NULL || result == NULL) {
        return STATUS_NULL_POINTER;
    }
    full_first = (char *)malloc(max_length);
    full_second = (char *)malloc(max_length);
    if (full_first == NULL || full_second == NULL) {
        status = STATUS_MEMORY_ERROR;
    } else if (_fullpath(full_first, first, max_length) == NULL || _fullpath(full_second, second, max_length) == NULL) {
        status = STATUS_FILE_INFO_ERROR;
    } else {
        *result = is_same_text_ignore_case(full_first, full_second);
    }
    free(full_first);
    free(full_second);
    return status;
}

int utf8_sequence_length(const int lead) {
    if (lead < 0x80) {
        return 1;
    }
    if (lead >= 0xC2 && lead <= 0xDF) {
        return 2;
    }
    if (lead >= 0xE0 && lead <= 0xEF) {
        return 3;
    }
    if (lead >= 0xF0 && lead <= 0xF4) {
        return 4;
    }
    return 0;
}

int is_valid_code_point(const unsigned long code, const int length) {
    if (length == 3 && code < 0x800UL) {
        return 0;
    }
    if (length == 4 && (code < 0x10000UL || code > 0x10FFFFUL)) {
        return 0;
    }
    return code < 0xD800UL || code > 0xDFFFUL;
}

unsigned long lead_bits(const int lead, const int length) {
    if (length == 2) {
        return (unsigned long)lead & 0x1FUL;
    }
    if (length == 3) {
        return (unsigned long)lead & 0x0FUL;
    }
    if (length == 4) {
        return (unsigned long)lead & 0x07UL;
    }
    return (unsigned long)lead;
}

Status decode_symbol(const char *text, const size_t length, size_t *position, unsigned long *code) {
    const unsigned char *bytes = (const unsigned char *)text;
    size_t start;
    int expected;
    int i;
    unsigned long value;
    if (text == NULL || position == NULL || code == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (*position >= length) {
        return STATUS_END_OF_FILE;
    }
    start = *position;
    expected = utf8_sequence_length(bytes[start]);
    if (expected < 2 || (size_t)expected > length - start) {
        *code = bytes[start];
        *position = start + 1;
        return STATUS_OK;
    }
    value = lead_bits(bytes[start], expected);
    for (i = 1; i < expected; i++) {
        if ((bytes[start + (size_t)i] & 0xC0) != 0x80) {
            *code = bytes[start];
            *position = start + 1;
            return STATUS_OK;
        }
        value = (value << 6) | ((unsigned long)bytes[start + (size_t)i] & 0x3FUL);
    }
    if (!is_valid_code_point(value, expected)) {
        *code = bytes[start];
        *position = start + 1;
        return STATUS_OK;
    }
    *code = value;
    *position = start + (size_t)expected;
    return STATUS_OK;
}

int is_separator(const int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

Status skip_bom(FILE *file) {
    unsigned char bytes[3];
    size_t count;
    if (file == NULL) {
        return STATUS_NULL_POINTER;
    }
    count = fread(bytes, 1, 3, file);
    if (ferror(file)) {
        return STATUS_READ_ERROR;
    }
    if (count == 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
        return STATUS_OK;
    }
    if (fseek(file, 0L, SEEK_SET) != 0) {
        return STATUS_READ_ERROR;
    }
    return STATUS_OK;
}

Status read_token(FILE *file, Token *token) {
    const size_t start_capacity = 16;
    size_t new_capacity;
    char *new_text;
    int c;
    int previous = 0;
    if (file == NULL || token == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (token->text == NULL && token->capacity != 0) {
        return STATUS_INVALID_ARGUMENT;
    }
    token->length = 0;
    c = fgetc(file);
    while (c != EOF && is_separator(c)) {
        if (c == '\r' || (c == '\n' && previous != '\r')) {
            token->line++;
        }
        previous = c;
        c = fgetc(file);
    }
    while (c != EOF && !is_separator(c)) {
        if (token->length == token->capacity) {
            if (token->capacity > SIZE_MAX / 2) {
                return STATUS_MEMORY_ERROR;
            }
            new_capacity = token->capacity == 0 ? start_capacity : token->capacity * 2;
            new_text = (char *)realloc(token->text, new_capacity);
            if (new_text == NULL) {
                return STATUS_MEMORY_ERROR;
            }
            token->text = new_text;
            token->capacity = new_capacity;
        }
        token->text[token->length] = (char)c;
        token->length++;
        c = fgetc(file);
    }
    if (ferror(file)) {
        return STATUS_READ_ERROR;
    }
    if (c != EOF && ungetc(c, file) == EOF) {
        return STATUS_READ_ERROR;
    }
    return token->length == 0 ? STATUS_END_OF_FILE : STATUS_OK;
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
        return STATUS_INVALID_CHARACTER;
    }
    return STATUS_OK;
}

Status analyze_number(const char *text, const size_t length, Number *number) {
    size_t start = 0;
    size_t i;
    int digit;
    int max_digit = 0;
    if (text == NULL || number == NULL) {
        return STATUS_NULL_POINTER;
    }
    number->is_negative = 0;
    number->digits = NULL;
    number->digit_count = 0;
    number->base = 0;
    number->value = 0;
    number->bad_position = 0;
    if (length > 0 && (text[0] == '-' || text[0] == '+')) {
        number->is_negative = text[0] == '-';
        start = 1;
    }
    if (start >= length) {
        return STATUS_NO_DIGITS;
    }
    for (i = start; i < length; i++) {
        if (char_to_digit(text[i], &digit) != STATUS_OK) {
            number->bad_position = i;
            return STATUS_INVALID_CHARACTER;
        }
        if (digit > max_digit) {
            max_digit = digit;
        }
    }
    while (start + 1 < length && text[start] == '0') {
        start++;
    }
    number->digits = text + start;
    number->digit_count = length - start;
    number->base = max_digit < 1 ? 2 : max_digit + 1;
    for (i = start; i < length; i++) {
        if (char_to_digit(text[i], &digit) != STATUS_OK) {
            number->bad_position = i;
            return STATUS_INVALID_CHARACTER;
        }
        if (number->value > (ULLONG_MAX - (unsigned long long)digit) / (unsigned long long)number->base) {
            return STATUS_OVERFLOW;
        }
        number->value = number->value * (unsigned long long)number->base + (unsigned long long)digit;
    }
    if (number->value == 0) {
        number->is_negative = 0;
    }
    return STATUS_OK;
}

Status write_number(FILE *out, const Number *number) {
    const char *sign;
    if (out == NULL || number == NULL || number->digits == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (number->digit_count == 0 || number->base < 2 || number->base > 36) {
        return STATUS_INVALID_ARGUMENT;
    }
    sign = number->is_negative ? "-" : "";
    if (number->is_negative && fputc('-', out) == EOF) {
        return STATUS_WRITE_ERROR;
    }
    if (fwrite(number->digits, 1, number->digit_count, out) != number->digit_count) {
        return STATUS_WRITE_ERROR;
    }
    if (fprintf(out, " %d %s%llu\n", number->base, sign, number->value) < 0) {
        return STATUS_WRITE_ERROR;
    }
    return STATUS_OK;
}

Status process_file(FILE *in, FILE *out, Token *token, Number *number, unsigned long long *count) {
    Status status;
    if (in == NULL || out == NULL || token == NULL || number == NULL || count == NULL) {
        return STATUS_NULL_POINTER;
    }
    *count = 0;
    status = skip_bom(in);
    if (status != STATUS_OK) {
        return status;
    }
    while ((status = read_token(in, token)) == STATUS_OK) {
        status = analyze_number(token->text, token->length, number);
        if (status != STATUS_OK) {
            return status;
        }
        status = write_number(out, number);
        if (status != STATUS_OK) {
            return status;
        }
        (*count)++;
    }
    return status == STATUS_END_OF_FILE ? STATUS_OK : status;
}

void print_symbol(const char *text, const size_t length, size_t *position) {
    size_t start;
    unsigned long code;
    if (text == NULL || position == NULL || *position >= length) {
        return;
    }
    start = *position;
    if (decode_symbol(text, length, position, &code) != STATUS_OK) {
        *position = length;
        return;
    }
    if (*position - start == 1 && (code < 0x20UL || code >= 0x7FUL)) {
        printf("\\x%02lX", code);
    } else {
        fwrite(text + start, 1, *position - start, stdout);
    }
}

void print_token(const char *text, const size_t length, const unsigned long long limit) {
    size_t position = 0;
    unsigned long long count = 0;
    unsigned long code;
    if (text == NULL) {
        return;
    }
    printf("\"");
    while (position < length) {
        if (count < limit) {
            print_symbol(text, length, &position);
        } else if (decode_symbol(text, length, &position, &code) != STATUS_OK) {
            position = length;
        }
        count++;
    }
    if (count > limit) {
        printf("...\" (длина в символах: %llu)", count);
    } else {
        printf("\"");
    }
}

void print_bad_symbol(const char *text, const size_t length, const size_t position) {
    size_t end = position;
    unsigned long code;
    if (text == NULL || position >= length) {
        return;
    }
    printf("'");
    print_symbol(text, length, &end);
    printf("'");
    end = position;
    if (decode_symbol(text, length, &end, &code) == STATUS_OK && end - position > 1) {
        printf(" (U+%04lX)", code);
    }
}

void print_bad_token(const Token *token, const unsigned long long count) {
    const unsigned long long preview_limit = 40;
    if (token == NULL) {
        return;
    }
    printf("Ошибка: число №%llu (строка %llu) ", count + 1, token->line);
    print_token(token->text, token->length, preview_limit);
}

void print_result(const Status status, const Token *token, const Number *number,
                  const unsigned long long count, const char *output_path) {
    if (token == NULL || number == NULL || output_path == NULL) {
        return;
    }
    switch (status) {
        case STATUS_OK:
            if (count == 0) {
                printf("Во входном файле нет чисел, создан пустой файл %s\n", output_path);
            } else {
                printf("Обработано чисел: %llu. Результат записан в файл %s\n", count, output_path);
            }
            return;
        case STATUS_NO_DIGITS:
            print_bad_token(token, count);
            printf(" - знак без цифр не является числом\n");
            break;
        case STATUS_INVALID_CHARACTER:
            print_bad_token(token, count);
            printf(" содержит недопустимый символ ");
            print_bad_symbol(token->text, token->length, number->bad_position);
            printf(" в позиции %llu; число может состоять только из цифр 0-9 и латинских букв A-Z (a-z) "
                   "с необязательным знаком + или - в начале\n", (unsigned long long)number->bad_position + 1);
            break;
        case STATUS_OVERFLOW:
            print_bad_token(token, count);
            printf(" в минимальной системе счисления с основанием %d по модулю превышает "
                   "максимальное допустимое значение %llu (2^64 - 1)\n", number->base, ULLONG_MAX);
            break;
        case STATUS_MEMORY_ERROR:
            printf("Ошибка: не хватает памяти, чтобы прочитать число №%llu (строка %llu)\n",
                   count + 1, token->line);
            break;
        case STATUS_READ_ERROR:
            printf("Ошибка чтения входного файла\n");
            break;
        case STATUS_WRITE_ERROR:
            printf("Ошибка записи в выходной файл %s\n", output_path);
            return;
        default:
            printf("Ошибка: внутренняя ошибка обработки файла\n");
            break;
    }
    if (count == 0) {
        printf("Обработка остановлена, выходной файл %s пуст\n", output_path);
    } else {
        printf("Обработка остановлена, в файл %s записаны результаты только для чисел до ошибочного "
               "(количество: %llu)\n", output_path, count);
    }
}

int main(int argc, char *argv[]) {
    FILE *in;
    FILE *out;
    Token token = {NULL, 0, 0, 1};
    Number number = {0, NULL, 0, 0, 0, 0};
    unsigned long long count = 0;
    int same = 0;
    int close_failed = 0;
    Status status;

    if (argc != 3) {
        printf("Использование: task8.exe <входной файл> <выходной файл>\n");
        return 1;
    }
    in = fopen(argv[1], "rb");
    if (in == NULL) {
        printf("Ошибка: не удалось открыть входной файл %s\n", argv[1]);
        return 1;
    }
    status = is_same_path(argv[1], argv[2], &same);
    if (status != STATUS_OK) {
        printf("Ошибка: не удалось проверить, совпадает ли выходной файл %s с входным\n", argv[2]);
        fclose(in);
        return 1;
    }
    if (same) {
        printf("Ошибка: входной и выходной файлы совпадают (%s и %s - один и тот же файл)\n", argv[1], argv[2]);
        fclose(in);
        return 1;
    }
    out = fopen(argv[2], "wb");
    if (out == NULL) {
        printf("Ошибка: не удалось открыть выходной файл %s\n", argv[2]);
        fclose(in);
        return 1;
    }

    status = process_file(in, out, &token, &number, &count);
    fclose(in);
    if (fclose(out) != 0) {
        close_failed = 1;
        if (status == STATUS_OK) {
            status = STATUS_WRITE_ERROR;
        }
    }

    print_result(status, &token, &number, count, argv[2]);
    if (close_failed && status != STATUS_WRITE_ERROR) {
        printf("Ошибка записи в выходной файл %s, его содержимое может быть неполным\n", argv[2]);
    }
    free(token.text);
    return status == STATUS_OK ? 0 : 1;
}
