# ECOZ System

include ../make.def

SOURCE_FILES=\
	hmm.c  \
	hmm_learn.c \
	hmm_classify.c \
	hmm_show.c \
	hmm_adjustb.c \
	hmm_file.c  \
	hmm_prob.c \
	hmm_log_prob.c \
	hmm_genQopt.c \
	hmm_estimateB.c  \
	hmm_gen.c \
	seq_show_files.c  \
	symbol.c  \
	distr.c

INCLUDE= -I ../include

OBJ=../../_out/o

OBJECT_FILES = $(SOURCE_FILES:%.c=$(OBJ)/%.o)

build: $(OBJECT_FILES)

$(OBJECT_FILES): $(OBJ)/%.o: %.c
	@mkdir -p $(@D)
	$(cc) -c $(cdebug) $(cflags) $(INCLUDE) -o $@ $<
