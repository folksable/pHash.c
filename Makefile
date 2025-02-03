# Makefile for pHash (Apple Silicon + x86 + Generic)

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lm
TARGET = phash_demo
SRC = pHash.c main.c
OBJ = $(SRC:.c=.o)

# Architecture detection
UNAME_M := $(shell uname -m)

# Architecture-specific flags
ifeq ($(UNAME_M), arm64)
    # Apple Silicon optimizations
    CFLAGS += -O3 -mcpu=apple-m1 -mtune=apple-m1
else ifeq ($(UNAME_M), x86_64)
    # x86 optimizations
    CFLAGS += -O3 -mavx2 -msse4.2
else
    # Generic optimizations
    CFLAGS += -O3
endif

# Build rules
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean