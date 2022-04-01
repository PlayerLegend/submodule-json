#ifndef FLAT_INCLUDES
#include "../table/string.h"
#endif

map_string_type_declare(json);

typedef struct json_table json_object;

typedef enum json_type {
    JSON_NULL,
    JSON_NUMBER,
    JSON_TRUE,
    JSON_FALSE,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT,
    JSON_BADTYPE,
}
    json_type;

typedef struct json_value json_value;

range_typedef(json_value, json_value);
typedef range_json_value json_array;

struct json_value {
    json_type type;
    union {
	double number;
	char * string;
	json_array array;
	json_object * object;
    };
};

map_string_type_define(json);
map_string_function_declare(json);
#define json_object_clear json_table_clear

void json_value_clear(json_value * value);

map_string_function_define(json);
void json_array_clear(json_array * array);
