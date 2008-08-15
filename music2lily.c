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
    if (!data) {
	printf("Error: Could not allocate 'data'.\n");
	exit(-1);
    }
    plan = fftw_plan_dft_r2c_1d(setsize, music, data, FFTW_ESTIMATE); //TODO use FFTW_MEASURE

    /* fft */
    printf("Executing the plan...\n");
    fftw_execute(plan);

    /* convert fft */
    printf("Copying result...\n");
    for (i = 0; (i < freqsize) && (i < setsize/2 + 1); i++)
	freq[i] = data[i];
    while (i < freqsize) {
	freq[i++] = 0.0;
    }

    /* free */
    fftw_destroy_plan(plan);
    fftw_free(data);
    fftw_cleanup();
}


double get_frequency(double *music, int setsize, double samplerate)
{
    int freqsize = 2000;
    complex *freq = mymalloc(freqsize * sizeof(complex));
    double *afreq = mymalloc(freqsize * sizeof(double));

    double avg, mass, stddev;
    int i;

    fft(music, setsize, freq, freqsize);

    /* convert to real freqs */
    for (i = 0; i < freqsize; i++) {
	afreq[i] = 1.0 / sqrt(setsize) * cabs(freq[i]);
    }
    write_to_file("fft.dat", freqsize, afreq, samplerate / setsize);


    /* frequency of maximum, average */
    avg = 0.0;
    mass = 0.0;
    for (i = 0; i < freqsize; i++) {
	avg += afreq[i] * i * (double)samplerate / setsize;
	mass += afreq[i];
    }
    avg /= mass;
    printf("Average frequency: %lf\n", avg);

    /* average intensity, stddev */
    avg = 0.0;
    mass = 0.0; // avg^2, really
    for (i = 0; i < freqsize; i++) {
	avg += afreq[i];
	mass += afreq[i] * afreq[i];
    }
    avg /= freqsize;
    mass /= freqsize;
    stddev = sqrt(mass - avg * avg);
    printf("Average intensity: %lf\n", avg);
    printf("Stddev: %lf\n", stddev);

    /* above 2 * stddev */
    i = 0;
    while (i < freqsize) {
	if (afreq[i] >= 2.0 * stddev)
	    printf("Above 2 * stddev: %lf (%lf)\n", i * (double)samplerate / setsize, afreq[i]);
	i++;
    }

    /* first maximum */
    mass = 0.0;
    i = 0;
    while (i < freqsize) {
	if (afreq[i] >= 2.0 * stddev) {
	    printf("Next: %lf (%lf)\n", i * (double)samplerate / setsize, afreq[i]);
	    if (afreq[i] > mass)
		mass = afreq[i];
	    else
		break;
	}
	i++;
    }
    i--;
    printf("First maximum: %lf (%lf)\n", i * (double)samplerate / setsize, afreq[i]);

    free(freq);
    free(afreq);

    return i * (double)samplerate / setsize;
}



/****************** main ****************/
int main (int argc, char *argv[])
{
    int status = 0;
    int i;
    SNDFILE *file = NULL;
    SF_INFO wavinfo = {};
    long long int setsize = 0;
    long long int frames = 0;
    double *music = NULL;
    int numnotes = 0;
    double *notes = NULL;
    double f;

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
    setsize = 0.05 * wavinfo.samplerate;
    music = mymalloc(setsize * sizeof(double));
    numnotes = wavinfo.frames / setsize + 1;
    notes = mymalloc(numnotes * sizeof(double));

    /* read music */
    i = 0;
    frames = setsize;
    while (frames == setsize) {
	frames = sf_readf_double(file, music, setsize);

	/* save values */
	write_to_file("data.dat", frames, music, 1.0 / wavinfo.samplerate);


	f = get_frequency(music, frames, wavinfo.samplerate);
	printf("i = %d, numnotes = %d\n", i, numnotes);
	notes[i++] = f;
	printf("=================>>> Frequency is %lf.\n", f);
    }

    write_to_file("notes.dat", numnotes, notes, (double)setsize / wavinfo.samplerate);


    /* free resources, close files */
    if ((status = sf_close(file)) != 0) {
	printf("Error closing file.\n");
	exit(status);
    }
    free(music);
    free(notes);

    return status;
}
