#ifndef LILY_H
#define LILY_H

#include <stdio.h>
#include "tune.h"

void write_lilyhead(FILE *lilyfile, char *name);

void print_note(MappingArray *durs, FILE *lilyfile, char *note, int duration);

void write_lilytail(FILE *lilyfile, int metronome);


#endif
