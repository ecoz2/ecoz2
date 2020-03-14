# ECOZ System

SOURCE_FILES=\
	lpc_signals.c \
	lpca.c \
    lpaOnSignal.c \
    prd.c \
	ref2raas.c

cc=gcc
cdebug=-g -O3 -Wall
cflags= -c -DDEBUG -O3 -ffast-math -std=c99

INCLUDE= -I ../include -I ../sgn

OBJ=../../_out/o

OBJECT_FILES = $(SOURCE_FILES:%.c=$(OBJ)/%.o)

build: $(OBJECT_FILES)

$(OBJECT_FILES): $(OBJ)/%.o: %.c
	@mkdir -p $(@D)
	$(cc) $(cdebug) $(cflags) $(INCLUDE) -o $@ $<
