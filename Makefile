EXE := aurora
TEST_EXE := aurora_tests
TEST_SOURCES := tests/main.cpp tests/test_bitboards.cpp tests/test_fen.cpp tests/test_perft.cpp \
	tests/test_castling.cpp tests/test_enpassant.cpp tests/test_makemove.cpp tests/test_zobrist.cpp \
	tests/test_game_status.cpp tests/test_moves.cpp tests/test_nnue.cpp tests/test_search.cpp
BUILD_OPTIONS := -march=x86-64-v3 -O3 -std=c++17 -Wno-deprecated-declarations
TEST_BUILD_OPTIONS := -I. -Itests -march=x86-64-v3 -O2 -std=c++17 -Wno-deprecated-declarations

ifneq ($(exe),)
    EXE := $(exe)
endif

ifeq ($(OS),Windows_NT)
    override EXE := $(EXE).exe
    override TEST_EXE := $(TEST_EXE).exe
endif

GIT_BRANCH := $(shell git rev-parse --abbrev-ref HEAD)
GIT_HASH := $(GIT_BRANCH)-$(shell git rev-parse --short HEAD)
GIT_DIFF := $(shell git diff --shortstat)

ifneq ($(GIT_DIFF),)
    GIT_TREE_HASH := $(shell git rev-parse --short=7 $(word 1, $(shell git add -u && git write-tree && git reset)))
    override GIT_HASH := $(GIT_HASH)-$(GIT_TREE_HASH)-dirty
endif

build:
	clang++ aurora.cpp nnue_data.cpp external/Fathom-1.0/src/tbprobe.cpp -o $(EXE) -DGIT_HASH=\"$(GIT_HASH)\" $(BUILD_OPTIONS)
bench: build
	./$(EXE) bench
hash:
	@echo $(GIT_HASH)
dev:
	clang++ aurora.cpp nnue_data.cpp external/Fathom-1.0/src/tbprobe.cpp -o $(EXE) -DGIT_HASH=\"$(GIT_HASH)\" $(BUILD_OPTIONS) -DDEV

test-build:
	clang++ $(TEST_SOURCES) nnue_data.cpp external/Fathom-1.0/src/tbprobe.cpp -o $(TEST_EXE) $(TEST_BUILD_OPTIONS)

test: test-build
	./$(TEST_EXE)