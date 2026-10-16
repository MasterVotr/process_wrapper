CXX      := g++
CXXFLAGS := -std=c++20 -I. -Wall -pedantic -Wextra -Werror
LDLIBS   := -pthread -lCatch2Main -lCatch2

UTIL_EXES := \
	tests/util_execs/exit_42 \
	tests/util_execs/sleep_forever \
	tests/util_execs/sigint_handler

.PHONY: compile test clean

compile: process_test

test: process_test $(UTIL_EXES)
	./process_test

clean:
	rm -f process_test $(UTIL_EXES)

format:
	clang-format -i process.hpp tests/process.t.cpp

tidy:
	clang-tidy process.hpp tests/process.t.cpp -- -std=c++20 -I.

process_test: tests/process.t.cpp $(UTIL_EXES)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDLIBS)

tests/util_execs/%: tests/util_execs/%.cpp
	$(CXX) -std=c++20 $< -o $@
