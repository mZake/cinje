CXX ?= g++

CXXFLAGS := -std=c++20 -O2 -Wall -Wextra

SOURCES := common.cpp main.cpp parser.cpp

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

.PHONY: all clean

all: patchbin$(EXE)
	@:

patchbin$(EXE): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $@

clean:
	$(RM) patchbin patchbin.exe
