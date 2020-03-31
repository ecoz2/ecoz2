# ECOZ System

OUTDIR=_out

SUBDIRS=src/utl \
        src/ecoz \
        src/sgn \
        src/lpc \
        src/vq \
        src/hmm

.PHONY: all lib $(SUBDIRS)
   
all: $(SUBDIRS)
	@mkdir -p $(OUTDIR)/bin
	@$(MAKE) -C src/x/

lib: $(SUBDIRS)
	@mkdir -p $(OUTDIR)/lib
# 	gcc -dynamiclib -o $(OUTDIR)/lib/libecoz.dylib $(OUTDIR)/o/*.o
	ar -r $(OUTDIR)/lib/libecoz.a $(OUTDIR)/o/*.o

$(SUBDIRS):
	@mkdir -p $(OUTDIR)/o
	@$(MAKE) -C $@

clean:
	@$(MAKE) -C src/x/ $@
	rm -rf $(OUTDIR)
	rm -rf $(OUTDIR)/*.dSYM
