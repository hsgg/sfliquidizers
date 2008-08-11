/* vim: sts=4, sw=4 */
#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>



int main (int argc, char *argv[])
{
    int status = 0;
    SNDFILE *file;
    SF_INFO wavinfo;

    if (argc != 2) {
	printf("Usage: %s <sndfile>\n", argv[0]);
	exit(-1);
    }

    /* open file */
    file = sf_open(argv[1], SFM_RDWR, &wavinfo);
    if (!file) {
	printf("Could not open file %s.\n", argv[1]);
	exit(-1);
    }

    /* close file */
    if ((status = sf_close(file)) != 0) {
	printf("Error closing file.\n");
	exit(status);
    }

    return status;
}
