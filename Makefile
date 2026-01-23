CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -O2
INCLUDES = -I.

.PHONY: all test clean

all: test_runner

test_runner: test.cpp include/SessionToken.h include/chacha20.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ test.cpp

test: test_runner
	./test_runner

clean:
	rm -f test_runner
