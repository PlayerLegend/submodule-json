#include "../json.c"

typedef struct json_object_key_value json_object_key_value;
struct json_object_key_value {
    const char * key;
    json_value value;
};

typedef struct range(json_object_key_value) range_json_object_key_value;

static void _bound_text (range_const_char * range, const char * text)
{
    range->begin = text;
    range->end = text + strlen (text);
}

static void _test_identify_string ()
{
    range_const_char text;

    _bound_text (&text, "   \"this is a string\"   ");

    assert (_identify_next(&text) == JSON_STRING);
    assert (text.begin[0] == '"');
    assert (text.begin[1] == 't');
}

static void _test_identify_number ()
{
    range_const_char text;

    _bound_text (&text, "    -0.05   ");

    assert (_identify_next(&text) == JSON_NUMBER);
    assert (text.begin[0] == '-');

    _bound_text (&text, "  0.5  ");

    assert (_identify_next(&text) == JSON_NUMBER);
    assert (text.begin[0] == '0');

    _bound_text (&text, "  573.2e+3  ");

    assert (_identify_next(&text) == JSON_NUMBER);
    assert (text.begin[0] == '5');
}

static void _test_identify_true_false_null ()
{
    range_const_char text;

    _bound_text (&text, "   true   ");
    assert (_identify_next(&text) == JSON_TRUE);

    _bound_text (&text, "   false   ");
    assert (_identify_next(&text) == JSON_FALSE);

    _bound_text (&text, "   null   ");
    assert (_identify_next(&text) == JSON_NULL);
}

static void _test_identify_object ()
{
    range_const_char text;

    _bound_text (&text, "   {}   ");
    assert (_identify_next(&text) == JSON_OBJECT);
    assert (text.begin[0] == '{');

    _bound_text (&text, "   { }   ");
    assert (_identify_next(&text) == JSON_OBJECT);
    assert (text.begin[0] == '{');

    _bound_text (&text, "   { \"asdf\" : true }   ");
    assert (_identify_next(&text) == JSON_OBJECT);
    assert (text.begin[0] == '{');
}

static void _test_identify_array ()
{
    range_const_char text;

    _bound_text (&text, "   []   ");
    assert (_identify_next(&text) == JSON_ARRAY);
    assert (text.begin[0] == '[');

    _bound_text (&text, "   [{ }]   ");
    assert (_identify_next(&text) == JSON_ARRAY);
    assert (text.begin[0] == '[');

    _bound_text (&text, "  [ { \"asdf\" : true } ]  ");
    assert (_identify_next(&text) == JSON_ARRAY);
    assert (text.begin[0] == '[');
}

static void _test_identify_next ()
{
    _test_identify_array();
    _test_identify_number();
    _test_identify_object();
    _test_identify_string();
    _test_identify_true_false_null();
}

static void _test_string (const char * input, const char * expect, const char * remaining)
{
    range_const_char text;
    window_char buffer = {0};
    _bound_text (&text, input);
    assert (_identify_next(&text) == JSON_STRING);
    assert (_read_string(&buffer, &text));
    assert (0 == strcmp (buffer.region.begin, expect));
    assert (0 == strcmp (text.begin, remaining));

    free (buffer.alloc.begin);
}

static void _test_number (const char * string, double should_be, const char * remaining)
{
    double have = 0;
    double radius = 0.001;

    range_const_char text;
    _bound_text (&text, string);

    assert (_identify_next(&text) == JSON_NUMBER);
    
    assert (_read_number(&have, &text));

    assert (have - should_be < radius && should_be - have < radius);

    assert (0 == strcmp (text.begin, remaining));
}

static void _test_skip_whitespace (bool retval, const char * input, const char * remaining)
{
    range_const_char text;
    _bound_text (&text, input);
    assert (retval == _skip_whitespace (&text));
    assert (0 == strcmp (text.begin, remaining));
}

static void _compare_values (json_value * a, json_value * b)
{
    assert (a->type == b->type);
    switch (a->type)
    {
    case JSON_STRING:
	assert (0 == strcmp (a->string, b->string));
	break;

    case JSON_NUMBER:
	assert ( (a->number - b->number) < 0.01 && (b->number - a->number) < 0.01 );
	break;
	
    default:
	break;
    }
}

static void _test_read_array (json_array * reference_array, const char * string, const char * remain)
{
    json_array read_array;
    json_tmp tmp = {0};
    range_const_char text;
    _bound_text (&text, string);
    assert (_identify_next(&text) == JSON_ARRAY);
    assert (_read_array(&read_array, &text, &tmp));

    assert (0 == strcmp (text.begin, remain));

    assert (range_count (read_array) == range_count (*reference_array));

    for (int i = 0; i < range_count (read_array); i++)
    {
	log_normal ("Testing array index %d", i);
	_compare_values (read_array.begin + i, reference_array->begin + i);
    }

    free (tmp.text.alloc.begin);
}

