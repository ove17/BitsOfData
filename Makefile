CC=gcc

CFLAGS=-Wall -Wextra \
        -Iinclude \
        -Iutils \
        -Itest/mocks \
        -Isrc

OBJDIR := obj

SRC := $(wildcard src/*.c) \
       $(wildcard test/mocks/*.c)

OBJ := $(patsubst %.c,$(OBJDIR)/%.o,$(SRC))

TARGET = libBDB.a

all: $(TARGET)

$(TARGET): $(OBJ)
	ar rcs $@ $^

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET)
