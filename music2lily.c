/* vim: sts=4, sw=4 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sndfile.h>
#include <complex.h> /* must before fftw3.h */
#include <fftw3.h>
#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_multifit.h>

#include "libc.h"



void write_to_file(char *filename, sf_count_t len, double *values)
{
    sf_count_t i = 0;
    FILE *file = fopen(filename, "w");

    printf("Writing to file \"%s\"...\n", filename);

    if (!file) {
	printf("Could not open %s for writing.\n", filename);
	exit(2);
    }

    for (i = 0; i < len; i++) {
	fprintf(file, "%lld\t%lf\n", i, values[i]);
    }

    fclose(file);
}


void fft(double *music, int setsize)
{
    fftw_complex *data = NULL;
    fftw_plan plan;
    int i = 0;

    /* create data, 'couse that's what's a plan's all about */
    printf("Creating a plan...\n");
    data = fftw_malloc(setsize * sizeof(fftw_complex));
    plan = fftw_plan_dft_1d(setsize, data, data, FFTW_FORWARD, FFTW_ESTIMATE); //TODO use FFTW_MEASURE

    /* initialize */
    printf("Initializing data...\n");
    for (i = 0; i < setsize; i++)
	data[i] = music[i];

    /* fft */
    printf("Executing the plan...\n");
    fftw_execute(plan);

    /* convert fft */
    printf("Taking absolute value...\n");
    for (i = 0; i < setsize; i++)
	music[i] = cabs(data[i]);

    /* free */
    fftw_destroy_plan(plan);
    fftw_free(data);
    fftw_cleanup();
}



void fit(double *music, int setsize, double *fita, int freqsize)
{
    /* allocate */
    int i = 0, j = 0;
    int skip = 10, off = 0;

    printf("Allocating %d x %d = %d blocks.\n", setsize/skip + off, 2 * freqsize, setsize * 2 * freqsize);
    gsl_matrix *X = gsl_matrix_alloc(setsize/skip + off, 2 * freqsize);
    printf("Allocated X gsl matrix.\n");
    gsl_vector *y = gsl_vector_alloc(setsize/skip + off);
    gsl_vector *c = gsl_vector_alloc(2 * freqsize);
    gsl_matrix *cov = gsl_matrix_alloc(2 * freqsize, 2 * freqsize);
    double chisq = 0;
    gsl_multifit_linear_workspace *ws = gsl_multifit_linear_alloc(
	    setsize/skip + off, 2 * freqsize);

    printf("Allocated all gsl tensors.\n");

    /* setup */
    double k = 2.0 * M_PI / 44100.0;
    for (i = 0; i < setsize; i += skip) {
	printf("Setting X_%d_%d...\n", i, j);
	for (j = 0; j < freqsize; j++) {
	    gsl_matrix_set(X, i/skip, j, sin(k * i * j));
	    gsl_matrix_set(X, i/skip, j + freqsize, cos(k * i * j));
	}
	gsl_vector_set(y, i/skip, music[i]);
    }

    printf("Calculating fit...\n");

    /* calc */
    gsl_multifit_linear(X, y, c, cov, &chisq, ws);

    printf("Chi^2 = %lf\n", chisq);

    /* copy */
    for (i = 0; i < freqsize; i++) {
	fita[i] = gsl_vector_get(c, i);
	fita[i + freqsize] = gsl_vector_get(c, i + freqsize);
    }

    /* free */
    gsl_matrix_free(X);
    gsl_vector_free(y);
    gsl_vector_free(c);
    gsl_matrix_free(cov);
    gsl_multifit_linear_free(ws);
}



/****************** main ****************/
int main (int argc, char *argv[])
{
    int status = 0;
    int i = 0, j = 0;
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
    setsize = 0.5 * wavinfo.samplerate;
    freqsize = 40; //setsize;

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
    for (i = 0; i < setsize; i++)
	//music[i] = 0.1 * sin(12.0 * i * 2.0 * M_PI / 44100.0) + (exp(-((i-center)/sigma)*((i-center)/sigma))) * sin(44.0 * i * 2.0 * M_PI / 44100.0);
	music[i] = 0.1 * sin(12.0 * i * 2.0 * M_PI / 44100.0);

    /* save values */
    write_to_file("data.dat", frames, music);

    fft(music, setsize);

    write_to_file("fft.dat", freqsize, music);

    double *fita = mymalloc(2 * freqsize * sizeof(double));
    fit(music, setsize, fita, freqsize);

    write_to_file("fit.dat", 2 * freqsize, fita);

    /* return */
    for (i = 0; i < setsize; i++) {
	music[i] = 0.0;
	for (j = 0; j < freqsize; j++) {
	    music[i] += fita[j] * sin(2.0 * M_PI / 44100.0 * i * j);
	    music[i] += fita[j + freqsize] * cos(2.0 * M_PI / 44100.0 * i * j);
	    printf("%d: %lf\n", i, music[i]);
	}
    }
    write_to_file("rec.dat", setsize, music);


    /* frequency of maximum, average */
    for (i = 0; i < setsize; i++) {
	avgfreq += music[i] * i;
	mass += music[i];
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
