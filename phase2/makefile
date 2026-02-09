# Compiler (use g++ for local development, aarch64-linux-gnu-g++ for cross-compilation)
CXX = g++
STRIP = strip

# Compiler and flags
CXXFLAGS = -Wall -Wextra -O0 -std=c++17 -I/usr/include
LDFLAGS = -lgpiod -lpthread

# Uncomment these for cross-compilation:
# CXX = aarch64-linux-gnu-g++
# STRIP = aarch64-linux-gnu-strip
# CXXFLAGS += -march=armv8-a -mtune=cortex-a53

# Target executable name
TARGET = action_cam

# Build directory
BUILD_DIR = build

# Source files
SRCS = main.cpp ssd1306.cpp
# Object files in build directory
OBJS = $(SRCS:%.cpp=$(BUILD_DIR)/%.o)
# Header files
DEPS = ssd1306.hpp
# Target executable in build directory
TARGET_PATH = $(BUILD_DIR)/$(TARGET)

# Default rule: build the target
all: $(TARGET_PATH)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link object files to create the executable
$(TARGET_PATH): $(OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files into object files in build directory
$(BUILD_DIR)/%.o: %.cpp $(DEPS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Clean up build directory
clean:
	rm -rf $(BUILD_DIR)

# Declare targets as phony
.PHONY: all clean