static void _test_read_array_numbers()
{
    const char * string = " [ 1, 2, 3, 4, 5 ] remain";
    json_value values[] = { { .type = JSON_NUMBER, .number = 1 },
			    { .type = JSON_NUMBER, .number = 2 },
			    { .type = JSON_NUMBER, .number = 3 },
			    { .type = JSON_NUMBER, .number = 4 },
			    { .type = JSON_NUMBER, .number = 5 } };
    
    json_array reference_array = { .begin = values, .end = values + sizeof(values) / sizeof (*values) };

    _test_read_array (&reference_array, string, " remain");
}

static void _test_read_array_strings()
{
    const char * string = " [ \"asdf\", \"bcle\", \"1234\", \"abcd\", \"jkl;\" ]  remain";
    json_value values[] = { { .type = JSON_STRING, .string = "asdf" },
			    { .type = JSON_STRING, .string = "bcle" },
			    { .type = JSON_STRING, .string = "1234" },
			    { .type = JSON_STRING, .string = "abcd" },
			    { .type = JSON_STRING, .string = "jkl;" } };
    
    json_array reference_array = { .begin = values, .end = values + sizeof(values) / sizeof (*values) };

    _test_read_array (&reference_array, string, "  remain");
}

static void _test_read_object(range_json_object_key_value * reference, const char * string, const char * remain)
{
    range_const_char text;
    _bound_text (&text, string);

    assert (_identify_next(&text) == JSON_OBJECT);

    json_tmp tmp = {0};
    json_object * object = _read_object(&text, &tmp);

    assert (object);

    json_value * real_value;
    json_object_key_value * reference_kv;

    for_range (reference_kv, *reference)
    {
	log_normal ("Testing reference value %s", reference_kv->key);
	real_value = json_lookup(object, reference_kv->key);
	assert (real_value);
	_compare_values (real_value, &reference_kv->value);
    }
    
    free (tmp.text.alloc.begin);
}

static void _test_read_object_numbers ()
{
    json_object_key_value kv_array[] = { { "asdf", { JSON_NUMBER, .number = 1 } },
					 { "bcle", { JSON_NUMBER, .number = 3 } },
					 { "test", { JSON_NUMBER, .number = 5 } } };

    range_json_object_key_value kv_range = { kv_array, kv_array + sizeof(kv_array) / sizeof(*kv_array) };

    _test_read_object (&kv_range, " { \"asdf\" : 1, \"bcle\" : 3, \"test\" : 5 } remain", " remain");
}

static void _test_read_object_numbers_strings ()
{
    json_object_key_value kv_array[] = { { "asdf", { JSON_STRING, .string = "something" } },
					 { "bcle", { JSON_NUMBER, .number = 3 } },
					 { "1234", { JSON_STRING, .string = " 2048 " } },
					 { "test", { JSON_NUMBER, .number = 5 } } };

    range_json_object_key_value kv_range = { kv_array, kv_array + sizeof(kv_array) / sizeof(*kv_array) };

    _test_read_object (&kv_range, " { \"asdf\" : \"something\", \"bcle\" : 3, \"1234\" : \" 2048 \", \"test\" : 5 } remain", " remain");
}

static void _test_skip_string(const char * string, const char * skip, const char * remain)
{
    range_const_char text;
    _bound_text (&text, string);
    assert (_skip_string(&text,skip));
    assert (0 == strcmp (text.begin, remain));
}

int main()
{
    _test_identify_next ();
    _test_string ("  \"this is a string\"  asdf", "this is a string", "  asdf");
    _test_string ("   \"this \\\"is\\\" a string with escaped quotes\"   1234  ", "this \"is\" a string with escaped quotes", "   1234  ");
    _test_number ("        1000   ", 1000, "   ");
    _test_number ("    5.23e+2  77", 523, "  77");
    _test_number ("  1.23   ", 1.23, "   ");
    _test_skip_whitespace (true, "   asdf", "asdf");
    _test_skip_whitespace (false, "", "");
    _test_skip_whitespace (true, "asdf", "asdf");

    _test_read_array_numbers ();
    _test_read_array_strings ();
    _test_read_object_numbers ();
    _test_read_object_numbers_strings ();
    
    _test_skip_string ("asdf bcle", "asdf", " bcle");
}
