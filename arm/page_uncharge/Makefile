CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread
TARGET = page_counter_test
SOURCE = page_counter_test.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -g -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/

test: $(TARGET)
	sudo ./$(TARGET)

help:
	@echo "Available targets:"
	@echo "  all     - Build the test program"
	@echo "  clean   - Remove built files"
	@echo "  test    - Run the test (requires root)"
	@echo "  install - Install to /usr/local/bin"
	@echo "  help    - Show this help"

.PHONY: all clean install test help
