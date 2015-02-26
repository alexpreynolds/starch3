CC=g++
PRODUCT=starch3
FLAGS=-Wall -Weverything -Wno-exit-time-destructors

all:
	${CC} ${FLAGS} -c starch3.cpp
	${CC} ${FLAGS} starch3.o -o ${PRODUCT}

clean:
	rm -rf *.o *~ ${PRODUCT}