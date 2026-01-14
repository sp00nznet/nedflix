/*
 * Nedflix for Original Xbox
 * Simple JSON parser
 *
 * This is a minimal JSON parser for the Xbox's limited memory.
 * Supports: objects, arrays, strings, numbers, booleans, null
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

/* JSON key-value pair for objects */
typedef struct json_pair {
    char *key;
    struct json_value *value;
    struct json_pair *next;
} json_pair_t;

/* JSON array element */
typedef struct json_element {
    struct json_value *value;
    struct json_element *next;
} json_element_t;

/* JSON value structure */
struct json_value {
    json_type_t type;
    union {
        bool bool_value;
        double number_value;
        char *string_value;
        json_element_t *array_value;
        json_pair_t *object_value;
    };
};

/* Parser state */
typedef struct {
    const char *text;
    size_t pos;
    size_t length;
} parser_t;

/* Forward declarations */
static json_value_t *parse_value(parser_t *p);

/*
 * Skip whitespace
 */
static void skip_whitespace(parser_t *p)
{
    while (p->pos < p->length && isspace((unsigned char)p->text[p->pos])) {
        p->pos++;
    }
}

/*
 * Peek current character
 */
static char peek(parser_t *p)
{
    if (p->pos >= p->length) return '\0';
    return p->text[p->pos];
}

/*
 * Consume current character
 */
static char consume(parser_t *p)
{
    if (p->pos >= p->length) return '\0';
    return p->text[p->pos++];
}

/*
 * Parse string
 */
static char *parse_string(parser_t *p)
{
    if (consume(p) != '"') return NULL;

    size_t start = p->pos;
    size_t len = 0;

    /* Find end of string */
    while (p->pos < p->length) {
        char c = p->text[p->pos];
        if (c == '"') break;
        if (c == '\\') {
            p->pos += 2;  /* Skip escape sequence */
            len += 1;     /* Escaped char counts as 1 */
        } else {
            p->pos++;
            len++;
        }
    }

    if (p->pos >= p->length) return NULL;

    /* Allocate and copy string */
    char *str = (char *)malloc(len + 1);
    if (!str) return NULL;

    size_t src = start;
    size_t dst = 0;

    while (src < p->pos) {
        char c = p->text[src++];
        if (c == '\\' && src < p->pos) {
            char escaped = p->text[src++];
            switch (escaped) {
                case 'n': str[dst++] = '\n'; break;
                case 'r': str[dst++] = '\r'; break;
                case 't': str[dst++] = '\t'; break;
                case '"': str[dst++] = '"'; break;
                case '\\': str[dst++] = '\\'; break;
                case '/': str[dst++] = '/'; break;
                default: str[dst++] = escaped; break;
            }
        } else {
            str[dst++] = c;
        }
    }
    str[dst] = '\0';

    consume(p);  /* Consume closing quote */
    return str;
}

/*
 * Parse number
 */
static double parse_number(parser_t *p)
{
    size_t start = p->pos;

    /* Skip sign */
    if (peek(p) == '-') p->pos++;

    /* Integer part */
    while (isdigit((unsigned char)peek(p))) p->pos++;

    /* Decimal part */
    if (peek(p) == '.') {
        p->pos++;
        while (isdigit((unsigned char)peek(p))) p->pos++;
    }

    /* Exponent */
    if (peek(p) == 'e' || peek(p) == 'E') {
        p->pos++;
        if (peek(p) == '+' || peek(p) == '-') p->pos++;
        while (isdigit((unsigned char)peek(p))) p->pos++;
    }

    /* Convert */
    char buf[64];
    size_t len = p->pos - start;
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    strncpy(buf, p->text + start, len);
    buf[len] = '\0';

    return atof(buf);
}

/*
 * Parse array
 */
static json_element_t *parse_array(parser_t *p)
{
    if (consume(p) != '[') return NULL;

    skip_whitespace(p);

    if (peek(p) == ']') {
        consume(p);
        return NULL;  /* Empty array */
    }

    json_element_t *head = NULL;
    json_element_t *tail = NULL;

    while (1) {
        skip_whitespace(p);

        json_value_t *value = parse_value(p);
        if (!value) break;

        json_element_t *elem = (json_element_t *)malloc(sizeof(json_element_t));
        if (!elem) {
            json_free(value);
            break;
        }

        elem->value = value;
        elem->next = NULL;

        if (tail) {
            tail->next = elem;
            tail = elem;
        } else {
            head = tail = elem;
        }

        skip_whitespace(p);

        if (peek(p) == ']') {
            consume(p);
            break;
        }

        if (peek(p) != ',') break;
        consume(p);
    }

    return head;
}

/*
 * Parse object
 */
static json_pair_t *parse_object(parser_t *p)
{
    if (consume(p) != '{') return NULL;

    skip_whitespace(p);

    if (peek(p) == '}') {
        consume(p);
        return NULL;  /* Empty object */
    }

    json_pair_t *head = NULL;
    json_pair_t *tail = NULL;

    while (1) {
        skip_whitespace(p);

        /* Parse key */
        if (peek(p) != '"') break;
        char *key = parse_string(p);
        if (!key) break;

        skip_whitespace(p);

        /* Expect colon */
        if (consume(p) != ':') {
            free(key);
            break;
        }

        skip_whitespace(p);

        /* Parse value */
        json_value_t *value = parse_value(p);
        if (!value) {
            free(key);
            break;
        }

        json_pair_t *pair = (json_pair_t *)malloc(sizeof(json_pair_t));
        if (!pair) {
            free(key);
            json_free(value);
            break;
        }

        pair->key = key;
        pair->value = value;
        pair->next = NULL;

        if (tail) {
            tail->next = pair;
            tail = pair;
        } else {
            head = tail = pair;
        }

        skip_whitespace(p);

        if (peek(p) == '}') {
            consume(p);
            break;
        }

        if (peek(p) != ',') break;
        consume(p);
    }

    return head;
}

