#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#define TRUE 1
#define FALSE 0
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
#define ARRAY(T) struct {\
    int16_t size;\
    int16_t capacity;\
    T *data;\
} *
#define ARRAY_CREATE(array, init_capacity, init_size) {\
    array = malloc(sizeof(*array)); \
    array->data = malloc((init_capacity) * sizeof(*array->data)); \
    assert(array->data != NULL); \
    array->capacity = init_capacity; \
    array->size = init_size; \
}
#define ARRAY_PUSH(array, item) {\
    if (array->size == array->capacity) {  \
        array->capacity *= 2;  \
        array->data = realloc(array->data, array->capacity * sizeof(*array->data)); \
        assert(array->data != NULL); \
    }  \
    array->data[array->size++] = item; \
}
#define STR_INT16_T_BUFLEN ((CHAR_BIT * sizeof(int16_t) - 1) / 3 + 2)
void str_int16_t_cat(char *str, int16_t num) {
    char numstr[STR_INT16_T_BUFLEN];
    sprintf(numstr, "%d", num);
    strcat(str, numstr);
}
enum kashrizz_v_type {kashrizz_VAR_NULL, kashrizz_VAR_UNDEFINED, kashrizz_VAR_NAN, kashrizz_VAR_BOOL, kashrizz_VAR_INT16, kashrizz_VAR_STRING, kashrizz_VAR_ARRAY, kashrizz_VAR_DICT};
struct kashrizz_var {
    enum kashrizz_v_type type;
    int16_t number;
    void *data;
};
struct array_kashrizz_v_t {
    int16_t size;
    int16_t capacity;
    struct kashrizz_var *data;
};
struct kashrizz_var kashrizz_var_from_int16_t(int16_t n) {
    struct kashrizz_var v;
    v.type = kashrizz_VAR_INT16;
    v.number = n;
    v.data = NULL;
    return v;
}
struct kashrizz_var kashrizz_var_from_str(const char *s) {
    struct kashrizz_var v;
    v.type = kashrizz_VAR_STRING;
    v.data = (void *)s;
    return v;
}
struct kashrizz_var str_to_int16_t(const char * str) {
    struct kashrizz_var v;
    const char *p = str;
    int r;

    v.data = NULL;

    while (*p && isspace(*p))
        p++;

    if (*p == 0)
        str = "0";

    if (*p == '-' && *(p+1))
        p++;

    while (*p) {
        if (!isdigit(*p)) {
            v.type = kashrizz_VAR_NAN;
            return v;
        }
        p++;
    }

    sscanf(str, "%d", &r);
    v.type = kashrizz_VAR_INT16;
    v.number = (int16_t)r;
    return v;
}
const char * kashrizz_vts(struct kashrizz_var v, uint8_t *need_dispose)
{
    char *buf;
    int16_t i;
    *need_dispose = 0;

    if (v.type == kashrizz_VAR_INT16) {
        buf = malloc(STR_INT16_T_BUFLEN);
        assert(buf != NULL);
        *need_dispose = 1;
        sprintf(buf, "%d", v.number);
        return buf;
    } else if (v.type == kashrizz_VAR_BOOL)
        return v.number ? "true" : "false";
    else if (v.type == kashrizz_VAR_STRING)
        return (const char *)v.data;
    else if (v.type == kashrizz_VAR_ARRAY) {
        struct array_kashrizz_v_t * arr = (struct array_kashrizz_v_t *)v.data;
        uint8_t dispose_elem = 0;
        buf = malloc(1);
        assert(buf != NULL);
        *need_dispose = 1;
        buf[0] = 0;
        for (i = 0; i < arr->size; i++) {
            const char * elem = kashrizz_vts(arr->data[i], &dispose_elem);
            buf = realloc(buf, strlen(buf) + strlen(elem) + 1 + (i != 0 ? 1 : 0));
            assert(buf != NULL);
            if (i != 0)
                strcat(buf, ",");
            strcat(buf, elem);
            if (dispose_elem)
                free((void *)elem);
        }
        return buf;
    }
    else if (v.type == kashrizz_VAR_DICT)
        return "[object Object]";
    else if (v.type == kashrizz_VAR_NAN)
        return "NaN";
    else if (v.type == kashrizz_VAR_NULL)
        return "null";
    else if (v.type == kashrizz_VAR_UNDEFINED)
        return "undefined";

    return NULL;
}

