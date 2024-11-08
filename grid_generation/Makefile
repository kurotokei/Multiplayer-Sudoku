MAKEFLAGS += -r
MAKEFLAGS += --warn-undefined-variables

SHELL := /bin/bash

#################################################

.PHONY: help all run clean install

.DEFAULT_GOAL := help

includes := -I./include
std_opts := -Wall -Wextra -g3 -Wfatal-errors -std=c++23
opt_opts := -O3 -march=native -flto -fno-fat-lto-objects
libs     := -ljsoncpp
defs     := -DNDEBUG

opts      = -fopenmp $(defs) $(std_opts) $(opt_opts) $(includes) $(libs)


$(info $(shell mkdir -p build))

sources := $(filter-out src/main.cpp,$(wildcard src/*.cpp))


help:
	@printf 'Makefile help:\n'
	@printf '\tall: builds ./sudoku.so and ./prog\n'
	@printf '\trun: generate ./build/sudoku_puzzles.data\n'


all: sudoku.so prog 
	@# nothing to do here


sudoku.so: $(sources) $(wildcard include/*.hpp)
	g++ -o $@ $(sources) $(opts) -fPIC -shared

prog: src/main.cpp sudoku.so
	g++ -o prog  src/main.cpp $(opts) -L. -l:sudoku.so

sudoku.so prog: .EXTRA_PREREQS += Makefile


run: all
	export LD_LIBRARY_PATH+=:. ; ./prog

# '-' is to ignore errors
clean:
	-rm -r build
	-rm prog sudoku.so

install: run
	cp ./sudoku.so ./build/sudoku_puzzles.data ../website/mysite/sudoku_utils
