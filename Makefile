.SUFFIXES:      .c .o .s

CC = gcc
OPTIONS = -O3 -static -W -Wall -Wno-unused-parameter -Wno-unused-but-set-variable -fomit-frame-pointer -marm -mfpu=neon
PROGNAME = memperf
OBJECTS = lcpy.o cpy.o par.o

LIBRARIES = -lm

all:		$(PROGNAME)

armv7a:		DEFINES = -DARMV7ACOUNTER -DHWCOUNTER
armv7a:		$(PROGNAME)

armv7aopt:	DEFINES = -DARMV7ACOUNTER -DHWCOUNTER -DHAVEOPT
armv7aopt:	OBJECTS += cpy_p3opt.o
armv7aopt:	cpy_p3opt.o $(PROGNAME)

shmem:		PROGNAME = $(PROGNAME)_shared
shmem:		DEFINES = -DSHAREDMEM
shmem:		$(PROGNAME)

$(PROGNAME):	$(OBJECTS)
	$(CC) $(OPTIONS) -o $(PROGNAME) $(OBJECTS) $(LIBRARIES)

lcpy.o:	lcpy.c  cpy.h par.h
	$(CC) $(OPTIONS) $(DEFINES) -c lcpy.c -o lcpy.o

cpy.o:	cpy.h gettime.h gettimearmv7a.h
	$(CC) $(OPTIONS) $(DEFINES) -c cpy.c -o cpy.o

cpy_p3opt.o: cpy_p3opt.c
	$(CC) $(OPTIONS) $(DEFINES) -S cpy_p3opt.c -o cpy_p3opt.s
	$(CC) $(OPTIONS) $(DEFINES) -c cpy_p3opt.s -o cpy_p3opt.o

par.o:	par.c
	$(CC) $(OPTIONS) -c par.c -o par.o

clean:
	rm -f *~ *.o $(PROGNAME) $(PROGNAME)_shared *.s
