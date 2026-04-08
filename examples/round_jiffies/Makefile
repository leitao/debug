CC	= gcc
CFLAGS	= -Wall -Wextra -O2

TARGET	= round_jiffies

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

.PHONY: all clean
