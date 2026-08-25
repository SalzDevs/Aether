# Compiler
CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# Target
TARGET = main

# Object files
OBJS = main.o sensor/sensor.o scheduler/scheduler.o History/history.o config/config.o ui/dashboard.o

# Raylib flags (installed via Homebrew)
RAYLIB_CFLAGS = $(shell pkg-config --cflags raylib)
RAYLIB_LIBS = $(shell pkg-config --libs raylib)

# libyaml flags (installed via Homebrew)
YAML_CFLAGS = $(shell pkg-config --cflags yaml-0.1)
YAML_LIBS = $(shell pkg-config --libs yaml-0.1)

# Module sources linked into every test binary
# (ui/ is excluded: only the main binary needs Raylib)
MODULE_SRCS = History/history.c sensor/sensor.c scheduler/scheduler.c config/config.c

# Auto-discover tests:
# Every file named tests/test_*.c becomes its own executable.
TEST_SRCS = $(wildcard tests/test_*.c)
TEST_BINS = $(patsubst tests/test_%.c,test_%,$(TEST_SRCS))


# -------------------------
# Main program
# -------------------------

# Final link step
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(RAYLIB_LIBS) $(YAML_LIBS) -lm


# -------------------------
# Compilation
# -------------------------

# Compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) $(YAML_CFLAGS) -c $< -o $@


# -------------------------
# Tests
# -------------------------

# Build and run every test binary
test: $(TEST_BINS)
	@if [ -z "$(TEST_SRCS)" ]; then \
	echo "No tests found - create tests/test_*.c files"; \
	exit 1; \
	fi
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


# Each test file compiles into its own binary
# tests/test_foo.c -> test_foo
test_%: tests/test_%.c $(MODULE_SRCS)
	$(CC) $(CFLAGS) $(YAML_CFLAGS) -I. $< $(MODULE_SRCS) -o $@ $(YAML_LIBS) -lm


# -------------------------
# Cleanup
# -------------------------

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_BINS)
