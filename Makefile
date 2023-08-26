all: aurora

test:
	g++ test.cpp -o test Fathom-1.0/src/tbprobe.c -march=native
aurora:
	g++ aurora.cpp -o aurora -std=c++20