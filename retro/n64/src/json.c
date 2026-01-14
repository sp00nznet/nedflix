/*
 * Nedflix N64 - Minimal JSON parser
 * Memory-constrained implementation for N64's 4-8MB RAM
 */

#include "nedflix.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_JSON_DEPTH 8
#define MAX_JSON_TOKENS 64
#define MAX_STRING_LEN 128

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
        bool bool_val;
        int int_val;
        char *str_val;
        struct {
            struct json_value **items;
            int count;
        } array;
        struct {
            char **keys;
            struct json_value **values;
            int count;
        } object;
    };
};

static const char *skip_ws(const char *p)
{
    while (*p && isspace(*p)) p++;
    return p;
}

static const char *parse_string(const char *p, char **out)
{
    if (*p != '"') return NULL;
    p++;

    const char *start = p;
    while (*p && *p != '"') {
        if (*p == '\\') p++;
        if (*p) p++;
    }

    size_t len = p - start;
    if (len > MAX_STRING_LEN) len = MAX_STRING_LEN;

    *out = malloc(len + 1);
    if (*out) {
        strncpy(*out, start, len);
        (*out)[len] = '\0';
    }

    if (*p == '"') p++;
    return p;
}

static const char *parse_value(const char *p, json_value_t **out);

static const char *parse_array(const char *p, json_value_t **out)
{
    if (*p != '[') return NULL;
    p++;

    *out = calloc(1, sizeof(json_value_t));
    if (!*out) return NULL;
    (*out)->type = JSON_ARRAY;

    p = skip_ws(p);
    if (*p == ']') {
        p++;
        return p;
    }

    json_value_t *items[MAX_JSON_TOKENS];
    int count = 0;

    while (*p && count < MAX_JSON_TOKENS) {
        p = skip_ws(p);
        json_value_t *item = NULL;
        p = parse_value(p, &item);
        if (!p) break;

        items[count++] = item;

        p = skip_ws(p);
        if (*p == ',') p++;
        else if (*p == ']') break;
    }

    if (*p == ']') p++;

    (*out)->array.items = malloc(count * sizeof(json_value_t *));
    if ((*out)->array.items) {
        memcpy((*out)->array.items, items, count * sizeof(json_value_t *));
        (*out)->array.count = count;
    }

    return p;
}

static const char *parse_object(const char *p, json_value_t **out)
{
    if (*p != '{') return NULL;
    p++;

    *out = calloc(1, sizeof(json_value_t));
    if (!*out) return NULL;
    (*out)->type = JSON_OBJECT;

    p = skip_ws(p);
    if (*p == '}') {
        p++;
        return p;
    }

    char *keys[MAX_JSON_TOKENS];
    json_value_t *values[MAX_JSON_TOKENS];
    int count = 0;

    while (*p && count < MAX_JSON_TOKENS) {
        p = skip_ws(p);

        char *key = NULL;
        p = parse_string(p, &key);
        if (!p || !key) break;

        p = skip_ws(p);
        if (*p != ':') { free(key); break; }
        p++;

        p = skip_ws(p);
        json_value_t *val = NULL;
        p = parse_value(p, &val);
        if (!p) { free(key); break; }

        keys[count] = key;
        values[count] = val;
        count++;

        p = skip_ws(p);
        if (*p == ',') p++;
        else if (*p == '}') break;
    }

    if (*p == '}') p++;

    (*out)->object.keys = malloc(count * sizeof(char *));
    (*out)->object.values = malloc(count * sizeof(json_value_t *));
    if ((*out)->object.keys && (*out)->object.values) {
        memcpy((*out)->object.keys, keys, count * sizeof(char *));
        memcpy((*out)->object.values, values, count * sizeof(json_value_t *));
        (*out)->object.count = count;
    }

    return p;
}

