CC=gcc

SRC=chat.c
TARGET=chat

CFLAGS += -std=gnu99 -Wall -pthread

default: $(TARGET)

dbase.o: dbase.c
	$(CC) -c dbase.c

sockstruct.o: sockstruct.c
	$(CC) -C sockstruct.c
	

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) dbase.o sockstruct.o

clean:
	rm -rf $(TARGET) $(TARGET)

