/*
 * Nedflix PS3 - Simple JSON parser
 * Minimal implementation for API responses
 */

#include "nedflix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_JSON_DEPTH 16
#define MAX_STRING_LEN 4096

typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} json_type_t;

struct json_value {
    json_type_t type;
    union {
        bool boolean;
        double number;
        char *string;
        struct {
            struct json_value **items;
            int count;
        } array;
        struct {
            char **keys;
            struct json_value **values;
            int count;
        } object;
    } data;
};

/* Forward declarations */
static json_value_t *parse_value(const char **p);
static void skip_whitespace(const char **p);

/* Skip whitespace */
static void skip_whitespace(const char **p)
{
    while (**p && isspace((unsigned char)**p)) (*p)++;
}

/* Parse string */
static char *parse_string(const char **p)
{
    if (**p != '"') return NULL;
    (*p)++;

    const char *start = *p;
    char *result = malloc(MAX_STRING_LEN);
    if (!result) return NULL;

    int i = 0;
    while (**p && **p != '"' && i < MAX_STRING_LEN - 1) {
        if (**p == '\\') {
            (*p)++;
            switch (**p) {
                case 'n': result[i++] = '\n'; break;
                case 't': result[i++] = '\t'; break;
                case 'r': result[i++] = '\r'; break;
                case '"': result[i++] = '"'; break;
                case '\\': result[i++] = '\\'; break;
                default: result[i++] = **p; break;
            }
        } else {
            result[i++] = **p;
        }
        (*p)++;
    }
    result[i] = '\0';

    if (**p == '"') (*p)++;

    return result;
}

/* Parse number */
static double parse_number(const char **p)
{
    char *end;
    double value = strtod(*p, &end);
    *p = end;
    return value;
}

/* Parse array */
static json_value_t *parse_array(const char **p)
{
    if (**p != '[') return NULL;
    (*p)++;

    json_value_t *arr = calloc(1, sizeof(json_value_t));
    if (!arr) return NULL;
    arr->type = JSON_ARRAY;

    int capacity = 16;
    arr->data.array.items = calloc(capacity, sizeof(json_value_t*));
    arr->data.array.count = 0;

    skip_whitespace(p);

    while (**p && **p != ']') {
        if (arr->data.array.count >= capacity) {
            capacity *= 2;
            arr->data.array.items = realloc(arr->data.array.items,
                                            capacity * sizeof(json_value_t*));
        }

        json_value_t *item = parse_value(p);
        if (item) {
            arr->data.array.items[arr->data.array.count++] = item;
        }

        skip_whitespace(p);
        if (**p == ',') {
            (*p)++;
            skip_whitespace(p);
        }
    }

    if (**p == ']') (*p)++;

    return arr;
}

/* Parse object */
static json_value_t *parse_object(const char **p)
{
    if (**p != '{') return NULL;
    (*p)++;

    json_value_t *obj = calloc(1, sizeof(json_value_t));
    if (!obj) return NULL;
    obj->type = JSON_OBJECT;

    int capacity = 16;
    obj->data.object.keys = calloc(capacity, sizeof(char*));
    obj->data.object.values = calloc(capacity, sizeof(json_value_t*));
    obj->data.object.count = 0;

    skip_whitespace(p);

    while (**p && **p != '}') {
        if (obj->data.object.count >= capacity) {
            capacity *= 2;
            obj->data.object.keys = realloc(obj->data.object.keys,
                                            capacity * sizeof(char*));
            obj->data.object.values = realloc(obj->data.object.values,
                                              capacity * sizeof(json_value_t*));
        }

        skip_whitespace(p);

        char *key = parse_string(p);
        if (!key) break;

        skip_whitespace(p);
        if (**p == ':') (*p)++;
        skip_whitespace(p);

        json_value_t *value = parse_value(p);

        obj->data.object.keys[obj->data.object.count] = key;
        obj->data.object.values[obj->data.object.count] = value;
        obj->data.object.count++;

        skip_whitespace(p);
        if (**p == ',') {
            (*p)++;
            skip_whitespace(p);
        }
    }

    if (**p == '}') (*p)++;

    return obj;
}

