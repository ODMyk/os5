CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

DIST=dist
BINARIES = writer reader keyboard_hook

all: $(BINARIES)

writer: writer.cpp
	$(CXX) $(CXXFLAGS) -o $(DIST)/writer writer.cpp

reader: reader.cpp
	$(CXX) $(CXXFLAGS) -o $(DIST)/reader reader.cpp

keyboard_hook: keyboard_hook.cpp
	$(CXX) $(CXXFLAGS) -o $(DIST)/keyboard_hook keyboard_hook.cpp -ludev

clean:
	rm -f $(DIST)/* keyboard.log
