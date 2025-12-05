CC = gcc
CFLAGS = -Wall -Wextra -O3 -pthread
TARGETS = test_hashtable benchmark example_custom_types test_defaults

all: $(TARGETS)

test_hashtable: tests/test_hashtable.c hashtable.h
	$(CC) $(CFLAGS) -o $@ $<

benchmark: benchmark.c hashtable.h
	$(CC) $(CFLAGS) -o $@ $<

example_custom_types: examples/example_custom_types.c hashtable.h
	$(CC) $(CFLAGS) -o $@ $<

test_defaults: tests/test_defaults.c hashtable.h
	$(CC) $(CFLAGS) -o $@ $<

test: test_hashtable test_defaults
	./test_hashtable
	./test_defaults

bench: benchmark
	./benchmark

clean:
	rm -f $(TARGETS)

.PHONY: all test bench clean
