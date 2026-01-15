/*
 * Nedflix for Xbox 360
 * Minimal JSON parser
 *
 * TECHNICAL DEMO / NOVELTY PORT
 */

#include "nedflix.h"

/* JSON value types */
typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} json_type_t;

/* JSON value */
struct json_value {
    json_type_t type;
    union {
        bool bool_val;
        double num_val;
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

/* Parser state */
typedef struct {
    const char *text;
    const char *pos;
} parser_t;

static void skip_whitespace(parser_t *p)
{
    while (*p->pos && (*p->pos == ' ' || *p->pos == '\t' ||
           *p->pos == '\n' || *p->pos == '\r')) {
        p->pos++;
    }
}

static json_value_t *parse_value(parser_t *p);

static char *parse_string_value(parser_t *p)
{
    if (*p->pos != '"') return NULL;
    p->pos++;

    const char *start = p->pos;
    while (*p->pos && *p->pos != '"') {
        if (*p->pos == '\\') p->pos++;
        if (*p->pos) p->pos++;
    }

    size_t len = p->pos - start;
    char *str = (char *)malloc(len + 1);
    if (str) {
        memcpy(str, start, len);
        str[len] = '\0';
    }

    if (*p->pos == '"') p->pos++;
    return str;
}

static json_value_t *parse_string(parser_t *p)
{
    json_value_t *val = (json_value_t *)calloc(1, sizeof(json_value_t));
    if (!val) return NULL;

    val->type = JSON_STRING;
    val->str_val = parse_string_value(p);

    if (!val->str_val) {
        free(val);
        return NULL;
    }

    return val;
}

static json_value_t *parse_number(parser_t *p)
{
    json_value_t *val = (json_value_t *)calloc(1, sizeof(json_value_t));
    if (!val) return NULL;

    val->type = JSON_NUMBER;
    val->num_val = strtod(p->pos, (char **)&p->pos);
    return val;
}

static json_value_t *parse_array(parser_t *p)
{
    if (*p->pos != '[') return NULL;
    p->pos++;

    json_value_t *val = (json_value_t *)calloc(1, sizeof(json_value_t));
    if (!val) return NULL;

    val->type = JSON_ARRAY;
    val->array.items = NULL;
    val->array.count = 0;

    skip_whitespace(p);

    if (*p->pos == ']') {
        p->pos++;
        return val;
    }

    int capacity = 8;
    val->array.items = (json_value_t **)malloc(sizeof(json_value_t *) * capacity);

    while (*p->pos) {
        skip_whitespace(p);

        json_value_t *item = parse_value(p);
        if (!item) break;

        if (val->array.count >= capacity) {
            capacity *= 2;
            val->array.items = realloc(val->array.items,
                                       sizeof(json_value_t *) * capacity);
        }
        val->array.items[val->array.count++] = item;

        skip_whitespace(p);

        if (*p->pos == ',') {
            p->pos++;
        } else if (*p->pos == ']') {
            p->pos++;
            break;
        } else {
            break;
        }
    }

    return val;
}

static json_value_t *parse_object(parser_t *p)
{
    if (*p->pos != '{') return NULL;
    p->pos++;

    json_value_t *val = (json_value_t *)calloc(1, sizeof(json_value_t));
    if (!val) return NULL;

    val->type = JSON_OBJECT;
    val->object.keys = NULL;
    val->object.values = NULL;
    val->object.count = 0;

    skip_whitespace(p);

    if (*p->pos == '}') {
        p->pos++;
        return val;
    }

    int capacity = 8;
    val->object.keys = (char **)malloc(sizeof(char *) * capacity);
    val->object.values = (json_value_t **)malloc(sizeof(json_value_t *) * capacity);

    while (*p->pos) {
        skip_whitespace(p);

        char *key = parse_string_value(p);
        if (!key) break;

        skip_whitespace(p);
        if (*p->pos != ':') {
            free(key);
            break;
        }
        p->pos++;

        skip_whitespace(p);
        json_value_t *value = parse_value(p);
        if (!value) {
            free(key);
            break;
        }

        if (val->object.count >= capacity) {
            capacity *= 2;
            val->object.keys = realloc(val->object.keys,
                                       sizeof(char *) * capacity);
            val->object.values = realloc(val->object.values,
                                         sizeof(json_value_t *) * capacity);
        }

        val->object.keys[val->object.count] = key;
        val->object.values[val->object.count] = value;
        val->object.count++;

        skip_whitespace(p);

        if (*p->pos == ',') {
            p->pos++;
        } else if (*p->pos == '}') {
            p->pos++;
            break;
        } else {
            break;
        }
    }

    return val;
}

static json_value_t *parse_value(parser_t *p)
{
    skip_whitespace(p);

    if (*p->pos == '"') {
        return parse_string(p);
    } else if (*p->pos == '{') {
        return parse_object(p);
    } else if (*p->pos == '[') {
        return parse_array(p);
    } else if (*p->pos == 't' && strncmp(p->pos, "true", 4) == 0) {
        p->pos += 4;
        json_value_t *val = (json_value_t *)calloc(1, sizeof(json_value_t));
        if (val) { val->type = JSON_BOOL; val->bool_val = true; }
        return val;
    } else if (*p->pos == 'f' && strncmp(p->pos, "false", 5) == 0) {
        p->pos += 5;
        json_value_t *val = (json_value_t *)calloc(1, sizeof(json_value_t));
        if (val) { val->type = JSON_BOOL; val->bool_val = false; }
        return val;
    } else if (*p->pos == 'n' && strncmp(p->pos, "null", 4) == 0) {
        p->pos += 4;
        json_value_t *val = (json_value_t *)calloc(1, sizeof(json_value_t));
        if (val) val->type = JSON_NULL;
        return val;
    } else if (*p->pos == '-' || (*p->pos >= '0' && *p->pos <= '9')) {
        return parse_number(p);
    }

    return NULL;
}

json_value_t *json_parse(const char *text)
{
    if (!text) return NULL;

    parser_t p;
    p.text = text;
    p.pos = text;

    return parse_value(&p);
}

void json_free(json_value_t *v)
{
    if (!v) return;

    switch (v->type) {
        case JSON_STRING:
            free(v->str_val);
            break;
        case JSON_ARRAY:
            for (int i = 0; i < v->array.count; i++) {
                json_free(v->array.items[i]);
            }
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
    if (!obj || obj->type != JSON_OBJECT || !key) return NULL;

    for (int i = 0; i < obj->object.count; i++) {
        if (strcmp(obj->object.keys[i], key) == 0) {
            if (obj->object.values[i]->type == JSON_STRING) {
                return obj->object.values[i]->str_val;
            }
            break;
        }
    }

    return NULL;
}

int json_get_int(json_value_t *obj, const char *key, int def)
{
    if (!obj || obj->type != JSON_OBJECT || !key) return def;

    for (int i = 0; i < obj->object.count; i++) {
        if (strcmp(obj->object.keys[i], key) == 0) {
            if (obj->object.values[i]->type == JSON_NUMBER) {
                return (int)obj->object.values[i]->num_val;
            }
            break;
        }
    }

    return def;
}

bool json_get_bool(json_value_t *obj, const char *key, bool def)
{
    if (!obj || obj->type != JSON_OBJECT || !key) return def;

    for (int i = 0; i < obj->object.count; i++) {
        if (strcmp(obj->object.keys[i], key) == 0) {
            if (obj->object.values[i]->type == JSON_BOOL) {
                return obj->object.values[i]->bool_val;
            }
            break;
        }
    }

    return def;
}

json_value_t *json_get_array(json_value_t *obj, const char *key)
{
    if (!obj || obj->type != JSON_OBJECT || !key) return NULL;

    for (int i = 0; i < obj->object.count; i++) {
        if (strcmp(obj->object.keys[i], key) == 0) {
            if (obj->object.values[i]->type == JSON_ARRAY) {
                return obj->object.values[i];
            }
            break;
        }
    }

    return NULL;
}

int json_array_length(json_value_t *arr)
{
    if (!arr || arr->type != JSON_ARRAY) return 0;
    return arr->array.count;
}

json_value_t *json_array_get(json_value_t *arr, int idx)
{
    if (!arr || arr->type != JSON_ARRAY) return NULL;
    if (idx < 0 || idx >= arr->array.count) return NULL;
    return arr->array.items[idx];
}
