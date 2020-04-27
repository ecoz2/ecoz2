# ECOZ System

SOURCE_FILES=\
	ecoz2.c

cc=gcc
#cdebug=-g -DDEBUG
cflags=-c -O3 -Wall -ffast-math -std=c99

INCLUDE= -I ../include

OBJ=../../_out/o

OBJECT_FILES = $(SOURCE_FILES:%.c=$(OBJ)/%.o)

build: $(OBJECT_FILES)

$(OBJECT_FILES): $(OBJ)/%.o: %.c
	@mkdir -p $(@D)
	$(cc) $(cdebug) $(cflags) $(INCLUDE) -o $@ $<
