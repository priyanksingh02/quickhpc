CC=gcc
PROGRAM=quickhpc
PAPI_DIR=../papi
PAPI_INSTALL=${PAPI_DIR}/src/install
PAPI_SRC=${PAPI_DIR}/src
OBJS=config.o
INC=-I${PAPI_INSTALL}/include 
LIB=-L${PAPI_INSTALL}/libs -lpapi

all: config ${PROGRAM}.c
	${CC} ${DEBUG_FLAG} ${INC} -g -Wall ${PROGRAM}.c \
		${OBJS} -o ${PROGRAM} ${LIB}

config: config.c
	${CC} ${INC} -g -c config.c

debug: DEBUG_FLAG += -D_DEBUG
debug: all

clean:
	-rm -f ${PROGRAM} ${OBJS}
