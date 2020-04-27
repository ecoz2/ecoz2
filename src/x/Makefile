# ECOZ System

include ../make.def

EXECUTABLE_NAMES=\
	sgn.show.c \
	sgn.endp.c \
	lpc.c \
	prd.show.c \
    vq.learn.c \
    vq.show.c \
    vq.quantize.c \
	hmm.learn.c \
	hmm.classify.c \
	hmm.show.c \
	seq.show.c \
    vq.classify.c

BIN=../../_out/bin
#libecoz=../../_out/lib/libecoz.a
libecoz=../../_out/o/*.o


EXECUTABLE_FILES = $(EXECUTABLE_NAMES:%.c=$(BIN)/%)

build: $(EXECUTABLE_FILES)

cflags= -Wall -g -DDEBUG -O3 -ffast-math -std=c99

INCLUDE= -I ../include -I ../sgn

$(EXECUTABLE_FILES): $(BIN)/% : %.c $(libecoz)
	@mkdir -p $(BIN)
	gcc $(cdebug) $(cflags) $(INCLUDE) -o $@ $^ -lm

tidy:
	rm -f *.o *~

clean: tidy
	rm -rf $(BIN)
