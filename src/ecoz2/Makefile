# ECOZ System

include ../make.def

SOURCE_FILES=\
	ecoz2.c

INCLUDE= -I ../include

OBJ=../../_out/o

OBJECT_FILES = $(SOURCE_FILES:%.c=$(OBJ)/%.o)

build: $(OBJECT_FILES)

$(OBJECT_FILES): $(OBJ)/%.o: %.c
	@mkdir -p $(@D)
	$(cc) -c $(cdebug) $(cflags) $(INCLUDE) -o $@ $<
