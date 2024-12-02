# makefile for sampgen library
POBJS = breakpoint.o waveform.o tuning.o envelope.o
CFILES = breakpoint.c waveform.c tuning.c envelope.c

CFLAGS = -lm

CC = gcc

.c.o: $(CC) -c $(CFLAGS) $< -o $@

.PHONY:	clean veryclean
all:	libsampgen.a

clean:
	-rm -f $(POBJS)

veryclean:
	-rm -f $(POBJS)
	rm -f libsampgen.a;

libsampgen.a:	$(POBJS)
	ar -rc libsampgen.a $(POBJS)
	ranlib libsampgen.a

install:	libsampgen.a
	cp libsampgen.a ../../lib
# dependencies:
CFILES:	../include/sampgen.h
