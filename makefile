CC=g++
PRODUCT=starch3
FLAGS=-Wall -Weverything -Wno-exit-time-destructors
CWD=$(shell pwd)
SRC=${CWD}/src
BUILD=${CWD}/build
BZIP2ARC=${SRC}/bzip2-1.0.6.tar.gz
BZIP2DIR=${SRC}/bzip2-1.0.6
BZIP2SYMDIR=${SRC}/bzip2
BZIP2LIBDIR=${BZIP2SYMDIR}
FLAGS2=-O3 -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
INC=${SRC}

all: prep bzip2
	${CC} ${FLAGS} ${FLAGS2} -I${INC} -c "${SRC}/starch3.cpp" -o "${BUILD}/starch3.o"
	${CC} ${FLAGS} ${FLAGS2} -I${INC} -I${BZIP2SYMDIR} -L"${BZIP2LIBDIR}" -lbz2 "${BUILD}/starch3.o" -o "${BUILD}/${PRODUCT}"

prep:
	if [ ! -d "${BUILD}" ]; then mkdir "${BUILD}"; fi

bzip2:
	if [ ! -d "${BZIP2DIR}" ]; then mkdir "${BZIP2DIR}"; fi
	tar zxvf "${BZIP2ARC}" -C "${SRC}"
	ln -s ${BZIP2DIR} ${BZIP2SYMDIR}
	${MAKE} -C ${BZIP2SYMDIR} libbz2.a

clean:
	rm -rf *~
	rm -rf ${SRC}/*~
	rm -rf ${BZIP2DIR}
	rm -rf ${BZIP2SYMDIR}
	rm -rf ${BUILD}