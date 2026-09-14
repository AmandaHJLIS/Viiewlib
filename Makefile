CC = gcc

CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Iinclude

AR = ar
ARFLAGS = rcs

TARGET = libviiewlib.a

SOURCES = \
	source/marc_record.c \
	source/marc_field.c \
	source/marc_subfield.c \
	source/marc_iso2709.c

OBJECTS = $(SOURCES:.c=.o)

TEST_TARGET = tests/test_marc

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $^

source/%.o: source/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_TARGET): tests/test_marc.c $(TARGET)
	$(CC) $(CFLAGS) $< -L. -lviiewlib -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET) $(TEST_TARGET)
	rm -f test.mrc