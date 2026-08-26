CXX      = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2
INCLUDE  = -Isrc/helpers -Isrc/sfetch -Iexternal/tomlplusplus/include
BINS     = sfetch scat sls sk se scopy space srm smv
OBJS     = src/s-kill/main.o src/simple/ls.o

all: $(BINS) $(OBJS)

sfetch: src/sfetch/main.o
	$(CXX) $(CXXFLAGS) -o $@ $^

scat: src/cat/scat.o
	$(CXX) $(CXXFLAGS) -o $@ $^

sls: src/simple/ls.o
	$(CXX) $(CXXFLAGS) -o $@ $^

sk: src/s-kill/main.o
	$(CXX) $(CXXFLAGS) -o $@ $^

scopy: src/another/copy.o
	$(CXX) $(CXXFLAGS) -o $@ $^

space: src/another/space.o
	$(CXX) $(CXXFLAGS) -o $@ $^

srm: src/another/deleter.o
	$(CXX) $(CXXFLAGS) -o $@ $^

smv: src/another/renamernsizer.o
	$(CXX) $(CXXFLAGS) -o $@ $^

se: src/shell/main.o
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c -o $@ $<

clean:
	rm -f $(BINS) src/*/*.o

.PHONY: all clean
