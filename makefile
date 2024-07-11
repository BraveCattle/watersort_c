CC=gcc
CFLAGS= -lGL -lm -lpthread -ldl -lrt -lX11 -w -g
UTIL=utils.c

main: main.c ${UTIL}
	$(CC) -o main main.c ${UTIL} -I./raylib/include -L./raylib/lib -lraylib $(CFLAGS)

clean:
	rm utils.o main.o main