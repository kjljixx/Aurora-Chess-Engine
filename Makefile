EXE := aurora

ifeq ($(OS),Windows_NT)
    override EXE := $(EXE).exe
endif

build:
	clang++ aurora.cpp -o ${EXE} -march=x86-64-v3 -O3 -Wno-deprecated-declarations