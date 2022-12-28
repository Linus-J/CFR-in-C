C_SOURCES = $(wildcard pcg/*.c)
HEADERS = $(wildcard pcg/*.h)
OBJ = ${C_SOURCES:.c=.o}
CFLAGS = -g -O2 #-fno-stack-protector
EXECUTABLES = cfr mccfr dcfr cfrplus

MAIN = all
CC = /usr/bin/gcc
LINKER = /usr/bin/ld

all: $(EXECUTABLES)

cfr: cfr.o $(OBJ)
	${CC} ${CFLAGS} $^ -o $@ -lm

mccfr: mccfr.o $(OBJ)
	${CC} ${CFLAGS} $^ -o $@ -lm

dcfr: dcfr.o $(OBJ)
	${CC} ${CFLAGS} $^ -o $@ -lm

cfrplus: cfrplus.o $(OBJ)
	${CC} ${CFLAGS} $^ -o $@ -lm

# Generic rules
%.o: %.c ${HEADERS}
	${CC} ${CFLAGS} -c $< -o $@ -lm

clean:
	rm pcg/*.o *.o $(EXECUTABLES)
