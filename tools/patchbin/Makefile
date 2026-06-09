CXX ?= g++

CXXFLAGS := -std=c++20 -O2 -Wall -Wextra

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

.PHONY: all clean

all: patchbin$(EXE)
	@:

patchbin$(EXE): main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o $@

clean:
	$(RM) patchbin patchbin.exe
