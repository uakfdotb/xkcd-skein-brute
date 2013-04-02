CC=gcc
CFLAGS=-Wall -O3 -I. -std=gnu99 -Wno-strict-aliasing -Wno-unused-result -Wno-unused-parameter -lpthread -fno-strict-aliasing
OBJ = SHA3api_ref.o skein_block.o skein.o skein_debug.o main.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

main: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

