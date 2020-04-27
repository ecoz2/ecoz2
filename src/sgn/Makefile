# ECOZ System

include ../make.def

SOURCE_FILES=\
	dr_wav.c \
	sgn.c \
	endpoint.c

INCLUDE= -I ../include -I .

OBJ=../../_out/o

OBJECT_FILES = $(SOURCE_FILES:%.c=$(OBJ)/%.o)

build: $(OBJECT_FILES)

$(OBJECT_FILES): $(OBJ)/%.o: %.c
	@mkdir -p $(@D)
	$(cc) $(cdebug) $(cflags) $(INCLUDE) -o $@ $<
