/* vim: sts=4, sw=4 */
#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>

#include "libc.h"



int main (int argc, char *argv[])
{
    int status = 0;
    SNDFILE *file = NULL;
    SF_INFO wavinfo = {};
    double *music = NULL;
    sf_count_t setsize = 0;
    sf_count_t frames = 0;

    if (argc != 2) {
	printf("Usage: %s <sndfile>\n", argv[0]);
	exit(1);
    }

    /* open file */
    file = sf_open(argv[1], SFM_READ, &wavinfo);
    if (!file) {
	printf("Could not open file %s.\n", argv[1]);
	exit(2);
    }

    /* accounting data */
    printf("filename: %s\n", argv[1]);
    printf("title: %s\n", sf_get_string(file, SF_STR_TITLE));
    printf("copyright: %s\n", sf_get_string(file, SF_STR_COPYRIGHT));
    printf("software: %s\n", sf_get_string(file, SF_STR_SOFTWARE));
    printf("artist: %s\n", sf_get_string(file, SF_STR_ARTIST));
    printf("comment: %s\n", sf_get_string(file, SF_STR_COMMENT));
    printf("date: %s\n", sf_get_string(file, SF_STR_DATE));

    printf("frames: %lld\n", wavinfo.frames);
    printf("samplerate: %d\n", wavinfo.samplerate);
    printf("channels: %d\n", wavinfo.channels);
    printf("format: %d\n", wavinfo.format);
    printf("sections: %d\n", wavinfo.sections);
    printf("seekable: %d\n", wavinfo.seekable);

    if (wavinfo.channels != 1) {
	printf("Haven't thought about what to do with more than one channel!...\n");
	exit (3);
    }

    /* read music */
    music = mymalloc(wavinfo.samplerate * sizeof(double));
    frames = sf_readf_double(file, music, setsize);

    /* save values */




    /* close file */
    if ((status = sf_close(file)) != 0) {
	printf("Error closing file.\n");
	exit(status);
    }

    return status;
}
