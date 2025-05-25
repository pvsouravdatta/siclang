# Detect operating system
ifeq ($(OS),Windows_NT)
    # Windows
    BINARY_EXT := .exe
    RM := del /F /Q
    BINARY := siclang$(BINARY_EXT)
else
    # Unix-like
    BINARY_EXT :=
    RM := rm -f
    BINARY := siclang$(BINARY_EXT)
endif

# Compiler settings
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2

# Source files
SRCS := main.cpp interpreter.cpp
OBJS := $(SRCS:.cpp=.o)

# Default target
all: $(BINARY)

# Link object files to create binary
$(BINARY): $(OBJS)
	$(CXX) $(OBJS) -o $(BINARY)

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	$(RM) $(OBJS) $(BINARY)

# Phony targets
.PHONY: all clean 