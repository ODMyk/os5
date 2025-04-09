CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

BINARIES = writer reader keyboard_hook

all: $(BINARIES)

writer: writer.cpp
	$(CXX) $(CXXFLAGS) -o writer writer.cpp

reader: reader.cpp
	$(CXX) $(CXXFLAGS) -o reader reader.cpp

keyboard_hook: keyboard_hook.cpp
	$(CXX) $(CXXFLAGS) -o keyboard_hook keyboard_hook.cpp -ludev

clean:
	rm -f $(BINARIES) keyboard.log
