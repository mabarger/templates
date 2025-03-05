# Define the compiler and the options
CC = gcc
CFLAGS = -O0 -g -no-pie

# Define the target executable
TARGET = spectre_v1

# Default target
all: $(TARGET)

# Rule to build the target
$(TARGET): spectre_v1.c
	$(CC) $(CFLAGS) -o $(TARGET) spectre_v1.c

# Clean target to remove the executable
clean:
	rm -f $(TARGET)

.PHONY: all clean
