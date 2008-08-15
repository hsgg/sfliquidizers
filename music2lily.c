/* vim: sts=4, sw=4 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sndfile.h>
#include <complex.h> /* must before fftw3.h */
#include <fftw3.h>

#include "libc.h"



void write_to_file(char *filename, sf_count_t len, double *values, double xfactor)
{
    sf_count_t i = 0;
    FILE *file = fopen(filename, "w");

    printf("Writing to file \"%s\"...\n", filename);

    if (!file) {
	printf("Could not open %s for writing.\n", filename);
	exit(2);
    }

    for (i = 0; i < len; i++) {
	fprintf(file, "%lf\t%lf\n", i * xfactor, values[i]);
    }

    fclose(file);
}


void fft(double *music, int setsize, complex *freq, int freqsize)
{
    fftw_complex *data = NULL;
    fftw_plan plan;
    int i = 0;

    /* create data, 'couse that's what's a plan's all about */
    printf("Creating a plan...\n");
    data = fftw_malloc((setsize/2 + 1) * sizeof(fftw_complex));
    plan = fftw_plan_dft_r2c_1d(setsize, music, data, FFTW_ESTIMATE); //TODO use FFTW_MEASURE

    /* fft */
    printf("Executing the plan...\n");
    fftw_execute(plan);

    /* convert fft */
    printf("Copying result...\n");
    for (i = 0; i < freqsize; i++)
	freq[i] = data[i];

    /* free */
    fftw_destroy_plan(plan);
    fftw_free(data);
    fftw_cleanup();
}



/****************** main ****************/
int main (int argc, char *argv[])
{
    int status = 0;
    int i = 0;
    SNDFILE *file = NULL;
    SF_INFO wavinfo = {};
    long long int setsize = 0;
    long long int frames = 0;
    long long int freqsize = 0;
    double *music = NULL;
    double avgfreq = 0, mass = 0;

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

    /* set sizes */
    setsize = 8.200 * wavinfo.samplerate;
    freqsize = 10000;

    /* read music */
    music = mymalloc(setsize * sizeof(double));
    frames = sf_readf_double(file, music, setsize);
    if (frames != setsize) {
	printf("Error: Impossible! setsize (%lld) = frames (%lld)\n", setsize, frames);
	exit(3);
    }

    /* initialize */
    //double sigma = 5050.0;
    //int center = 22050;
    //for (i = 0; i < setsize; i++)
	//music[i] = 1.0 * sin(656.0 * i * 2.0 * M_PI / wavinfo.samplerate) + /*(exp(-((i-center)/sigma)*((i-center)/sigma))) */ sin(440.0 * i * 2.0 * M_PI / wavinfo.samplerate);
	//music[i] = 0.1 * sin(12.0 * i * 2.0 * M_PI / wavinfo.samplerate);

    /* save values */
    write_to_file("data.dat", setsize, music, 1.0 / wavinfo.samplerate);

    complex *freq = mymalloc(freqsize * sizeof(complex));
    fft(music, setsize, freq, freqsize);

    /* convert to real freqs */
    for (i = 0; i < freqsize; i++) {
	music[i] = 1.0 / sqrt(setsize) * cabs(freq[i]);
    }
    write_to_file("fft.dat", freqsize, music, (double)wavinfo.samplerate / setsize);


    /* frequency of maximum, average */
    for (i = 0; i < freqsize; i++) {
	avgfreq += cabs(freq[i]) * i;
	mass += cabs(freq[i]);
    }
    avgfreq /= mass;
    printf("Average frequency: %lf\n", avgfreq);



    /* free resources, close files */
    if ((status = sf_close(file)) != 0) {
	printf("Error closing file.\n");
	exit(status);
    }
    free(music);

    return status;
}
