CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lsndfile -lfftw3 -lgsl


PROG = music2lily
SRC = $(PROG).c

DEPFILE = .depend
PROGDEPFILE = .progdepend


# all
.PHONY: all
all: $(PROGDEPFILE) $(PROG)


# dependecies
$(DEPFILE): *.c *.h
	$(CC) -MM $(SRC) > $(DEPFILE)

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
	rm -f $(PROG)
	rm -f $(DEPFILE) $(PROGDEPFILE)

.PHONY: realclean
realclean: clean
	rm -f *.dat
