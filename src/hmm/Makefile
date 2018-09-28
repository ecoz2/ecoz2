# ECOZ System

SOURCE_FILES=\
	hmm.c  \
	hmm_adjustb.c \
	hmm_file.c  \
	hmm_refinement.c \
	hmm_log_prob.c \
	hmm_genQopt.c \
	hmm_estimateB.c  \
	hmm_gen.c \
	symbol.c  \
	distr.c

cc=gcc
cdebug=-g -O3 -Wall
cflags= -c -DDEBUG -O3 -ffast-math -std=c99

INCLUDE= -I ../include

OBJ=../../_out/o

OBJECT_FILES = $(SOURCE_FILES:%.c=$(OBJ)/%.o)

build: $(OBJECT_FILES)

$(OBJECT_FILES): $(OBJ)/%.o: %.c
	@mkdir -p $(@D)
	$(cc) $(cdebug) $(cflags) $(INCLUDE) -o $@ $<
