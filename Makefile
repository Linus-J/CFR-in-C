C_SOURCES = $(wildcard pcg/*.c *.c)
HEADERS = $(wildcard pcg/*.h *.h)
OBJ = ${C_SOURCES:.c=.o}
CFLAGS = -g -O0

MAIN = cfr
CC = /usr/bin/gcc
LINKER = /usr/bin/ld

main: ${OBJ}
	${CC} ${CFLAGS} $^ -o $@ -lm

# Generic rules
%.o: %.c ${HEADERS}
	${CC} ${CFLAGS} -c $< -o $@ -lm

clean:
	rm pcg/*.o *.o ${MAIN}
