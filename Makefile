# ECOZ System

OUTDIR=_out

SUBDIRS=src/utl \
        src/sgn \
        src/lpc \
        src/vq \
        src/hmm

.PHONY: all lib $(SUBDIRS)
   
all: $(SUBDIRS)
	@mkdir -p $(OUTDIR)/bin
	@$(MAKE) -C src/x/

#lib: $(SUBDIRS)
#	@mkdir -p $(OUTDIR)/lib
#	ar -r $(OUTDIR)/lib/libecoz.a $(OUTDIR)/o/*.o

$(SUBDIRS):
	@mkdir -p $(OUTDIR)/o
	@$(MAKE) -C $@

clean:
	@$(MAKE) -C src/x/ $@
	rm -rf $(OUTDIR)
	rm -rf $(OUTDIR)/*.dSYM
