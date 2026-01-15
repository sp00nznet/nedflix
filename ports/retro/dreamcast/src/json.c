/*
 * Nedflix for Sega Dreamcast
 * Minimal JSON parser
 *
 * This is a simple, memory-efficient JSON parser designed for the
 * Dreamcast's limited 16MB RAM. It only supports the subset of JSON
 * needed for the Nedflix API responses.
 */

#include "nedflix.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* JSON value types */
typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} json_type_t;

/* JSON value structure */
struct json_value {
    json_type_t type;
    union {
        bool bool_val;
        double num_val;
        char *str_val;
        struct {
            struct json_value **items;
            int count;
            int capacity;
        } array;
        struct {
            char **keys;
            struct json_value **values;
            int count;
            int capacity;
        } object;
    } data;
};

/* Parser state */
typedef struct {
    const char *json;
    const char *ptr;
    char error[128];
} parser_t;

/* Forward declarations */
static json_value_t *parse_value(parser_t *p);

/*
 * Skip whitespace
 */
static void skip_whitespace(parser_t *p)
{
    while (*p->ptr && isspace((unsigned char)*p->ptr)) {
        p->ptr++;
    }
}

/*
 * Parse a string (including quotes)
 */
static char *parse_string_value(parser_t *p)
{
    if (*p->ptr != '"') return NULL;
    p->ptr++;

    const char *start = p->ptr;
    size_t len = 0;

    /* Find end of string */
    while (*p->ptr && *p->ptr != '"') {
        if (*p->ptr == '\\' && *(p->ptr + 1)) {
            p->ptr += 2;
            len++;
        } else {
            p->ptr++;
            len++;
        }
    }

    if (*p->ptr != '"') return NULL;

    /* Allocate and copy string */
    char *str = (char *)malloc(len + 1);
    if (!str) return NULL;

    /* Copy with escape handling */
    const char *src = start;
    char *dst = str;
    while (src < p->ptr) {
        if (*src == '\\' && src + 1 < p->ptr) {
            src++;
            switch (*src) {
                case 'n': *dst++ = '\n'; break;
                case 'r': *dst++ = '\r'; break;
                case 't': *dst++ = '\t'; break;
                case '"': *dst++ = '"'; break;
                case '\\': *dst++ = '\\'; break;
                case '/': *dst++ = '/'; break;
                default: *dst++ = *src; break;
            }
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';

    p->ptr++;  /* Skip closing quote */
    return str;
}

/*
 * Parse a number
 */
static double parse_number_value(parser_t *p)
{
    char *end;
    double val = strtod(p->ptr, &end);
    p->ptr = end;
    return val;
}

/*
 * Parse an array
 */
static json_value_t *parse_array(parser_t *p)
{
    if (*p->ptr != '[') return NULL;
    p->ptr++;

    json_value_t *arr = (json_value_t *)calloc(1, sizeof(json_value_t));
    if (!arr) return NULL;

    arr->type = JSON_ARRAY;
    arr->data.array.capacity = 8;
    arr->data.array.items = (json_value_t **)calloc(arr->data.array.capacity,
                                                     sizeof(json_value_t *));
    if (!arr->data.array.items) {
        free(arr);
        return NULL;
    }

    skip_whitespace(p);

    while (*p->ptr && *p->ptr != ']') {
        json_value_t *item = parse_value(p);
        if (!item) break;

        /* Grow array if needed */
        if (arr->data.array.count >= arr->data.array.capacity) {
            int new_cap = arr->data.array.capacity * 2;
            json_value_t **new_items = (json_value_t **)realloc(
                arr->data.array.items, new_cap * sizeof(json_value_t *));
            if (!new_items) {
                json_free(item);
                break;
            }
            arr->data.array.items = new_items;
            arr->data.array.capacity = new_cap;
        }

        arr->data.array.items[arr->data.array.count++] = item;

        skip_whitespace(p);
        if (*p->ptr == ',') {
            p->ptr++;
            skip_whitespace(p);
        }
    }

    if (*p->ptr == ']') p->ptr++;

    return arr;
}

/*
 * Parse an object
 */
static json_value_t *parse_object(parser_t *p)
{
    if (*p->ptr != '{') return NULL;
    p->ptr++;

    json_value_t *obj = (json_value_t *)calloc(1, sizeof(json_value_t));
    if (!obj) return NULL;

    obj->type = JSON_OBJECT;
    obj->data.object.capacity = 8;
    obj->data.object.keys = (char **)calloc(obj->data.object.capacity, sizeof(char *));
    obj->data.object.values = (json_value_t **)calloc(obj->data.object.capacity,
                                                       sizeof(json_value_t *));

    if (!obj->data.object.keys || !obj->data.object.values) {
        free(obj->data.object.keys);
        free(obj->data.object.values);
        free(obj);
        return NULL;
    }

    skip_whitespace(p);

    while (*p->ptr && *p->ptr != '}') {
        /* Parse key */
        char *key = parse_string_value(p);
        if (!key) break;

        skip_whitespace(p);
        if (*p->ptr != ':') {
            free(key);
            break;
        }
        p->ptr++;
        skip_whitespace(p);

        /* Parse value */
        json_value_t *value = parse_value(p);
        if (!value) {
            free(key);
            break;
        }

        /* Grow object if needed */
        if (obj->data.object.count >= obj->data.object.capacity) {
            int new_cap = obj->data.object.capacity * 2;
            char **new_keys = (char **)realloc(obj->data.object.keys,
                                                new_cap * sizeof(char *));
            json_value_t **new_values = (json_value_t **)realloc(
                obj->data.object.values, new_cap * sizeof(json_value_t *));

            if (!new_keys || !new_values) {
                free(key);
                json_free(value);
                break;
            }

            obj->data.object.keys = new_keys;
            obj->data.object.values = new_values;
            obj->data.object.capacity = new_cap;
        }

        obj->data.object.keys[obj->data.object.count] = key;
        obj->data.object.values[obj->data.object.count] = value;
        obj->data.object.count++;

        skip_whitespace(p);
        if (*p->ptr == ',') {
            p->ptr++;
            skip_whitespace(p);
        }
    }

    if (*p->ptr == '}') p->ptr++;

    return obj;
}

/*
 * Parse a JSON value
 */
static json_value_t *parse_value(parser_t *p)
{
    skip_whitespace(p);

    if (*p->ptr == '\0') return NULL;

    json_value_t *val = NULL;

    switch (*p->ptr) {
        case '{':
            val = parse_object(p);
            break;

        case '[':
            val = parse_array(p);
            break;

        case '"':
            val = (json_value_t *)calloc(1, sizeof(json_value_t));
            if (val) {
                val->type = JSON_STRING;
                val->data.str_val = parse_string_value(p);
                if (!val->data.str_val) {
                    free(val);
                    val = NULL;
                }
            }
            break;

        case 't':
            if (strncmp(p->ptr, "true", 4) == 0) {
                val = (json_value_t *)calloc(1, sizeof(json_value_t));
                if (val) {
                    val->type = JSON_BOOL;
                    val->data.bool_val = true;
                }
                p->ptr += 4;
            }
            break;

        case 'f':
            if (strncmp(p->ptr, "false", 5) == 0) {
                val = (json_value_t *)calloc(1, sizeof(json_value_t));
                if (val) {
                    val->type = JSON_BOOL;
                    val->data.bool_val = false;
                }
                p->ptr += 5;
            }
            break;

        case 'n':
            if (strncmp(p->ptr, "null", 4) == 0) {
                val = (json_value_t *)calloc(1, sizeof(json_value_t));
                if (val) {
                    val->type = JSON_NULL;
                }
                p->ptr += 4;
            }
            break;

        default:
            if (*p->ptr == '-' || isdigit((unsigned char)*p->ptr)) {
                val = (json_value_t *)calloc(1, sizeof(json_value_t));
                if (val) {
                    val->type = JSON_NUMBER;
                    val->data.num_val = parse_number_value(p);
                }
            }
            break;
    }

    return val;
}

/*
 * Parse JSON string
 */
json_value_t *json_parse(const char *json_str)
{
    if (!json_str) return NULL;

    parser_t parser = {
        .json = json_str,
        .ptr = json_str,
        .error = ""
    };

    return parse_value(&parser);
}

/*
 * Free JSON value
 */
void json_free(json_value_t *value)
{
    if (!value) return;

    switch (value->type) {
        case JSON_STRING:
            free(value->data.str_val);
            break;

        case JSON_ARRAY:
            for (int i = 0; i < value->data.array.count; i++) {
                json_free(value->data.array.items[i]);
            }
            free(value->data.array.items);
            break;

        case JSON_OBJECT:
            for (int i = 0; i < value->data.object.count; i++) {
                free(value->data.object.keys[i]);
                json_free(value->data.object.values[i]);
            }
            free(value->data.object.keys);
            free(value->data.object.values);
            break;

        default:
            break;
    }

    free(value);
}

/*
 * Get string value from object
 */
const char *json_get_string(json_value_t *obj, const char *key)
{
    if (!obj || obj->type != JSON_OBJECT || !key) return NULL;

    for (int i = 0; i < obj->data.object.count; i++) {
        if (strcmp(obj->data.object.keys[i], key) == 0) {
            json_value_t *val = obj->data.object.values[i];
            if (val && val->type == JSON_STRING) {
                return val->data.str_val;
            }
            return NULL;
        }
    }
    return NULL;
}

/*
 * Get integer value from object
 */
int json_get_int(json_value_t *obj, const char *key, int default_val)
{
    if (!obj || obj->type != JSON_OBJECT || !key) return default_val;

    for (int i = 0; i < obj->data.object.count; i++) {
        if (strcmp(obj->data.object.keys[i], key) == 0) {
            json_value_t *val = obj->data.object.values[i];
            if (val && val->type == JSON_NUMBER) {
                return (int)val->data.num_val;
            }
            return default_val;
        }
    }
    return default_val;
}

/*
 * Get boolean value from object
 */
bool json_get_bool(json_value_t *obj, const char *key, bool default_val)
{
    if (!obj || obj->type != JSON_OBJECT || !key) return default_val;

    for (int i = 0; i < obj->data.object.count; i++) {
        if (strcmp(obj->data.object.keys[i], key) == 0) {
            json_value_t *val = obj->data.object.values[i];
            if (val && val->type == JSON_BOOL) {
                return val->data.bool_val;
            }
            return default_val;
        }
    }
    return default_val;
}

/*
 * Get array from object
 */
json_value_t *json_get_array(json_value_t *obj, const char *key)
{
    if (!obj || obj->type != JSON_OBJECT || !key) return NULL;

    for (int i = 0; i < obj->data.object.count; i++) {
        if (strcmp(obj->data.object.keys[i], key) == 0) {
            json_value_t *val = obj->data.object.values[i];
            if (val && val->type == JSON_ARRAY) {
                return val;
            }
            return NULL;
        }
    }
    return NULL;
}

/*
 * Get object from object
 */
json_value_t *json_get_object(json_value_t *obj, const char *key)
{
    if (!obj || obj->type != JSON_OBJECT || !key) return NULL;

    for (int i = 0; i < obj->data.object.count; i++) {
        if (strcmp(obj->data.object.keys[i], key) == 0) {
            json_value_t *val = obj->data.object.values[i];
            if (val && val->type == JSON_OBJECT) {
                return val;
            }
            return NULL;
        }
    }
    return NULL;
}

/*
 * Get array length
 */
int json_array_length(json_value_t *arr)
{
    if (!arr || arr->type != JSON_ARRAY) return 0;
    return arr->data.array.count;
}

/*
 * Get array element
 */
json_value_t *json_array_get(json_value_t *arr, int index)
{
    if (!arr || arr->type != JSON_ARRAY) return NULL;
    if (index < 0 || index >= arr->data.array.count) return NULL;
    return arr->data.array.items[index];
}