struct kashrizz_var kashrizz_vtn(struct kashrizz_var v)
{
    struct kashrizz_var result;
    result.type = kashrizz_VAR_INT16;
    result.number = 0;

    if (v.type == kashrizz_VAR_INT16)
        result.number = v.number;
    else if (v.type == kashrizz_VAR_BOOL)
        result.number = v.number;
    else if (v.type == kashrizz_VAR_STRING)
        return str_to_int16_t((const char *)v.data);
    else if (v.type == kashrizz_VAR_ARRAY) {
        struct array_kashrizz_v_t * arr = (struct array_kashrizz_v_t *)v.data;
        if (arr->size == 0)
            result.number = 0;
        else if (arr->size > 1)
            result.type = kashrizz_VAR_NAN;
        else
            result = kashrizz_vtn(arr->data[0]);
    } else if (v.type != kashrizz_VAR_NULL)
        result.type = kashrizz_VAR_NAN;

    return result;
}
static ARRAY(void *) gc_main;

struct kashrizz_var kashrizz_vp(struct kashrizz_var left, struct kashrizz_var right)
{
    struct kashrizz_var result, left_to_number, right_to_number;
    const char *left_as_string, *right_as_string;
    uint8_t need_dispose_left, need_dispose_right;
    result.data = NULL;

    if (left.type == kashrizz_VAR_STRING || right.type == kashrizz_VAR_STRING 
        || left.type == kashrizz_VAR_ARRAY || right.type == kashrizz_VAR_ARRAY
        || left.type == kashrizz_VAR_DICT || right.type == kashrizz_VAR_DICT)
    {
        left_as_string = kashrizz_vts(left, &need_dispose_left);
        right_as_string = kashrizz_vts(right, &need_dispose_right);
        
        result.type = kashrizz_VAR_STRING;
        result.data = malloc(strlen(left_as_string) + strlen(right_as_string) + 1);
        assert(result.data != NULL);
        ARRAY_PUSH(gc_main, result.data);

        strcpy(result.data, left_as_string);
        strcat(result.data, right_as_string);

        if (need_dispose_left)
            free((void *)left_as_string);
        if (need_dispose_right)
            free((void *)right_as_string);
        return result;
    }

    left_to_number = kashrizz_vtn(left);
    right_to_number = kashrizz_vtn(right);

    if (left_to_number.type == kashrizz_VAR_NAN || right_to_number.type == kashrizz_VAR_NAN) {
        result.type = kashrizz_VAR_NAN;
        return result;
    }

    result.type = kashrizz_VAR_INT16;
    result.number = left_to_number.number + right_to_number.number;
    return result;
}

enum kashrizz_var_op {kashrizz_VAR_MINUS, kashrizz_VAR_ASTERISK, kashrizz_VAR_SLASH, kashrizz_VAR_PERCENT, kashrizz_VAR_SHL, kashrizz_VAR_SHR, kashrizz_VAR_USHR, kashrizz_VAR_OR, kashrizz_VAR_AND};
struct kashrizz_var kashrizz_var_compute(struct kashrizz_var left, enum kashrizz_var_op op, struct kashrizz_var right)
{
    struct kashrizz_var result, left_to_number, right_to_number;
    result.data = NULL;

    left_to_number = kashrizz_vtn(left);
    right_to_number = kashrizz_vtn(right);

    if (left_to_number.type == kashrizz_VAR_NAN || right_to_number.type == kashrizz_VAR_NAN) {
        if (op == kashrizz_VAR_MINUS || op == kashrizz_VAR_ASTERISK || op == kashrizz_VAR_SLASH || op == kashrizz_VAR_PERCENT) {
            result.type = kashrizz_VAR_NAN;
            return result;
        }
    }
    
