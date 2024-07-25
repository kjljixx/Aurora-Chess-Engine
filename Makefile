EXE := aurora

ifeq ($(OS),Windows_NT)
    override EXE := $(EXE).exe
endif

test:
	clang++ aurora.cpp external/Fathom-1.0/src/tbprobe.cpp -o ${EXE} -march=x86-64-v3 -O3 -Wno-deprecated-declarations