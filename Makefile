test:
	clang++ aurora.cpp external/Fathom-1.0/src/tbprobe.cpp -o aurora.exe -march=x86-64-v3 -O3 -Wno-deprecated-declarations