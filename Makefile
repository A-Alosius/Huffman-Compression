CC := gcc

BINDIR := bin
OBJDIR := $(BINDIR)/object
DEPDIR := $(BINDIR)/dependency

TARGET1 := $(BINDIR)/compress
TARGET2 := $(BINDIR)/extract
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

OBJ1 := $(addprefix $(OBJDIR)/,$(SRC1:.c=.o))
OBJ2 := $(addprefix $(OBJDIR)/,$(SRC2:.c=.o))

DEP1 := $(patsubst $(OBJDIR)/%.o,$(DEPDIR)/%.d,$(OBJ1))
DEP2 := $(patsubst $(OBJDIR)/%.o,$(DEPDIR)/%.d,$(OBJ2))

.PHONY: all clean run-compress run-extract

all: $(TARGETS)

$(TARGET1): $(OBJ1) ; @mkdir -p $(dir $@) && $(CC) $(OBJ1) $(LDFLAGS) -o $@

$(TARGET2): $(OBJ2) ; @mkdir -p $(dir $@) && $(CC) $(OBJ2) $(LDFLAGS) -o $@

$(OBJDIR)/%.o: %.c ; @mkdir -p $(dir $@) $(dir $(patsubst $(OBJDIR)/%.o,$(DEPDIR)/%.d,$@)) && $(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF $(patsubst $(OBJDIR)/%.o,$(DEPDIR)/%.d,$@) -MT $@ -c $< -o $@

run-compress: $(TARGET1)
	./$(TARGET1)

run-extract: $(TARGET2)
	./$(TARGET2)

clean: ; rm -f compress extract && rm -rf $(BINDIR) object dependency

-include $(sort $(DEP1) $(DEP2))