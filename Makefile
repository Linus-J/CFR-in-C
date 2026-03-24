C_SOURCES = $(wildcard pcg/*.c)
HEADERS = $(wildcard pcg/*.h)
OBJ = ${C_SOURCES:.c=.o}
CFLAGS = -O2
EXECUTABLES = cfr mccfr dcfr dcfrLeduc cfrplus test_cfr

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
	
dcfrLeduc: dcfrLeduc.o $(OBJ)
	${CC} ${CFLAGS} $^ -o $@ -lm

cfrplus: cfrplus.o $(OBJ)
	${CC} ${CFLAGS} $^ -o $@ -lm

test_cfr: test_cfr.o $(OBJ)
	${CC} ${CFLAGS} $^ -o $@ -lm

test: test_cfr
	./test_cfr

# Generic rules
%.o: %.c ${HEADERS}
	${CC} ${CFLAGS} -c $< -o $@ -lm

clean:
	rm -f pcg/*.o *.o $(EXECUTABLES)
