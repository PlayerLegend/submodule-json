#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define FLAT_INCLUDES
#include "../range/def.h"
#include "../range/string.h"
#include "../window/def.h"
#include "../window/alloc.h"
#include "../keyargs/keyargs.h"
#include "def.h"
#include "parse.h"
#include "traverse.h"
#include "../log/log.h"
//#include "../list/list.h"
//#include "../libc/string-extensions.h"

#include "../table/table.h"
#define table_string_value json_value child_value
#define table_string_value_free(value) json_clear (&(value).child_value)
#include "../table/table-string.h"

struct json_object {
    table_string map;
};

window_typedef(json_value, json_value);

typedef struct {
    window_char text;
}
    json_tmp;

static bool _skip_whitespace (range_const_char * text)
{
    while (true)
    {
	if (text->begin == text->end)
	{
	    return false;
	}

	if (isspace(*text->begin))
	{
	    text->begin++;
	    continue;
	}

	return true;
    }

    return false;
}

static json_type _identify_next (range_const_char * text)
{
    if (!_skip_whitespace (text))
    {
	return JSON_BADTYPE;
    }
    
    char first_c = *text->begin;
    switch (first_c)
    {
    case '"':
	return JSON_STRING;

    case '{':
	return JSON_OBJECT;

    case '[':
	return JSON_ARRAY;

    case 't':
	return JSON_TRUE;

    case 'f':
	return JSON_FALSE;

    case 'n':
	return JSON_NULL;

    default:
	if (first_c == '-' || ('0' <= first_c && first_c <= '9'))
	{
	    return JSON_NUMBER;
	}
    }

    return JSON_BADTYPE;
}

static bool _read_number (double * output, range_const_char * text)
{
    char * endptr;

    *output = strtod (text->begin, &endptr);

    if (endptr == text->begin)
    {
	return false;
    }

    text->begin = endptr;
    
    return true;
}

static bool _read_string (window_char * string, range_const_char * text)
{
    assert (*text->begin == '"');

    text->begin++;

    bool escape = false;
    char add_c;

    window_rewrite (*string);

    while (text->begin < text->end)
    {
	if (*text->begin == '\\')
	{
	    escape = true;
	    goto next;
	}

	if (escape)
	{
	    escape = false;
	    assert(text->begin[-1] == '\\');
	    switch (*text->begin)
	    {
	    case '"':
		add_c = '"';
		goto add_c;

	    case '\\':
		add_c = '\\';
		goto add_c;

	    case '/':
		add_c = '/';
		goto add_c;

	    case 'b':
		add_c = '\b';
		goto add_c;

	    case 'f':
		add_c = '\f';
		goto add_c;

	    case 'n':
		add_c = '\n';
		goto add_c;

	    case 'r':
		add_c = '\r';
		goto add_c;

	    case 't':
		add_c = '\t';
		goto add_c;

	    case 'u':
		log_fatal ("u-hex characters are currently unsupported");

	    default:
		log_fatal ("unrecognized escape code in string (%c): %.*s", *text->begin, range_count(*text) - 1, text->begin);
	    }
	}
	else if (*text->begin == '"')
	{
	    *window_push (*string) = '\0';
	    text->begin++;
	    return true;
	}
	else
	{
	    add_c = *text->begin;
	    goto add_c;
	}

    add_c:
	*window_push (*string) = add_c;
	
    next:
	text->begin++;
    }

fail:
    log_fatal ("file ended while reading string");
    
    return false;
}

static bool _skip_string (range_const_char * text, const char * string)
{
    int len = strlen (string);
    if (range_count (*text) < len + 1)
    {
	return false;
    }

    if (0 != strncmp (text->begin, string, len))
    {
	return false;
    }

    text->begin += len;

    return true;
}

static bool _read_array (json_array * array, range_const_char * input, json_tmp * tmp);
static json_object * _read_object (range_const_char * input, json_tmp * tmp);

