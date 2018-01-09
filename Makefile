CC=gcc

SRC=chat.c
TARGET=chat

CFLAGS += -Wall -pthread

default: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -rf $(TARGET) $(TARGET)

