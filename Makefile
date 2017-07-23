CC=gcc

SRC=chat.c
TARGET=chat

CFLAGS += -std=gnu99 -Wall -pthread

default: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -rf $(TARGET) $(TARGET)

