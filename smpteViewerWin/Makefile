CXX=clang++-3.5
CFLAGS=-std=c++11 -pthread -I/usr/local/include/SDL2/ -I/usr/include/SDL2/ -c -Wall -O3
LDFLAGS=-L/usr/local/lib -lSDL2 -pthread 
SOURCES=launcher.cpp imagePacker.cpp packetGetter.cpp pcapGenerator.cpp imageGenerator.cpp windowManager.cpp smpteForwarder.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=hello

all: $(SOURCES) $(EXECUTABLE)
    
$(EXECUTABLE): $(OBJECTS) 
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CXX) $(CFLAGS) $< -o $@

