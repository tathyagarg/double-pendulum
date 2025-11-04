CC := gcc
CFLAGS := -std=c17 -Wall -g

INCLUDE := -Iinclude
LIBS := -Llib -ldl -lm

SOURCE := src/main.c src/glad.c
LIBGLFW := lib/libglfw.3.4.dylib

BIN_DIR := bin
BIN := $(BIN_DIR)/main

HEADERS := $(patsubst shaders/%.vert, include/shaders/%.vert.h, $(wildcard shaders/*.vert)) \
					 $(patsubst shaders/%.frag, include/shaders/%.frag.h, $(wildcard shaders/*.frag))

.PHONY: build run shaders

shaders: $(HEADERS)

include/shaders/%.vert.h: shaders/%.vert
	@mkdir -p include/shaders
	xxd -i $< > $@

include/shaders/%.frag.h: shaders/%.frag
	@mkdir -p include/shaders
	xxd -i $< > $@

build: shaders
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) $(SOURCE) $(LIBGLFW) -o $(BIN)

run: build
	./$(BIN)
