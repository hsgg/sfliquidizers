#include <stdlib.h>
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
	printf("Could not open %s for writing.\n", filename);
	exit(2);
    }

    for (i = 0; i < len; i++) {
	fprintf(file, "%lf\t%lf\n", i * xfactor + xoffset, values[i]);
    }

    fclose(file);

    DECDBG;
}
