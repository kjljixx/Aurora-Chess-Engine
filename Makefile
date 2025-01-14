EXE := aurora

ifeq ($(OS),Windows_NT)
    override EXE := $(EXE).exe
endif

GIT_HASH := $(shell git rev-parse --short HEAD)
GIT_DIFF := $(shell git diff --shortstat)
ifneq ($(GIT_DIFF),)
		GIT_TREE_FULL_HASH := $(wordlist 1,1, $(shell git add -u && git write-tree && git reset))
		override GIT_HASH := $(GIT_HASH)-dirty-$(GIT_TREE_FULL_HASH)
endif

build:
	clang++ aurora.cpp external/Fathom-1.0/src/tbprobe.cpp -o $(EXE) -march=x86-64-v3 -O3 -Wno-deprecated-declarations -DGIT_HASH=\"$(GIT_HASH)\"