all: aurora

test:
	g++ test.cpp -o test -std=c++20
aurora:
	g++ aurora.cpp -o aurora -std=c++20