#!/usr/bin/make

# -g for debug -Os for optimize
CPPFLAGS = -std=c++11 -Os

sources = $(wildcard *.cpp)
objects = $(addsuffix .o,$(basename $(sources)))
targets = $(notdir $(PWD))

all: $(targets)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<
	@$(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< > $*.d

$(targets): $(objects)
	g++ $^ -lusb-1.0 -o $@

%.cpp.lst: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -O0 -g -fverbose-asm -Wa,-adhln $< > $@

clean:
	rm -f rpc.cpp.i *.o *.d *.lst $(targets)

# include dependencies
-include $(wildcard *.d)