/* Parse any value */
static json_value_t *parse_value(const char **p)
{
    skip_whitespace(p);

    json_value_t *val = calloc(1, sizeof(json_value_t));
    if (!val) return NULL;

    if (**p == '"') {
        val->type = JSON_STRING;
        val->data.string = parse_string(p);
    } else if (**p == '[') {
        json_free(val);
        return parse_array(p);
    } else if (**p == '{') {
        json_free(val);
        return parse_object(p);
    } else if (strncmp(*p, "true", 4) == 0) {
        val->type = JSON_BOOL;
        val->data.boolean = true;
        *p += 4;
    } else if (strncmp(*p, "false", 5) == 0) {
        val->type = JSON_BOOL;
        val->data.boolean = false;
        *p += 5;
    } else if (strncmp(*p, "null", 4) == 0) {
        val->type = JSON_NULL;
        *p += 4;
    } else if (**p == '-' || isdigit((unsigned char)**p)) {
        val->type = JSON_NUMBER;
        val->data.number = parse_number(p);
    } else {
        free(val);
        return NULL;
    }

    return val;
}

/* Public API */
json_value_t *json_parse(const char *text)
{
    if (!text) return NULL;
    const char *p = text;
    return parse_value(&p);
}

void json_free(json_value_t *v)
{
    if (!v) return;

    switch (v->type) {
        case JSON_STRING:
            free(v->data.string);
            break;
        case JSON_ARRAY:
            for (int i = 0; i < v->data.array.count; i++) {
                json_free(v->data.array.items[i]);
            }
            free(v->data.array.items);
            break;
        case JSON_OBJECT:
            for (int i = 0; i < v->data.object.count; i++) {
                free(v->data.object.keys[i]);
                json_free(v->data.object.values[i]);
            }
            free(v->data.object.keys);
            free(v->data.object.values);
            break;
        default:
            break;
    }

    free(v);
}

const char *json_get_string(json_value_t *obj, const char *key)
{
    if (!obj || obj->type != JSON_OBJECT) return NULL;

    for (int i = 0; i < obj->data.object.count; i++) {
        if (strcmp(obj->data.object.keys[i], key) == 0) {
            json_value_t *v = obj->data.object.values[i];
            if (v && v->type == JSON_STRING) {
                return v->data.string;
            }
        }
    }

    return NULL;
}

int json_get_int(json_value_t *obj, const char *key, int def)
{
    if (!obj || obj->type != JSON_OBJECT) return def;

    for (int i = 0; i < obj->data.object.count; i++) {
        if (strcmp(obj->data.object.keys[i], key) == 0) {
            json_value_t *v = obj->data.object.values[i];
            if (v && v->type == JSON_NUMBER) {
                return (int)v->data.number;
            }
        }
    }

    return def;
}

double json_get_double(json_value_t *obj, const char *key, double def)
{
    if (!obj || obj->type != JSON_OBJECT) return def;

    for (int i = 0; i < obj->data.object.count; i++) {
        if (strcmp(obj->data.object.keys[i], key) == 0) {
            json_value_t *v = obj->data.object.values[i];
            if (v && v->type == JSON_NUMBER) {
                return v->data.number;
            }
        }
    }

    return def;
}

bool json_get_bool(json_value_t *obj, const char *key, bool def)
{
    if (!obj || obj->type != JSON_OBJECT) return def;

    for (int i = 0; i < obj->data.object.count; i++) {
        if (strcmp(obj->data.object.keys[i], key) == 0) {
            json_value_t *v = obj->data.object.values[i];
            if (v && v->type == JSON_BOOL) {
                return v->data.boolean;
            }
        }
    }

    return def;
}

json_value_t *json_get_object(json_value_t *obj, const char *key)
{
    if (!obj || obj->type != JSON_OBJECT) return NULL;

    for (int i = 0; i < obj->data.object.count; i++) {
        if (strcmp(obj->data.object.keys[i], key) == 0) {
            json_value_t *v = obj->data.object.values[i];
            if (v && v->type == JSON_OBJECT) {
                return v;
            }
        }
    }

    return NULL;
}

json_value_t *json_get_array(json_value_t *obj, const char *key)
{
    if (!obj || obj->type != JSON_OBJECT) return NULL;

    for (int i = 0; i < obj->data.object.count; i++) {
        if (strcmp(obj->data.object.keys[i], key) == 0) {
            json_value_t *v = obj->data.object.values[i];
            if (v && v->type == JSON_ARRAY) {
                return v;
            }
        }
    }

    return NULL;
}

int json_array_length(json_value_t *arr)
{
    if (!arr || arr->type != JSON_ARRAY) return 0;
    return arr->data.array.count;
}

json_value_t *json_array_get(json_value_t *arr, int i)
{
    if (!arr || arr->type != JSON_ARRAY) return NULL;
    if (i < 0 || i >= arr->data.array.count) return NULL;
    return arr->data.array.items[i];
}
