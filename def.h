#ifndef FLAT_INCLUDES
#include <stdbool.h>
#define FLAT_INCLUDES
#include "../range/def.h"
#include "../keyargs/keyargs.h"
#endif

typedef struct json_value json_value;

range_typedef(json_value, json_value);

typedef range_json_value json_array;

typedef struct json_object json_object;
typedef enum json_type json_type;

enum json_type {
    JSON_NULL,
    JSON_NUMBER,
    JSON_TRUE,
    JSON_FALSE,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT,
    JSON_BADTYPE,
};

struct json_value {
    json_type type;
    union {
	double number;
	char * string;
	json_array array;
	json_object * object;
    };
};
