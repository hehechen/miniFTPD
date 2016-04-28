#目标
TARGET=chenftpd
SOURCES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp,%.o,$(SOURCES))
CXX=g++
CFLAGS=-Wall -g -std=c++11
LIBS=-lcap -lcrypt -lpthread

$(TARGET):$(OBJS)
	$(CXX) $(CFLAGS) $^ -o $@ $(LIBS)
.cpp.o:
	$(CXX) $(CFLAGS) -c $< -o $@

.PHONY:clean
clean:
	rm -f *.o 