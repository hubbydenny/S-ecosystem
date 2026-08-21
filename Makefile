CXX      = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2
INCLUDE  = -Isrc/helpers -Isrc/sfetch -Iexternal/tomlplusplus/include

# Готовые программы (только с main + кодом)
BINS     = sfetch scat sls
# Объектные файлы остальных исходников (заглушки тоже компилируются в .o)
OBJS     = src/s-kill/main.o src/simple/ls.o

all: $(BINS) $(OBJS)

sfetch: src/sfetch/main.o
	$(CXX) $(CXXFLAGS) -o $@ $^

scat: src/cat/scat.o
	$(CXX) $(CXXFLAGS) -o $@ $^

sls: src/simple/ls.o
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c -o $@ $<

clean:
	rm -f $(BINS) src/*/*.o

.PHONY: all clean