static bool _read_value (json_value * value, range_const_char * input, json_tmp * tmp)
{
    assert (value);
    
    *value = (json_value){0};
    
    value->type = _identify_next (input);
    //log_normal ("Read %s", json_type_name(value->type));

    switch (value->type)
    {
    case JSON_OBJECT:
	value->object = _read_object (input, tmp);
	return value->object != NULL;

    case JSON_STRING:
	if (!_read_string(&tmp->text, input))
	{
	    return false;
	}

	value->string = range_strdup_to_string (&tmp->text.region.const_cast);
	
	if (!value->string)
	{
	    perror ("malloc");
	    return false;
	}
	
	return true;

    case JSON_ARRAY:
	if (!_read_array (&value->array, input, tmp))
	{
	    return false;
	}
	return true;

    case JSON_NUMBER:
	if (!_read_number(&value->number, input))
	{
	    return false;
	}
	return true;

    case JSON_FALSE:
	return _skip_string (input, "false");
	
    case JSON_TRUE:
	return _skip_string (input, "true");
	
    case JSON_NULL:
	return _skip_string (input, "null");

    default:
    case JSON_BADTYPE:
	return false;
    }

    return false;
}

static bool _read_array (json_array * array, range_const_char * input, json_tmp * tmp)
{
    window_json_value build_array = {0};
    assert (*input->begin == '[');
    input->begin++;

    bool passed_comma = false;

    while (true)
    {
	_skip_whitespace (input);

	if (*input->begin == ']')
	{
	    if (passed_comma)
	    {
		log_fatal ("Hanging comma in array: %s", input->begin);
	    }
	    else
	    {
		goto success;
	    }
	}

	if (!_read_value (window_push (build_array), input, tmp))
	{
	    log_fatal ("Failed to read a value in the array: %s", input->begin);
	}

	passed_comma = false;

	_skip_whitespace (input);

	if (*input->begin == ',')
	{
	    input->begin++;
	    passed_comma = true;
	}
    }

fail:
    free (build_array.alloc.begin);
    *array = (range_json_value){0};
    return false;
    
success:
    range_copy(*array, build_array.region);
    free (build_array.alloc.begin);
    input->begin++;
    return true;
}

static void json_clear (json_value * value);

static void _free_object (json_object * object)
{
    table_string_clear (object->map);
    free (object);
}

static json_object * _read_object (range_const_char * text, json_tmp * tmp)
{
    json_object * object = calloc (1, sizeof(*object));
    
    table_string_resize (object->map, 1031);
	
    assert (*text->begin == '{');

    text->begin++;

    json_value * set_value;

    bool expect_pair = false;

    while (true)
    {
	if (_identify_next (text) != JSON_STRING)
	{
	    if (*text->begin == '}')
	    {
		if (expect_pair)
		{
		    goto fail;
		}
		else
		{
		    goto success;
		}
	    }
	    else
	    {
		log_fatal ("Object key is type %s, it should be a string\n%s", json_type_name (_identify_next(text)), text->begin);
	    }
	}
	
	if (!_read_string (&tmp->text, text))
	{
	    log_fatal ("JSON object key is not a string: %s", text->begin);
	}

	_skip_whitespace (text);

	if (*text->begin != ':')
	{
	    log_fatal ("Pair separator is missing within JSON object: %s", text->begin);
	}

	text->begin++;
	
	set_value = &table_string_include(object->map, tmp->text.region.begin)->value.child_value;

	if (!_read_value (set_value, text, tmp))
	{
	    goto fail;
	}

	expect_pair = false;

	_skip_whitespace (text);

	if (*text->begin == ',')
	{
	    expect_pair = true;
	    text->begin++;
	}
    }
    
fail:
    _free_object (object);
    return NULL;

success:
    text->begin++;
    return object;
}

json_value * json_parse (const range_const_char * input)
{
    range_const_char text = *input;
    json_tmp tmp = {0};

    json_value * value = calloc (1, sizeof(*value));

    if (!_read_value (value, &text, &tmp))
    {
	free (tmp.text.alloc.begin);
	json_free (value);
	return NULL;
    }

    free (tmp.text.alloc.begin);

    return value;
}

