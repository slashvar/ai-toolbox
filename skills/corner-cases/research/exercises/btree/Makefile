CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -g -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lgtest -lgtest_main

test_btree: test_btree.cpp btree.cpp btree.hpp
	$(CXX) $(CXXFLAGS) -o test_btree test_btree.cpp btree.cpp $(LDFLAGS)

test: test_btree
	./test_btree

clean:
	rm -f test_btree

.PHONY: test clean
