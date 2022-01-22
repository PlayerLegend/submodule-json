C_PROGRAMS += test/json
RUN_TESTS += test/json

json-tests: test/json

test/json: src/json/test/json.test.o
test/json: src/log/log.o
test/json: src/table/table.o
test/json: src/range/range_strdup_to_string.o
test/json: src/window/alloc.o

tests: json-tests