json_value * json_lookup (const json_object * object, const char * key)
{
    table_string_query query = table_string_query(key);
    table_string_bucket bucket = table_string_bucket(object->map, query);

    table_string_match(bucket, query);

    table_string_item * item = table_get_bucket_item (bucket);

    return item ? &item->value.child_value : NULL;
}

static void json_clear (json_value * value)
{
    json_value * member;
    
    switch (value->type)
    {
    case JSON_ARRAY:
	for_range(member, value->array)
	{
	    json_clear (member);
	}
	free (value->array.begin);
	break;

    case JSON_BADTYPE:
    case JSON_FALSE:
    case JSON_NULL:
    case JSON_TRUE:
    case JSON_NUMBER:
	break;

    case JSON_STRING:
	free (value->string);
	break;

    case JSON_OBJECT:
	_free_object (value->object);
	break;

    }
}

void json_free (json_value * value)
{
    if (!value)
    {
	return;
    }

    json_clear (value);
    
    free (value);
}

const char * json_type_name(json_type type)
{
    static const char * _name[] = { "null", "number", "true", "false", "string", "array", "object", "badtype" };
    return _name[type];
}

typedef struct json_number_options json_number_options;
struct json_number_options {
    double null_value;
};

keyargs_define(json_get_bool)
{
    json_value * value = json_lookup(args.parent, args.key);

    if (!value || value->type == JSON_NULL)
    {
	if (args.optional)
	{
	    return args.default_value;
	}
	
	log_fatal ("Object has no child %s", args.key);
    }

    if (value->type == JSON_TRUE)
    {
	return true;
    }
    else if (value->type == JSON_FALSE)
    {
	return false;
    }
    else
    {
	log_fatal ("Object child %s is not a boolean value", args.key);
    }
    
fail:
    if (args.success)
    {
	*args.success = false;
    }
    
    return false;
}

keyargs_define(json_get_number)
{
    json_value * value = json_lookup(args.parent, args.key);

    if (!value || value->type == JSON_NULL)
    {
	if (args.optional)
	{
	    return args.default_value;
	}
	
	log_fatal ("Object has no child %s", args.key);
    }

    if (value->type != JSON_NUMBER)
    {
	log_fatal ("Object child %s is not a number", args.key);
    }

    return value->number;

fail:
    if (args.success)
    {
	*args.success = false;
    }
    return 0;
}

keyargs_define(json_get_string)
{
    json_value * value = json_lookup(args.parent, args.key);

    if (value->type == JSON_NULL)
    {
	if (args.optional && args.default_value)
	{
	    return args.default_value;
	}
	
	log_fatal ("Object has no child %s", args.key);
    }

    if (value->type != JSON_STRING)
    {
	log_fatal ("Object child %s is not a string", args.key);
    }

    assert (value->string != NULL);

    return value->string;

fail:
    if (args.success)
    {
	*args.success = false;
    }
    
    return NULL;
}

keyargs_define(json_get_array)
{
    json_value * value = json_lookup(args.parent, args.key);

    if (!value || value->type == JSON_NULL)
    {
	if (!args.optional)
	{
	    log_fatal ("Object has no child %s", args.key);
	}
	else
	{
	    return NULL;
	}
    }

    if (value->type != JSON_ARRAY)
    {
	log_fatal ("Object child %s is not an array", args.key);
    }

    return &value->array;
    
fail:
    return NULL;
}

keyargs_define(json_get_object)
{
    json_value * value = json_lookup(args.parent, args.key);

    if (!value || value->type == JSON_NULL)
    {
	if (!args.optional)
	{
	    log_fatal ("Object has no child %s", args.key);
	}
	else
	{
	    return NULL;
	}
    }

    if (value->type != JSON_OBJECT)
    {
	log_fatal ("Object child %s is not an object", args.key);
    }

    return value->object;

fail:
    return NULL;
}
