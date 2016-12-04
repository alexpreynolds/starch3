PRODUCT = starch3
FLAGS = -Wall -Wno-exit-time-destructors -Wno-global-constructors -Wno-padded -std=c++11
CWD = $(shell pwd)
SRC = ${CWD}/src
INCLUDE = ${CWD}/include
BUILD = ${CWD}/build
THIRD_PARTY = ${CWD}/third-party
BZIP2_ARC = ${THIRD_PARTY}/bzip2-1.0.6.tar.gz
BZIP2_DIR = ${THIRD_PARTY}/bzip2-1.0.6
BZIP2_SYM_DIR = ${THIRD_PARTY}/bzip2
BZIP2_INC_DIR = ${BZIP2_SYM_DIR}
BZIP2_LIB_DIR = ${BZIP2_SYM_DIR}
JSON_ARC = ${THIRD_PARTY}/jansson-2.9.tar.gz
JSON_DIR = ${THIRD_PARTY}/jansson-2.9
JSON_SYM_DIR = ${THIRD_PARTY}/jansson
JSON_INC_DIR = ${JSON_SYM_DIR}/include
JSON_LIB_DIR = ${JSON_SYM_DIR}/lib
FLAGS2 = -O3 -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DDEBUG
INC = -I${SRC} -I${INCLUDE} -I${BZIP2_INC_DIR} -I${JSON_INC_DIR}
UNAME := $(shell uname -s)

ifeq ($(UNAME),Darwin)
	CC = clang
	CXX = clang++
	FLAGS += -Weverything -Wno-c++98-compat-pedantic
endif

all: prep bzip2 jansson
	${CXX} ${FLAGS} ${FLAGS2} ${INC} -c "${SRC}/starch3.cpp" -o "${BUILD}/starch3.o" 
	${CXX} ${FLAGS} ${FLAGS2} ${INC} -L"${BZIP2_LIB_DIR}" -L"${JSON_LIB_DIR}" "${BUILD}/starch3.o" -o "${BUILD}/${PRODUCT}" -lbz2 -lpthread -ljansson

prep:
	@if [ ! -d "${BUILD}" ]; then mkdir "${BUILD}"; fi

bzip2:
	@if [ ! -d "${BZIP2_DIR}" ]; then \
		mkdir "${BZIP2_DIR}"; \
		tar zxvf "${BZIP2_ARC}" -C "${THIRD_PARTY}"; \
		ln -sf ${BZIP2_DIR} ${BZIP2_SYM_DIR}; \
		${MAKE} -C ${BZIP2_SYM_DIR} libbz2.a CC=${CC}; \
	fi

jansson:
	@if [ ! -d "${JSON_DIR}" ]; then \
		mkdir "${JSON_DIR}"; \
		tar zxvf "${JSON_ARC}" -C "${THIRD_PARTY}"; \
		ln -sf ${JSON_DIR} ${JSON_SYM_DIR}; \
		cd ${JSON_SYM_DIR} && ./configure --prefix=${JSON_SYM_DIR} && make && make install && cd ${CWD}; \
	fi

clean:
	rm -rf *~
	rm -rf ${SRC}/*~
	rm -rf ${BZIP2_DIR}
	rm -rf ${BZIP2_SYM_DIR}
	rm -rf ${JSON_DIR}
	rm -rf ${JSON_SYM_DIR}
	rm -rf ${BUILD}