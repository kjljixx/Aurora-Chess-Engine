SRC_FILES := aurora.cpp ${wildcard *.h} external/incbin.h $(wildcard external/Fathom-1.0/src/*.*)
EXE := aurora
dg := -DNO_DATAGEN
dev :=

ifeq (${dev},1)
	override dev := -DDEV
	override EXE := ${EXE}dev
endif

ifeq (${dg},1)
	override dg :=
	override EXE := ${EXE}datagen
endif

ifeq (${OS},Windows_NT)
  override EXE := ${EXE}.exe
endif

${EXE}: ${SRC_FILES}
	clang++ aurora.cpp external/Fathom-1.0/src/tbprobe.cpp -o ${EXE} -march=x86-64-v3 -O3 -Wno-deprecated-declarations ${dev} ${dg}

run: ${EXE}
	./${EXE}
