#include <stdlib.h>
#include <err.h>
#include "libc.h"
#include "write_array.h"
#include "fpDEBUG.h"


/********** file data *********/

void write_to_file(char *filename, int len, double *values, double xfactor, double xoffset)
{
    INCDBG;

    int i = 0;
    FILE *file = fopen(filename, "w");

    DBG("Writing to file \"%s\" with xfactor %lf and xoffset %lf...\n", filename, xfactor, xoffset);

    if (!file) {
	warnx("Could not open %s for writing.\n", filename);
	exit(2);
    }

    for (i = 0; i < len; i++) {
	fprintf(file, "%lf\t%lf\n", i * xfactor + xoffset, values[i]);
    }

    fclose(file);

    DECDBG;
}


void write_histogram(char *filename, int len, double *values, double binsize, double maxvalue)
{
    INCDBG;

    int i = 0;
    int histsize = maxvalue / binsize + 1;
    double *histo = mymalloc(histsize * sizeof(double));

    DBG("Calculating histogram with binsize %lf and maxvalue %lf...\n",
	    binsize, maxvalue);

    /* initialize */
    for (i = 0; i < histsize; i++)
	histo[i] = 0;

    /* calculate histogram */
    for (i = 0; i < len; i++) {
	int const bin = values[i] / binsize;
	if (bin >= histsize) {
	    warnx("Ignoring too high value %lf in histogram!\n", values[i]);
	    continue;
	}
	histo[bin]++;
    }

    write_to_file(filename, histsize, histo, binsize, 0);

    free(histo);

    DECDBG;
}
