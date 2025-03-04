# Compiler to use
CC = gcc
# Compiler flags, all/extra warnings
CFLAGS = -Wall -Wextra
# Name of the executable
TARGET = JBash
# Source file
SRC = JBash.c
# Object files (derived from source files)
OBJ = $(SRC:.c=.o)
# Header files
HEADERS = JBash.h

# Main target: link object files to create executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Pattern rule: compile source files to object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Phony target to clean up build artifacts
.PHONY: clean
clean:
	$(RM) -f $(TARGET) $(OBJ)