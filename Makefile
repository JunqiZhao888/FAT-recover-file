CFLAGS = -Wall -g -std=c99 -D_POSIX_C_SOURCE=200809L

LDFLAGS = -lcrypto

TARGET = nyufile

all: $(TARGET)

$(TARGET): nyufile.o root_func.o restore.o
	$(CC) $(CFLAGS) -o $(TARGET) nyufile.o root_func.o restore.o $(LDFLAGS)

nyufile.o: nyufile.c ds.h
	$(CC) $(CFLAGS) -c nyufile.c

root_func.o: root_func.c ds.h
	$(CC) $(CFLAGS) -c root_func.c

restore.o: restore.c ds.h
	$(CC) $(CFLAGS) -c restore.c

clean:
	rm -f $(TARGET) nyufile.o root_func.o restore.o

.PHONY: all clean
