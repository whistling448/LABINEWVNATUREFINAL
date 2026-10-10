// drmemory -suppress mingw.supp -- .\task7.exe -r tests\a7.txt tests\b7.txt tests\o7r.txt
// drmemory -suppress mingw.supp -- .\task7.exe -a tests\c7.txt tests\o7a.txt
// drmemory -suppress mingw.supp -- .\task7.exe -r tests\a7.txt нет_файла tests\o.txt

//.\task7.exe -r tests\a7.txt tests\b7.txt tests\o7r.txt
//.\task7.exe -a tests\c7.txt tests\o7a.txt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <ctype.h>

typedef enum {
    STATUS_OK,
    STATUS_END_OF_FILE,
    STATUS_NULL_POINTER,
    STATUS_INVALID_ARGUMENT,
    STATUS_INVALID_BASE,
    STATUS_BUFFER_TOO_SMALL,
    STATUS_MEMORY_ERROR,
    STATUS_READ_ERROR,
    STATUS_WRITE_ERROR,
    STATUS_FILE_INFO_ERROR,
    STATUS_SAME_FILE
} Status;

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

Status check_output(const char *path, const char *first, const char *second) {
    int same = 0;
    Status status;
    if (path == NULL || first == NULL) {
        return STATUS_NULL_POINTER;
    }
    status = is_same_path(path, first, &same);
    if (status == STATUS_OK && !same && second != NULL) {
        status = is_same_path(path, second, &same);
    }
    if (status != STATUS_OK) {
        return status;
    }
    return same ? STATUS_SAME_FILE : STATUS_OK;
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

unsigned long to_lower_latin(const unsigned long code) {
    if (code >= 'A' && code <= 'Z') {
        return code - 'A' + 'a';
    }
    return code;
}

Status skip_bom(FILE *file) {
    const int bom[3] = {0xEF, 0xBB, 0xBF};
    int i = 0;
    if (file == NULL) {
        return STATUS_NULL_POINTER;
    }
    while (i < 3 && fgetc(file) == bom[i]) {
        i++;
    }
    if (ferror(file)) {
        return STATUS_READ_ERROR;
    }
    if (i < 3 && fseek(file, 0L, SEEK_SET) != 0) {
        return STATUS_READ_ERROR;
    }
    return STATUS_OK;
}

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
    do {
        c = fgetc(file);
    } while (c != EOF && isspace(c));
    while (c != EOF && !isspace(c)) {
        if (*length == *capacity) {
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
    return *length == 0 ? STATUS_END_OF_FILE : STATUS_OK;
}

Status write_token(FILE *file, const char *token, const size_t length, int *is_first) {
    if (file == NULL || token == NULL || is_first == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (length == 0) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (!*is_first && fputc(' ', file) == EOF) {
        return STATUS_WRITE_ERROR;
    }
    if (fwrite(token, 1, length, file) != length) {
        return STATUS_WRITE_ERROR;
    }
    *is_first = 0;
    return STATUS_OK;
}

Status to_base(unsigned long value, const int base, char *out, const size_t out_size, size_t *out_length) {
    char digits[sizeof(unsigned long) * CHAR_BIT];
    size_t count = 0;
    size_t i;
    if (out == NULL || out_length == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (base < 2 || base > 10) {
        return STATUS_INVALID_BASE;
    }
    do {
        digits[count] = (char)('0' + value % (unsigned long)base);
        count++;
        value /= (unsigned long)base;
    } while (value > 0);
    if (count > out_size) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    for (i = 0; i < count; i++) {
        out[i] = digits[count - 1 - i];
    }
    *out_length = count;
    return STATUS_OK;
}

Status transform_token(const char *token, const size_t length, const int position, char **result,
                       size_t *result_length) {
    size_t max_length;
    size_t i = 0;
    size_t written;
    unsigned long code;
    int base = 0;
    int to_lower = 0;
    Status status;
    if (token == NULL || result == NULL || result_length == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (length == 0 || position < 1 || position > 10) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (position == 10) {
        to_lower = 1;
        base = 4;
    } else if (position % 2 == 0) {
        to_lower = 1;
    } else if (position == 5) {
        base = 8;
    }
    max_length = length;
    if (base != 0) {
        if (length > SIZE_MAX / 4) {
            return STATUS_MEMORY_ERROR;
        }
        max_length = length * 4;
    }
    *result = (char *)malloc(max_length);
    if (*result == NULL) {
        return STATUS_MEMORY_ERROR;
    }
    *result_length = 0;
    if (base == 0) {
        for (i = 0; i < length; i++) {
            code = (unsigned char)token[i];
            if (to_lower) {
                code = to_lower_latin(code);
            }
            (*result)[i] = (char)code;
        }
        *result_length = length;
        return STATUS_OK;
    }
    while (i < length) {
        status = decode_symbol(token, length, &i, &code);
        if (status == STATUS_OK) {
            if (to_lower) {
                code = to_lower_latin(code);
            }
            status = to_base(code, base, *result + *result_length, max_length - *result_length, &written);
        }
        if (status != STATUS_OK) {
            free(*result);
            *result = NULL;
            return status;
        }
        *result_length += written;
    }
    return STATUS_OK;
}

Status copy_next_token(FILE *in, FILE *out, char **buffer, size_t *capacity, int *is_first, int *has_more) {
    size_t length;
    Status status;
    if (in == NULL || out == NULL || buffer == NULL || capacity == NULL || is_first == NULL ||
        has_more == NULL) {
        return STATUS_NULL_POINTER;
    }
    status = read_token(in, buffer, capacity, &length);
    if (status == STATUS_END_OF_FILE) {
        *has_more = 0;
        return STATUS_OK;
    }
    if (status != STATUS_OK) {
        return status;
    }
    return write_token(out, *buffer, length, is_first);
}

Status merge_files(FILE *first, FILE *second, FILE *out) {
    char *token = NULL;
    size_t capacity = 0;
    int has_more1 = 1;
    int has_more2 = 1;
    int is_first = 1;
    Status status;
    if (first == NULL || second == NULL || out == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (first == second || first == out || second == out) {
        return STATUS_INVALID_ARGUMENT;
    }
    status = skip_bom(first);
    if (status == STATUS_OK) {
        status = skip_bom(second);
    }
    while (status == STATUS_OK && (has_more1 || has_more2)) {
        if (has_more1) {
            status = copy_next_token(first, out, &token, &capacity, &is_first, &has_more1);
        }
        if (status == STATUS_OK && has_more2) {
            status = copy_next_token(second, out, &token, &capacity, &is_first, &has_more2);
        }
    }
    free(token);
    return status;
}

Status convert_file(FILE *in, FILE *out) {
    char *token = NULL;
    char *converted;
    size_t capacity = 0;
    size_t length;
    size_t converted_length;
    int position = 0;
    int is_first = 1;
    Status status;
    if (in == NULL || out == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (in == out) {
        return STATUS_INVALID_ARGUMENT;
    }
    status = skip_bom(in);
    if (status != STATUS_OK) {
        return status;
    }
    while ((status = read_token(in, &token, &capacity, &length)) == STATUS_OK) {
        position = position % 10 + 1;
        status = transform_token(token, length, position, &converted, &converted_length);
        if (status != STATUS_OK) {
            break;
        }
        status = write_token(out, converted, converted_length, &is_first);
        free(converted);
        if (status != STATUS_OK) {
            break;
        }
    }
    free(token);
    return status == STATUS_END_OF_FILE ? STATUS_OK : status;
}

int is_flag(const char *arg, const char letter) {
    return arg != NULL && (arg[0] == '-' || arg[0] == '/') && arg[1] == letter && arg[2] == '\0';
}

void print_usage(void) {
    printf("Использование:\n");
    printf("  task7.exe -r <файл1> <файл2> <выходной файл>\n");
    printf("  task7.exe -a <файл> <выходной файл>\n");
    printf("Флаг может начинаться с '-' или '/'\n");
}

void print_status(const Status status) {
    switch (status) {
        case STATUS_OK:
            printf("Результат записан в выходной файл\n");
            break;
        case STATUS_MEMORY_ERROR:
            printf("Ошибка: не удалось выделить память\n");
            break;
        case STATUS_READ_ERROR:
            printf("Ошибка чтения входного файла\n");
            break;
        case STATUS_WRITE_ERROR:
            printf("Ошибка записи в выходной файл\n");
            break;
        case STATUS_SAME_FILE:
            printf("Ошибка: выходной файл совпадает с входным\n");
            break;
        case STATUS_FILE_INFO_ERROR:
            printf("Ошибка: не удалось проверить, совпадает ли выходной файл с входным\n");
            break;
        case STATUS_END_OF_FILE:
        case STATUS_NULL_POINTER:
        case STATUS_INVALID_ARGUMENT:
        case STATUS_INVALID_BASE:
        case STATUS_BUFFER_TOO_SMALL:
            printf("Ошибка: внутренняя ошибка обработки файлов\n");
            break;
    }
}

int main(int argc, char *argv[]) {
    const char *flag;
    const char *out_path;
    FILE *first;
    FILE *second = NULL;
    FILE *out;
    int is_merge;
    Status status;

    if (argc < 2) {
        printf("Ошибка: не указан флаг\n");
        print_usage();
        return 1;
    }

    flag = argv[1];
    if (!is_flag(flag, 'r') && !is_flag(flag, 'a')) {
        printf("Ошибка: некорректный флаг %s\n", flag);
        print_usage();
        return 1;
    }
    is_merge = flag[1] == 'r';
    if (is_merge && argc != 5) {
        printf("Ошибка: для флага %s нужно 3 пути к файлам: <файл1> <файл2> <выходной файл>\n", flag);
        return 1;
    }
    if (!is_merge && argc != 4) {
        printf("Ошибка: для флага %s нужно 2 пути к файлам: <файл> <выходной файл>\n", flag);
        return 1;
    }
    out_path = argv[argc - 1];

    first = fopen(argv[2], "rb");
    if (first == NULL) {
        printf("Ошибка: не удалось открыть входной файл %s\n", argv[2]);
        return 1;
    }
    if (is_merge) {
        second = fopen(argv[3], "rb");
        if (second == NULL) {
            printf("Ошибка: не удалось открыть входной файл %s\n", argv[3]);
            fclose(first);
            return 1;
        }
    }

    status = check_output(out_path, argv[2], is_merge ? argv[3] : NULL);
    if (status != STATUS_OK) {
        print_status(status);
        fclose(first);
        if (second != NULL) {
            fclose(second);
        }
        return 1;
    }
    out = fopen(out_path, "wb");
    if (out == NULL) {
        printf("Ошибка: не удалось открыть выходной файл %s\n", out_path);
        fclose(first);
        if (second != NULL) {
            fclose(second);
        }
        return 1;
    }

    if (is_merge) {
        status = merge_files(first, second, out);
    } else {
        status = convert_file(first, out);
    }
    fclose(first);
    if (second != NULL) {
        fclose(second);
    }
    if (fclose(out) != 0 && status == STATUS_OK) {
        status = STATUS_WRITE_ERROR;
    }
    print_status(status);
    return status == STATUS_OK ? 0 : 1;
}
