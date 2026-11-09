CC := gcc

TARGET1 := compress
TARGET2 := extract
TARGETS := $(TARGET1) $(TARGET2)

CPPFLAGS := -Iinclude

CFLAGS := -std=c17 \
          -Wall \
          -Wextra \
          -Wpedantic \
          -Wshadow \
          -Wconversion \
          -Wsign-conversion \
          -g3 \
          -O0 \
          -fsanitize=address \
          -fno-omit-frame-pointer

LDFLAGS := -fsanitize=address

COMMON_SRC := $(shell find src -type f -name '*.c')

SRC1 := compress.c $(COMMON_SRC)
SRC2 := extract.c $(COMMON_SRC)

OBJ1 := $(SRC1:.c=.o)
OBJ2 := $(SRC2:.c=.o)

DEP1 := $(OBJ1:.o=.d)
DEP2 := $(OBJ2:.o=.d)

.PHONY: all clean run-compress run-extract

all: $(TARGETS)

$(TARGET1): $(OBJ1)
	$(CC) $(OBJ1) $(LDFLAGS) -o $@

$(TARGET2): $(OBJ2)
	$(CC) $(OBJ2) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

run-compress: $(TARGET1)
	./$(TARGET1)

run-extract: $(TARGET2)
	./$(TARGET2)

clean:
	rm -f $(TARGETS)
	rm -f $(sort $(OBJ1) $(OBJ2))
	rm -f $(sort $(DEP1) $(DEP2))

-include $(sort $(DEP1) $(DEP2))