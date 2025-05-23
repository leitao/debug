# Define the compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Define the source files
SOURCES = create_threads_and_malloc.c

# Define the object files
OBJECTS = $(SOURCES:.c=.o)

# Define the executable name
EXECUTABLE = create_threads_and_malloc

# Default target: build the executable
all: $(EXECUTABLE)

# Rule to build the executable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Rule to build object files from source files
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# Clean target: remove object files and executable
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

# Run target: run the executable
run: $(EXECUTABLE)
	sudo cgexec -g memory:my_memcg ./$(EXECUTABLE)
