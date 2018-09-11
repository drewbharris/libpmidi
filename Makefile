CC=gcc
ALSA_OPTS=`pkg-config --libs --cflags alsa`
GSTREAMER_OPTS=`pkg-config --libs --cflags gstreamer-1.0`
CFLAGS=-g3 -c ${ALSA_OPTS}

all: src/libpmidi.a listports playfile

src/libpmidi.a: src/pmidi/elements.o src/pmidi/except.o src/pmidi/mdutil.o src/pmidi/midiread.o src/pmidi/pmidi.o src/pmidi/seqlib.o src/pmidi/seqlib.o src/pmidi/seqmidi.o
	ar rc src/libpmidi.a src/pmidi/*.o
	ranlib src/libpmidi.a

src/pmidi/elements.o: src/pmidi/elements.c
	$(CC) $(CFLAGS) src/pmidi/elements.c -o src/pmidi/elements.o

src/pmidi/except.o: src/pmidi/except.c
	$(CC) $(CFLAGS) src/pmidi/except.c -o src/pmidi/except.o

src/pmidi/mdutil.o: src/pmidi/mdutil.c
	$(CC) $(CFLAGS) src/pmidi/mdutil.c -o src/pmidi/mdutil.o

src/pmidi/midiread.o: src/pmidi/midiread.c
	$(CC) $(CFLAGS) src/pmidi/midiread.c -o src/pmidi/midiread.o

src/pmidi/pmidi.o: src/pmidi/pmidi.c
	$(CC) $(CFLAGS) src/pmidi/pmidi.c -o src/pmidi/pmidi.o

src/pmidi/seqlib.o: src/pmidi/seqlib.c
	$(CC) $(CFLAGS) src/pmidi/seqlib.c -o src/pmidi/seqlib.o

src/pmidi/seqmidi.o: src/pmidi/seqmidi.c
	$(CC) $(CFLAGS) src/pmidi/seqmidi.c -o src/pmidi/seqmidi.o

listports: src/libpmidi.a
	$(CC) -g3 src/listports.c -o bin/listports -L./src -lpmidi ${ALSA_OPTS} ${GSTREAMER_OPTS}

playfile: src/libpmidi.a
	$(CC) -g3 src/playfile.c -o bin/playfile -L./src -lpmidi ${ALSA_OPTS} ${GSTREAMER_OPTS}

clean:
	rm src/libpmidi.a src/pmidi/*.o bin/*