/*
 * Parse value
 */
static json_value_t *parse_value(parser_t *p)
{
    skip_whitespace(p);

    json_value_t *value = (json_value_t *)malloc(sizeof(json_value_t));
    if (!value) return NULL;

    char c = peek(p);

    if (c == '"') {
        value->type = JSON_STRING;
        value->string_value = parse_string(p);
        if (!value->string_value) {
            free(value);
            return NULL;
        }
    } else if (c == '{') {
        value->type = JSON_OBJECT;
        value->object_value = parse_object(p);
    } else if (c == '[') {
        value->type = JSON_ARRAY;
        value->array_value = parse_array(p);
    } else if (c == 't' && strncmp(p->text + p->pos, "true", 4) == 0) {
        value->type = JSON_BOOL;
        value->bool_value = true;
        p->pos += 4;
    } else if (c == 'f' && strncmp(p->text + p->pos, "false", 5) == 0) {
        value->type = JSON_BOOL;
        value->bool_value = false;
        p->pos += 5;
    } else if (c == 'n' && strncmp(p->text + p->pos, "null", 4) == 0) {
        value->type = JSON_NULL;
        p->pos += 4;
    } else if (c == '-' || isdigit((unsigned char)c)) {
        value->type = JSON_NUMBER;
        value->number_value = parse_number(p);
    } else {
        free(value);
        return NULL;
    }

    return value;
}

/*
 * Public API
 */

json_value_t *json_parse(const char *text)
{
    if (!text) return NULL;

    parser_t p = {
        .text = text,
        .pos = 0,
        .length = strlen(text)
    };

    return parse_value(&p);
}

void json_free(json_value_t *value)
{
    if (!value) return;

    switch (value->type) {
        case JSON_STRING:
            free(value->string_value);
            break;

        case JSON_ARRAY: {
            json_element_t *elem = value->array_value;
            while (elem) {
                json_element_t *next = elem->next;
                json_free(elem->value);
                free(elem);
                elem = next;
            }
            break;
        }

        case JSON_OBJECT: {
            json_pair_t *pair = value->object_value;
            while (pair) {
                json_pair_t *next = pair->next;
                free(pair->key);
                json_free(pair->value);
                free(pair);
                pair = next;
            }
            break;
        }

        default:
            break;
    }

    free(value);
}

const char *json_get_string(json_value_t *obj, const char *key)
{
    if (!obj || obj->type != JSON_OBJECT) return NULL;

    json_pair_t *pair = obj->object_value;
    while (pair) {
        if (strcmp(pair->key, key) == 0) {
            if (pair->value && pair->value->type == JSON_STRING) {
                return pair->value->string_value;
            }
            return NULL;
        }
        pair = pair->next;
    }
    return NULL;
}

int json_get_int(json_value_t *obj, const char *key, int default_val)
{
    if (!obj || obj->type != JSON_OBJECT) return default_val;

    json_pair_t *pair = obj->object_value;
    while (pair) {
        if (strcmp(pair->key, key) == 0) {
            if (pair->value && pair->value->type == JSON_NUMBER) {
                return (int)pair->value->number_value;
            }
            return default_val;
        }
        pair = pair->next;
    }
    return default_val;
}

bool json_get_bool(json_value_t *obj, const char *key, bool default_val)
{
    if (!obj || obj->type != JSON_OBJECT) return default_val;

    json_pair_t *pair = obj->object_value;
    while (pair) {
        if (strcmp(pair->key, key) == 0) {
            if (pair->value && pair->value->type == JSON_BOOL) {
                return pair->value->bool_value;
            }
            return default_val;
        }
        pair = pair->next;
    }
    return default_val;
}

json_value_t *json_get_object(json_value_t *obj, const char *key)
{
    if (!obj || obj->type != JSON_OBJECT) return NULL;

    json_pair_t *pair = obj->object_value;
    while (pair) {
        if (strcmp(pair->key, key) == 0) {
            if (pair->value && pair->value->type == JSON_OBJECT) {
                return pair->value;
            }
            return NULL;
        }
        pair = pair->next;
    }
    return NULL;
}

json_value_t *json_get_array(json_value_t *obj, const char *key)
{
    if (!obj || obj->type != JSON_OBJECT) return NULL;

    json_pair_t *pair = obj->object_value;
    while (pair) {
        if (strcmp(pair->key, key) == 0) {
            if (pair->value && pair->value->type == JSON_ARRAY) {
                return pair->value;
            }
            return NULL;
        }
        pair = pair->next;
    }
    return NULL;
}

int json_array_length(json_value_t *array)
{
    if (!array || array->type != JSON_ARRAY) return 0;

    int count = 0;
    json_element_t *elem = array->array_value;
    while (elem) {
        count++;
        elem = elem->next;
    }
    return count;
}

json_value_t *json_array_get(json_value_t *array, int index)
{
    if (!array || array->type != JSON_ARRAY || index < 0) return NULL;

    json_element_t *elem = array->array_value;
    while (elem && index > 0) {
        elem = elem->next;
        index--;
    }

    return elem ? elem->value : NULL;
}
