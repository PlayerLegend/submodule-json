C_PROGRAMS += test/json

json-tests: test/json

depend: json-depend
json-depend:
	cdeps src/json > src/json/depends.makefile

run-tests: run-json-tests
run-json-tests:
	sh run-tests.sh test/json

test/json: src/json/test/json.test.o
test/json: src/log/log.o
test/json: src/table/string.o
test/json: src/range/strdup_to_string.o
test/json: src/range/streq.o
test/json: src/range/strdup.o
test/json: src/range/string_init.o
test/json: src/window/alloc.o

tests: json-tests
