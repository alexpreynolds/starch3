CC=g++
PRODUCT=starch3
FLAGS=-Wall -Weverything -Wno-exit-time-destructors
SRC=./src
BUILD=./build

all: prep
	${CC} ${FLAGS} -c ${SRC}/starch3.cpp -o ${BUILD}/starch3.o
	${CC} ${FLAGS} ${BUILD}/starch3.o -o ${BUILD}/${PRODUCT}

prep:
	if [ ! -d "${BUILD}" ]; then mkdir "${BUILD}"; fi

clean:
	rm -rf *~
	rm -rf ${SRC}/*~
	rm -rf ${BUILD}