    result.type = kashrizz_VAR_INT16;
    switch (op) {
        case kashrizz_VAR_MINUS:
            result.number = left_to_number.number - right_to_number.number;
            break;
        case kashrizz_VAR_ASTERISK:
            result.number = left_to_number.number * right_to_number.number;
            break;
        case kashrizz_VAR_SLASH:
            result.number = left_to_number.number / right_to_number.number;
            break;
        case kashrizz_VAR_PERCENT:
            result.number = left_to_number.number % right_to_number.number;
            break;
        case kashrizz_VAR_SHL:
            result.number = left_to_number.number << right_to_number.number;
            break;
        case kashrizz_VAR_SHR:
            result.number = left_to_number.number >> right_to_number.number;
            break;
        case kashrizz_VAR_USHR:
            result.number = ((uint16_t)left_to_number.number) >> right_to_number.number;
            break;
        case kashrizz_VAR_AND:
            result.number = left_to_number.number & right_to_number.number;
            break;
        case kashrizz_VAR_OR:
            result.number = left_to_number.number | right_to_number.number;
            break;
    }
    return result;
}
int16_t gc_i;

static struct kashrizz_var IP;
static char * tmp_result_2 = NULL;
static char * tmp_result = NULL;
static int16_t F;
static const char * binary;
static const char * xyz;
static const char * tmp_str;
static uint8_t tmp_need_dispose;
int main(void) {
    ARRAY_CREATE(gc_main, 2, 0);

    tmp_result_2 = malloc(strlen(xyz) + strlen(binary) + 1);
    assert(tmp_result_2 != NULL);
    tmp_result_2[0] = '\0';
    strcat(tmp_result_2, xyz);
    strcat(tmp_result_2, binary);
    tmp_result = malloc(strlen(tmp_result_2) + STR_INT16_T_BUFLEN + 1);
    assert(tmp_result != NULL);
    tmp_result[0] = '\0';
    strcat(tmp_result, tmp_result_2);
    str_int16_t_cat(tmp_result, 200 * F);
    IP = kashrizz_var_compute(kashrizz_vp(kashrizz_var_compute(kashrizz_var_from_str(tmp_result), kashrizz_VAR_MINUS, kashrizz_var_from_int16_t(50)), kashrizz_var_from_int16_t(50)), kashrizz_VAR_MINUS, kashrizz_var_from_int16_t(250));
    F = 1;
    binary = "000000000000000000000000000000110010000010000000000010000000000000000000000010000000000000000000000000000000011011011011001111010010010000000000000000000000000000000000000000000100010000000000000000000000000000000000100011011000000100010010000000000000100001001001001001001001001001001001001001001100100011011100010010000000000000000000000000000000000000000000000000000000000000100001001001001001001001001001001100000000000000000000100001001001100011011100010001001001001001001001001001001001100000000000000000000000100011100010010000000000000000000000100001001001001001001001001001001100000000000000000000100011011100010010001001001001001001001001100000000000000000000000000100001001100000100011011100010010100001001001001001100011011100010010001001001001001001001001001001001001100000000000000000000000000000100001001001100001001001001001001100000000000000000000000000100011011100010010000000000000100001100011011100010010000000000000000000100001001001001001001001001001001001001100000100000000000000000000000000000000100011011100010010001001001001001001001100001001001100000000000000000100001001001100011011100010000000000000000000000000000000000100010001001001100000000000000000000000000000000000000100100001001001001100000000000100011011000000000000000000000000000000000000000000000000000000000000000000000000000000100001001001001001001001001001001001100100010010001001001001001001001001100011000000000000000000100010000000000000000000000000100011000000000000000000000100010001100011000100010000000000000000000000000100100011011001100010000000000100000000000100010001001001001001100011001001001001001001100000000000000100001100011100010000000100001001001001001001001001001100010001100011001001001001001001001001100";
    xyz = "1030.116, 394, 442.9482";
    printf("%s\n", tmp_str = kashrizz_vts(IP, &tmp_need_dispose));
    if (tmp_need_dispose)
        free((void *)tmp_str);
    free((char *)tmp_result);
    free((char *)tmp_result_2);
    for (gc_i = 0; gc_i < gc_main->size; gc_i++)
        free(gc_main->data[gc_i]);
    free(gc_main->data);
    free(gc_main);

    return 0;
}
