# ECOZ System

include ../make.def

SOURCE_FILES=\
	vq_learn.c \
	vq_quantize.c \
	vq_show.c \
	vq_classify.c \
	vq.c \
	distortion.c  \
	report.c  \
	sigma.c \
	inertia.c \
	quantize.c

INCLUDE= -I ../include -I ../sgn

OBJ=../../_out/o

OBJECT_FILES = $(SOURCE_FILES:%.c=$(OBJ)/%.o)

build: $(OBJECT_FILES)

$(OBJ)/vq_learn.o: vq_learn.i vq_learn_par.c vq_learn_ser.c

$(OBJECT_FILES): $(OBJ)/%.o: %.c
	@mkdir -p $(@D)
	$(cc) -c $(cdebug) $(cflags) $(INCLUDE) -o $@ $<
