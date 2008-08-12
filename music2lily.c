/* vim: sts=4, sw=4 */
#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include <fftw3.h>

#include "libc.h"



void write_to_file(char *filename, sf_count_t len, double *values)
{
    sf_count_t i = 0;
    FILE *file = fopen(filename, "w");

    printf("Writing to file %s...\n", filename);

    if (!file) {
	printf("Could not open %s for writing.\n", filename);
	exit(2);
    }

    for (i = 0; i < len; i++) {
	fprintf(file, "%lld\t%lf\n", i, values[i]);
    }

    fclose(file);
}



int main (int argc, char *argv[])
{
    int status = 0;
    SNDFILE *file = NULL;
    SF_INFO wavinfo = {};
    sf_count_t setsize = 0;
    sf_count_t frames = 0;
    double *music = NULL;
    fftw_complex *data = NULL;
    fftw_plan plan;

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
    setsize = 8.5 * wavinfo.samplerate;
    music = mymalloc(setsize * sizeof(double));
    frames = sf_readf_double(file, music, setsize);

    /* save values */
    write_to_file("data.dat", frames, music);


    /* initialize in and out, 'couse that's what a plan's all about */
    printf("Creating a plan...\n");
    data = fftw_malloc(setsize * sizeof(fftw_complex));
    plan = fftw_plan_dft_1d(setsize, data, data, FFTW_FORWARD, FFTW_ESTIMATE); //TODO use FFTW_MEASURE


    printf("Executing the plan...\n");
    fftw_execute(plan);


    /* free resources, close files */
    fftw_destroy_plan(plan);
    fftw_free(data);
    if ((status = sf_close(file)) != 0) {
	printf("Error closing file.\n");
	exit(status);
    }

    return status;
}
