#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    const char *name;
    const char *value;
} TemplateVariable;

static int append_data(char **buffer, size_t *length, size_t *capacity,
                       const char *data, size_t data_length) {
    if (data_length == 0) {
        return 1;
    }

    if (*length > SIZE_MAX - data_length - 1) {
        return 0;
    }

    size_t required = *length + data_length + 1;
    if (required > *capacity) {
        size_t new_capacity = (*capacity == 0) ? 64 : *capacity;
        while (new_capacity < required) {
            if (new_capacity > SIZE_MAX / 2) {
                new_capacity = required;
                break;
            }
            new_capacity *= 2;
        }

        char *new_buffer = realloc(*buffer, new_capacity);
        if (!new_buffer) {
            return 0;
        }

        *buffer = new_buffer;
        *capacity = new_capacity;
    }

    memcpy(*buffer + *length, data, data_length);
    *length += data_length;
    (*buffer)[*length] = '\0';
    return 1;
}

static const char *find_variable(const char *name, size_t name_length,
                                 const TemplateVariable *variables,
                                 size_t variable_count) {
    for (size_t i = 0; i < variable_count; ++i) {
        if (strlen(variables[i].name) == name_length &&
            strncmp(variables[i].name, name, name_length) == 0) {
            return variables[i].value ? variables[i].value : "";
        }
    }
    return NULL;
}

char *render_template(const char *template_text,
                      const TemplateVariable *variables,
                      size_t variable_count) {
    if (!template_text) {
        return NULL;
    }

    char *result = NULL;
    size_t result_length = 0;
    size_t result_capacity = 0;
    size_t position = 0;
    size_t template_length = strlen(template_text);

    while (position < template_length) {
        if (position + 1 < template_length &&
            template_text[position] == '{' &&
            template_text[position + 1] == '{') {
            const char *start = template_text + position + 2;
            const char *end = strstr(start, "}}");

            if (!end) {
                if (!append_data(&result, &result_length, &result_capacity,
                                 template_text + position,
                                 template_length - position)) {
                    free(result);
                    return NULL;
                }
                break;
            }

            const char *name_start = start;
            const char *name_end = end;

            while (name_start < name_end && isspace((unsigned char)*name_start)) {
                ++name_start;
            }
            while (name_end > name_start &&
                   isspace((unsigned char)name_end[-1])) {
                --name_end;
            }

            const char *value = NULL;
            if (name_start != name_end) {
                value = find_variable(name_start,
                                      (size_t)(name_end - name_start),
                                      variables,
                                      variable_count);
            }

            if (value) {
                if (!append_data(&result, &result_length, &result_capacity,
                                 value, strlen(value))) {
                    free(result);
                    return NULL;
                }
            } else {
                size_t placeholder_length =
                    (size_t)((end + 2) - (template_text + position));

                if (!append_data(&result, &result_length, &result_capacity,
                                 template_text + position,
                                 placeholder_length)) {
                    free(result);
                    return NULL;
                }
            }

            position = (size_t)((end + 2) - template_text);
        } else {
            if (!append_data(&result, &result_length, &result_capacity,
                             template_text + position, 1)) {
                free(result);
                return NULL;
            }
            ++position;
        }
    }

    if (!result) {
        result = malloc(1);
        if (!result) {
            return NULL;
        }
        result[0] = '\0';
    }

    return result;
}
