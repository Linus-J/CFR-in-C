C_SOURCES = $(wildcard pcg/*.c)
HEADERS = $(wildcard pcg/*.h)
OBJ = ${C_SOURCES:.c=.o}
CFLAGS = -O2
EXECUTABLES = cfr mccfr dcfr cfrplus dcfrLeduc

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

dcfrLeduc: dcfrLeduc.o $(OBJ)
	${CC} ${CFLAGS} $^ -o $@ -lm

# Generic rules
%.o: %.c ${HEADERS}
	${CC} ${CFLAGS} -c $< -o $@ -lm

clean:
	rm pcg/*.o *.o $(EXECUTABLES)
