# Define the compiler and the options
CC = gcc
CFLAGS = -O0 -g -no-pie

# Define the target executable
TARGET = spectre_v2

# Default target
all: $(TARGET)

# Rule to build the target
$(TARGET): spectre_v2.c
	$(CC) $(CFLAGS) -o $(TARGET) spectre_v2.c

# Clean target to remove the executable
clean:
	rm -f $(TARGET)

.PHONY: all clean
