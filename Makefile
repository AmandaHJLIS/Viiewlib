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

WII_CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc
WII_AR = $(DEVKITPPC)/bin/powerpc-eabi-ar

WII_CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Iinclude

WII_TARGET = libviiewlib_wii.a

WII_OBJECTS = \
	source/marc_record_wii.o \
	source/marc_field_wii.o \
	source/marc_subfield_wii.o \
	source/marc_iso2709_wii.o

TEST_TARGET = tests/test_marc

.PHONY: all wii clean test

all: $(TARGET)

wii: $(WII_TARGET)

$(TARGET): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $^

source/%.o: source/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(WII_TARGET): $(WII_OBJECTS)
	$(WII_AR) $(ARFLAGS) $@ $^

source/%_wii.o: source/%.c
	$(WII_CC) $(WII_CFLAGS) -c $< -o $@

$(TEST_TARGET): tests/test_marc.c $(TARGET)
	$(CC) $(CFLAGS) $< -L. -lviiewlib -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(OBJECTS) $(WII_OBJECTS) $(TARGET) $(WII_TARGET) $(TEST_TARGET)
	rm -f test.mrc