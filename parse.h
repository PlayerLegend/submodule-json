#ifndef FLAT_INCLUDES
#include <stdbool.h>
#define FLAT_INCLUDES
#include "../range/def.h"
#include "../keyargs/keyargs.h"
#include "def.h"
#endif

json_value * json_parse (const range_const_char * input);
void json_free (json_value * value);
