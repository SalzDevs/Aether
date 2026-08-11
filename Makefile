# Variables
CC = gcc
CFLAGS = -Wall -Wextra -std=c99
OBJS = main.o sensor/sensor.o scheduler/scheduler.o Qeue/qeue.o 
TARGET = main

# Raylib flags (installed via Homebrew); prefix any compile line with these
# to use raylib in your code: $(CC) $(CFLAGS) $(RAYLIB_CFLAGS) ... $(RAYLIB_LIBS)
RAYLIB_CFLAGS = $(shell pkg-config --cflags raylib)
RAYLIB_LIBS = $(shell pkg-config --libs raylib)

# Module sources linked into every test binary
MODULE_SRCS = Qeue/qeue.c sensor/sensor.c scheduler/scheduler.c

# Auto-discover tests: every file named tests/test_*.c is its own executable.
# Each one must define its own main() and exit 0 on success, non-zero on failure.
TEST_SRCS = $(wildcard tests/test_*.c)
TEST_BINS = $(patsubst tests/test_%.c,test_%,$(TEST_SRCS))

# Final Link Step (combines .o files into executable)
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

# Compile Steps (.c to .o)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build and run every test binary
test: $(TEST_BINS)
	@if [ -z "$(TEST_SRCS)" ]; then echo "No tests found - create tests/test_*.c files"; exit 1; fi
	@echo "--- Running $(words $(TEST_BINS)) test file(s) ---"
	@fail=0; \
	for t in $(TEST_BINS); do \
	  ./$$t || fail=1; \
	done; \
	if [ $$fail -eq 0 ]; then \
	  echo "ALL TESTS PASSED"; \
	else \
	  echo "SOME TESTS FAILED"; \
	  exit 1; \
	fi

# Each test file compiles into its own binary (tests/test_foo.c -> test_foo)
test_%: tests/test_%.c $(MODULE_SRCS)
	$(CC) $(CFLAGS) -I. $< $(MODULE_SRCS) -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET) $(TEST_BINS)