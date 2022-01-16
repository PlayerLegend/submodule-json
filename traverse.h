#ifndef FLAT_INCLUDES
#include <stdbool.h>
#define FLAT_INCLUDES
#include "../range/def.h"
#include "../keyargs/keyargs.h"
#include "def.h"
#endif

json_value * json_lookup (const json_object * object, const char * key);
const char * json_type_name(json_type type);

#define json_get_number(...) keyargs_call(json_get_number, __VA_ARGS__)
keyargs_declare(double, json_get_number, 
		const json_object * parent;
		const char * key;
		bool * success;
		bool optional;
		double default_value;);

#define json_get_bool(...) keyargs_call(json_get_bool, __VA_ARGS__)
keyargs_declare(double, json_get_bool, 
		const json_object * parent;
		const char * key;
		bool * success;
		bool optional;
		bool default_value;);

#define json_get_string(...) keyargs_call(json_get_string, __VA_ARGS__)
keyargs_declare(const char*, json_get_string,
		const json_object * parent;
		const char * key;
		bool * success;
		bool optional;
		const char * default_value;);

#define json_get_array(...) keyargs_call(json_get_array, __VA_ARGS__)
keyargs_declare(const json_array*, json_get_array,
		const json_object * parent;
		const char * key;
		bool optional;
		bool * success;);

#define json_get_object(...) keyargs_call(json_get_object, __VA_ARGS__)
keyargs_declare(const json_object*, json_get_object,
		const json_object * parent;
		const char * key;
		bool * success;
		bool optional;);
	        
