C_PROGRAMS += test/json

json-tests: test/json

depend: json-depend
json-depend:
	sh makedepend.sh src/json/json.makefile

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
# DO NOT DELETE

src/json/json.o: src/json/traverse.h src/json/def.h src/table/string.h
src/json/json.o: src/range/def.h src/keyargs/keyargs.h src/json/parse.h
src/json/json.o: src/window/def.h src/window/alloc.h src/log/log.h
src/json/json.o: src/range/alloc.h src/range/string.h
src/json/test/json.test.o: src/json/json.c src/json/traverse.h src/json/def.h
src/json/test/json.test.o: src/table/string.h src/range/def.h
src/json/test/json.test.o: src/keyargs/keyargs.h src/json/parse.h
src/json/test/json.test.o: src/window/def.h src/window/alloc.h src/log/log.h
src/json/test/json.test.o: src/range/alloc.h src/range/string.h
