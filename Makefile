CFLAGS := -g -Wall -Werror -O2 -mfpmath=sse
LDFLAGS := -Wl,--as-needed
LDLIBS :=

CFLAGS += $(shell pkg-config --cflags sndfile)
LDLIBS += $(shell pkg-config --libs sndfile)

CFLAGS += $(shell pkg-config --cflags fftw3) -DUSE_FFTW3
LDLIBS += $(shell pkg-config --libs fftw3)

PROGS = music2lily synth

DEPFILE = .depend
PROGDEPFILE = .progdepend


# all
.PHONY: all
all: $(PROGDEPFILE) $(PROGS)


# dependecies
$(DEPFILE): *.c *.h
	$(CC) -MM *.c > $(DEPFILE)

$(PROGDEPFILE): $(DEPFILE)
	# Technically, the dependecies-only should be removed.
	sed -e "s/\.o//g" -e "s/\.cpp/.o/g" -e "s/\.c/.o/g" -e "s/\.h/.o/g" \
		$(DEPFILE) > $(PROGDEPFILE)

-include $(PROGDEPFILE)
-include $(DEPFILE)


# cleaners
.PHONY: clean
clean:
	rm -f *.o
	rm -f $(PROGS)
	rm -f $(DEPFILE) $(PROGDEPFILE)

.PHONY: realclean
realclean: clean
	rm -f *.dat