static const char *parse_value(const char *p, json_value_t **out)
{
    p = skip_ws(p);
    if (!*p) return NULL;

    if (*p == '"') {
        char *str = NULL;
        p = parse_string(p, &str);
        if (str) {
            *out = calloc(1, sizeof(json_value_t));
            if (*out) {
                (*out)->type = JSON_STRING;
                (*out)->str_val = str;
            } else {
                free(str);
            }
        }
        return p;
    }

    if (*p == '[') return parse_array(p, out);
    if (*p == '{') return parse_object(p, out);

    if (*p == 't' && strncmp(p, "true", 4) == 0) {
        *out = calloc(1, sizeof(json_value_t));
        if (*out) {
            (*out)->type = JSON_BOOL;
            (*out)->bool_val = true;
        }
        return p + 4;
    }

    if (*p == 'f' && strncmp(p, "false", 5) == 0) {
        *out = calloc(1, sizeof(json_value_t));
        if (*out) {
            (*out)->type = JSON_BOOL;
            (*out)->bool_val = false;
        }
        return p + 5;
    }

    if (*p == 'n' && strncmp(p, "null", 4) == 0) {
        *out = calloc(1, sizeof(json_value_t));
        if (*out) (*out)->type = JSON_NULL;
        return p + 4;
    }

    if (*p == '-' || isdigit(*p)) {
        *out = calloc(1, sizeof(json_value_t));
        if (*out) {
            (*out)->type = JSON_NUMBER;
            (*out)->int_val = atoi(p);
        }
        if (*p == '-') p++;
        while (isdigit(*p)) p++;
        if (*p == '.') {
            p++;
            while (isdigit(*p)) p++;
        }
        return p;
    }

    return NULL;
}

json_value_t *json_parse(const char *text)
{
    if (!text) return NULL;
    json_value_t *root = NULL;
    parse_value(text, &root);
    return root;
}

void json_free(json_value_t *v)
{
    if (!v) return;

    switch (v->type) {
        case JSON_STRING:
            free(v->str_val);
            break;
        case JSON_ARRAY:
            for (int i = 0; i < v->array.count; i++)
                json_free(v->array.items[i]);
            free(v->array.items);
            break;
        case JSON_OBJECT:
            for (int i = 0; i < v->object.count; i++) {
                free(v->object.keys[i]);
                json_free(v->object.values[i]);
            }
            free(v->object.keys);
            free(v->object.values);
            break;
        default:
            break;
    }
    free(v);
}

const char *json_get_string(json_value_t *obj, const char *key)
{
    if (!obj || obj->type != JSON_OBJECT) return NULL;

    for (int i = 0; i < obj->object.count; i++) {
        if (strcmp(obj->object.keys[i], key) == 0) {
            json_value_t *v = obj->object.values[i];
            if (v && v->type == JSON_STRING)
                return v->str_val;
        }
    }
    return NULL;
}

int json_get_int(json_value_t *obj, const char *key, int def)
{
    if (!obj || obj->type != JSON_OBJECT) return def;

    for (int i = 0; i < obj->object.count; i++) {
        if (strcmp(obj->object.keys[i], key) == 0) {
            json_value_t *v = obj->object.values[i];
            if (v && v->type == JSON_NUMBER)
                return v->int_val;
        }
    }
    return def;
}

bool json_get_bool(json_value_t *obj, const char *key, bool def)
{
    if (!obj || obj->type != JSON_OBJECT) return def;

    for (int i = 0; i < obj->object.count; i++) {
        if (strcmp(obj->object.keys[i], key) == 0) {
            json_value_t *v = obj->object.values[i];
            if (v && v->type == JSON_BOOL)
                return v->bool_val;
        }
    }
    return def;
}

json_value_t *json_get_array(json_value_t *obj, const char *key)
{
    if (!obj || obj->type != JSON_OBJECT) return NULL;

    for (int i = 0; i < obj->object.count; i++) {
        if (strcmp(obj->object.keys[i], key) == 0) {
            json_value_t *v = obj->object.values[i];
            if (v && v->type == JSON_ARRAY)
                return v;
        }
    }
    return NULL;
}

int json_array_length(json_value_t *arr)
{
    if (!arr || arr->type != JSON_ARRAY) return 0;
    return arr->array.count;
}

json_value_t *json_array_get(json_value_t *arr, int i)
{
    if (!arr || arr->type != JSON_ARRAY) return NULL;
    if (i < 0 || i >= arr->array.count) return NULL;
    return arr->array.items[i];
}
