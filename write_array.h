#ifndef WRITE_ARRAY_H
#define WRITE_ARRAY_H


void write_to_file(char *filename, int len, double *values, double xfactor, double xoffset);

void write_histogram(char *filename, int len, double *values, double binsize, double maxvalue);

#endif
