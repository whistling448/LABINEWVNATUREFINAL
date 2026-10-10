// .\task4.exe -i tests\in4.txt
// .\task4.exe -d tests\in4.txt
// .\task4.exe /s tests\in4.txt
// .\task4.exe -a tests\in4.txt

//.\task4.exe -nd tests\in4.txt tests\res_d.txt
//.\task4.exe -ni tests\in4.txt tests\res_i.txt
//.\task4.exe /ns tests\in4.txt tests\res_s.txt
// .\task4.exe -na tests\in4.txt tests\res_a.txt

//drmemory -suppress mingw.supp -- .\task4.exe -d tests\in4.txt
//drmemory -suppress mingw.supp -- .\task4.exe -nd tests\in4.txt нет_папки\res.txt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

typedef enum {
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_INVALID_FLAG,
    STATUS_INVALID_PATH,
    STATUS_MEMORY_ERROR,
    STATUS_READ_ERROR,
    STATUS_WRITE_ERROR,
    STATUS_FILE_INFO_ERROR,
    STATUS_END_OF_FILE
} Status;

typedef struct {
    unsigned char bytes[4];
    int length;
    int is_valid;
    unsigned long code;
} Symbol;

int is_latin(const unsigned long c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int is_digit(const unsigned long c) {
    return c >= '0' && c <= '9';
}

int is_special(const unsigned long c) {
    return !is_latin(c) && !is_digit(c) && c != ' ';
}

int is_path_separator(const char c) {
    return c == '/' || c == '\\' || c == ':';
}

void print_usage(void) {
    printf("Использование: task4.exe <флаг> <входной файл> [выходной файл]\n");
    printf("Флаги: -d -i -s -a, с выходным файлом: -nd -ni -ns -na (вместо '-' можно '/')\n");
}

Status parse_flag(const char *flag, char *action, int *has_output) {
    const char *letter;
    if (flag == NULL || action == NULL || has_output == NULL) {
        return STATUS_NULL_POINTER;
    }
    if (flag[0] != '-' && flag[0] != '/') {
        return STATUS_INVALID_FLAG;
    }
    letter = flag + 1;
    *has_output = 0;
    if (letter[0] == 'n') {
        *has_output = 1;
        letter++;
    }
    if (letter[0] == '\0' || letter[1] != '\0') {
        return STATUS_INVALID_FLAG;
    }
    if (letter[0] != 'd' && letter[0] != 'i' && letter[0] != 's' && letter[0] != 'a') {
        return STATUS_INVALID_FLAG;
    }
    *action = letter[0];
    return STATUS_OK;
}

Status make_output_name(const char *input, char **output) {
    const char *prefix = "out_";
    size_t prefix_length;
    size_t input_length;
    size_t dir_length = 0;
    size_t i;
    if (input == NULL || output == NULL) {
        return STATUS_NULL_POINTER;
    }
    prefix_length = strlen(prefix);
    input_length = strlen(input);
    for (i = 0; i < input_length; i++) {
        if (is_path_separator(input[i])) {
            dir_length = i + 1;
        }
    }
    if (dir_length == input_length) {
        return STATUS_INVALID_PATH;
    }
    if (input_length > SIZE_MAX - prefix_length - 1) {
        return STATUS_MEMORY_ERROR;
    }
    *output = (char *)malloc(input_length + prefix_length + 1);
    if (*output == NULL) {
        return STATUS_MEMORY_ERROR;
    }
    memcpy(*output, input, dir_length);
    memcpy(*output + dir_length, prefix, prefix_length);
    memcpy(*output + dir_length + prefix_length, input + dir_length, input_length - dir_length + 1);
    return STATUS_OK;
}

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

Status read_symbol(FILE *in, Symbol *symbol) {
    int c;
    int expected;
    if (in == NULL || symbol == NULL) {
        return STATUS_NULL_POINTER;
    }
    c = fgetc(in);
    if (c == EOF) {
        return ferror(in) ? STATUS_READ_ERROR : STATUS_END_OF_FILE;
    }
    expected = utf8_sequence_length(c);
    symbol->bytes[0] = (unsigned char)c;
    symbol->length = 1;
    symbol->is_valid = expected != 0;
    symbol->code = lead_bits(c, expected);
    while (symbol->is_valid && symbol->length < expected) {
        c = fgetc(in);
        if (c == EOF || (c & 0xC0) != 0x80) {
            if (c != EOF && ungetc(c, in) == EOF) {
                return STATUS_READ_ERROR;
            }
            symbol->is_valid = 0;
        } else {
            symbol->bytes[symbol->length] = (unsigned char)c;
            symbol->length++;
            symbol->code = (symbol->code << 6) | ((unsigned long)c & 0x3FUL);
        }
    }
    if (ferror(in)) {
        return STATUS_READ_ERROR;
    }
    if (symbol->is_valid && !is_valid_code_point(symbol->code, expected)) {
        symbol->is_valid = 0;
    }
    return STATUS_OK;
}

Status remove_digits(FILE *in, FILE *out) {
    int c;
    if (in == NULL || out == NULL) {
        return STATUS_NULL_POINTER;
    }
    while ((c = fgetc(in)) != EOF) {
        if (!is_digit(c) && fputc(c, out) == EOF) {
            return STATUS_WRITE_ERROR;
        }
    }
    return ferror(in) ? STATUS_READ_ERROR : STATUS_OK;
}

Status count_in_line(FILE *in, int (*should_count)(const unsigned long), const int is_first_line,
                     unsigned long long *count, int *has_line) {
    Symbol symbol;
    Status status;
    int is_file_start = is_first_line;
    int i;
    if (in == NULL || should_count == NULL || count == NULL || has_line == NULL) {
        return STATUS_NULL_POINTER;
    }
    *count = 0;
    *has_line = 0;
    while ((status = read_symbol(in, &symbol)) == STATUS_OK) {
        if (is_file_start && symbol.is_valid && symbol.code == 0xFEFFUL) {
            is_file_start = 0;
            continue;
        }
        is_file_start = 0;
        if (symbol.is_valid && symbol.code == '\n') {
            *has_line = 1;
            return STATUS_OK;
        }
        if (symbol.is_valid && symbol.code == '\r') {
            continue;
        }
        *has_line = 1;
        if (symbol.is_valid) {
            if (should_count(symbol.code)) {
                (*count)++;
            }
        } else {
            for (i = 0; i < symbol.length; i++) {
                if (should_count(symbol.bytes[i])) {
                    (*count)++;
                }
            }
        }
    }
    return status == STATUS_END_OF_FILE ? STATUS_OK : status;
}

Status count_in_lines(FILE *in, FILE *out, int (*should_count)(const unsigned long)) {
    unsigned long long count;
    int has_line;
    int is_first_line = 1;
    Status status;
    if (in == NULL || out == NULL || should_count == NULL) {
        return STATUS_NULL_POINTER;
    }
    do {
        status = count_in_line(in, should_count, is_first_line, &count, &has_line);
        if (status != STATUS_OK) {
            return status;
        }
        if (has_line && fprintf(out, "%llu\n", count) < 0) {
            return STATUS_WRITE_ERROR;
        }
        is_first_line = 0;
    } while (has_line);
    return STATUS_OK;
}

Status replace_with_hex(FILE *in, FILE *out) {
    Symbol symbol;
    Status status;
    int written;
    int i;
    if (in == NULL || out == NULL) {
        return STATUS_NULL_POINTER;
    }
    while ((status = read_symbol(in, &symbol)) == STATUS_OK) {
        if (symbol.is_valid && is_digit(symbol.code)) {
            written = fputc((int)symbol.code, out) == EOF ? -1 : 1;
        } else if (symbol.is_valid) {
            written = fprintf(out, "%02lX", symbol.code);
        } else {
            written = 1;
            for (i = 0; i < symbol.length && written >= 0; i++) {
                written = fprintf(out, "%02X", (unsigned int)symbol.bytes[i]);
            }
        }
        if (written < 0) {
            return STATUS_WRITE_ERROR;
        }
    }
    return status == STATUS_END_OF_FILE ? STATUS_OK : status;
}

Status process_file(FILE *in, FILE *out, const char action) {
    if (in == NULL || out == NULL) {
        return STATUS_NULL_POINTER;
    }
    switch (action) {
        case 'd':
            return remove_digits(in, out);
        case 'i':
            return count_in_lines(in, out, is_latin);
        case 's':
            return count_in_lines(in, out, is_special);
        case 'a':
            return replace_with_hex(in, out);
        default:
            return STATUS_INVALID_FLAG;
    }
}

int main(int argc, char *argv[]) {
    char action;
    int has_output;
    int is_same;
    int expected_argc;
    char *output_name = NULL;
    FILE *in;
    FILE *out;
    Status status;

    if (argc < 2) {
        printf("Ошибка: не указан флаг и входной файл\n");
        print_usage();
        return 1;
    }

    status = parse_flag(argv[1], &action, &has_output);
    if (status != STATUS_OK) {
        printf("Ошибка: некорректный флаг \"%s\"\n", argv[1]);
        print_usage();
        return 1;
    }

    expected_argc = has_output ? 4 : 3;
    if (argc != expected_argc) {
        if (has_output) {
            printf("Ошибка: для флага %s нужны 2 аргумента (входной и выходной файлы), передано %d\n",
                   argv[1], argc - 2);
        } else {
            printf("Ошибка: для флага %s нужен 1 аргумент (входной файл), передано %d\n", argv[1], argc - 2);
        }
        print_usage();
        return 1;
    }

    if (argv[2][0] == '\0') {
        printf("Ошибка: путь к входному файлу не должен быть пустым\n");
        return 1;
    }
    if (has_output && argv[3][0] == '\0') {
        printf("Ошибка: путь к выходному файлу не должен быть пустым\n");
        return 1;
    }

    if (has_output) {
        output_name = (char *)malloc(strlen(argv[3]) + 1);
        if (output_name == NULL) {
            printf("Ошибка: не удалось выделить память\n");
            return 1;
        }
        strcpy(output_name, argv[3]);
    } else {
        status = make_output_name(argv[2], &output_name);
        if (status == STATUS_INVALID_PATH) {
            printf("Ошибка: путь к входному файлу должен заканчиваться именем файла: %s\n", argv[2]);
            return 1;
        }
        if (status != STATUS_OK) {
            printf("Ошибка: не удалось выделить память\n");
            return 1;
        }
    }

    in = fopen(argv[2], "rb");
    if (in == NULL) {
        printf("Ошибка: не удалось открыть входной файл %s\n", argv[2]);
        free(output_name);
        return 1;
    }

    status = is_same_path(argv[2], output_name, &is_same);
    if (status != STATUS_OK) {
        printf("Ошибка: не удалось проверить, что выходной файл %s отличается от входного\n", output_name);
        fclose(in);
        free(output_name);
        return 1;
    }
    if (is_same) {
        printf("Ошибка: входной и выходной файлы совпадают (%s и %s - один и тот же файл)\n", argv[2], output_name);
        fclose(in);
        free(output_name);
        return 1;
    }

    out = fopen(output_name, "wb");
    if (out == NULL) {
        printf("Ошибка: не удалось создать выходной файл %s\n", output_name);
        fclose(in);
        free(output_name);
        return 1;
    }

    status = process_file(in, out, action);
    fclose(in);
    if (fclose(out) != 0 && status == STATUS_OK) {
        status = STATUS_WRITE_ERROR;
    }

    switch (status) {
        case STATUS_OK:
            printf("Результат записан в файл %s\n", output_name);
            break;
        case STATUS_READ_ERROR:
            printf("Ошибка чтения входного файла %s\n", argv[2]);
            break;
        case STATUS_WRITE_ERROR:
            printf("Ошибка записи в выходной файл %s\n", output_name);
            break;
        default:
            printf("Ошибка обработки файла\n");
            break;
    }
    if (status != STATUS_OK && remove(output_name) != 0) {
        printf("Ошибка: не удалось удалить неполный выходной файл %s\n", output_name);
    }
    free(output_name);
    return status == STATUS_OK ? 0 : 1;
}
