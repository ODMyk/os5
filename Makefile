CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

DIST=dist
BINARIES = writer reader keyboard_hook

READER_LIBS = -lelf

KEYBOARD_LIBS = -ludev

all: $(BINARIES)

writer: writer.cpp
	@mkdir -p $(DIST)
	$(CXX) $(CXXFLAGS) -o $(DIST)/writer writer.cpp
	@echo "✅ Built: $@"

reader: reader.cpp
	@mkdir -p $(DIST)
	$(CXX) $(CXXFLAGS) -o $(DIST)/reader reader.cpp $(READER_LIBS)
	@echo "✅ Built: $@"

keyboard_hook: keyboard_hook.cpp
	@mkdir -p $(DIST)
	$(CXX) $(CXXFLAGS) -o $(DIST)/keyboard_hook keyboard_hook.cpp $(KEYBOARD_LIBS)
	@echo "✅ Built: $@"

clean:
	@rm -f $(DIST)/* keyboard.log
	@echo "🧹 Cleaned $(DIST) and keyboard.log"

.PHONY: all writer reader keyboard_hook clean