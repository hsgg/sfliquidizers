CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lsndfile -lfftw3


HDR_C = libc.h
SRC_C = music2lily.c \
	$(HDR_C:.h=.c)

SRC = $(SRC_C)
OBJ = $(SRC_C:.c=.o)
PROG = music2lily

DEPFILE = .depend
PROGDEPFILE = .progdepend


# all
.PHONY: all
all: $(PROGDEPFILE) $(PROG)


# dependecies
$(DEPFILE): $(SRC)
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
	rm -f $(OBJ)
	rm -f $(PROG)
	rm -f $(DEPFILE) $(PROGDEPFILE)
