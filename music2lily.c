/* vim: sts=4, sw=4 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sndfile.h>

#include "libc.h"
#include "tune.h"
#include "freq.h"
#include "lily.h"
#include "fpDEBUG.h"





/********** file data *********/

void write_to_file(char *filename, sf_count_t len, double *values, double xfactor, double xoffset)
{
    INCDBG;

    sf_count_t i = 0;
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



/********* synth ***********/

void synthesize(char *filename, double *freqs, int numfreqs, int setsize, SF_INFO wavinfo)
{
    SNDFILE *file = sf_open(filename, SFM_WRITE, &wavinfo);
    int i, n;
    double *synth = mymalloc(setsize * sizeof(double));
    double k = 2.0 * M_PI / wavinfo.samplerate;
    double phase = 0.0;
    double y1 = 0.0;
    double y2 = 0.0;
    double dt = k / (2.0 * M_PI);
    double f = 0.0;

    DBG("Synthesizing to file \"%s\"...\n", filename);

    /* checks */
    if (wavinfo.channels != 1) {
	printf("Error: Number of channels must be 1, currently.\n");
	exit(7);
    }
    if (!file) {
	printf("Could not open file \"%s\".\n", filename);
	exit(2);
    }

    /* synthesize */
    for (n = 0; n < numfreqs; n++) {
	f = k * freqs[n];

	phase = asin(2.0 * y2) - f * (-1);

	double yprime = (y2 - y1) / dt;
	double synprime = 0.5 * f / dt * cos(f * (-1.0 - 2.0) / 2.0 + phase);
	if (abs(yprime - synprime) > 0.01 * f / dt) {
	    phase = M_PI - phase - f * 2 * (-1);
	}

	for (i = 0; i < setsize; i++)
	    synth[i] = 0.5 * sin(f * i + phase);

	if (setsize >= 2) {
	    y1 = synth[setsize - 2];
	    y2 = synth[setsize - 1];
	}

	/* write to file */
	if (sf_write_double(file, synth, setsize) != setsize) {
	    printf("Couldn't write all frames! Don't know why.\n");
	    exit(8);
	}
    }

    /* free */
    if (sf_close(file) != 0) {
	printf("Error closing file.\n");
	exit(9);
    }
    free(synth);
}


/****************** main ****************/
int main (int argc, char *argv[])
{
    int status = 0;
    int i;
    SNDFILE *file = NULL;
    SF_INFO wavinfo = {};
    int setsize = 0;
    int freqsize = 0;
    long long int frames = 0;
    double *music = NULL;
    int numfreqs = 0;
    double *freqs = NULL;
    double f;
    char *note = NULL;
    char *lastnote = NULL;
    int duration = 0;
    FILE *lilyfile = NULL;
    tmp_fft fft = {};

    /* frequencies */
    MappingArray fns = fns_tune();

    /* durations */
    MappingArray durs = dur_tune();


    if (argc != 3) {
	printf("Usage: %s <sndfile> <lilyfile>\n", argv[0]);
	exit(1);
    }

    /* open file */
    file = sf_open(argv[1], SFM_READ, &wavinfo);
    if (!file) {
	printf("Could not open file \"%s\".\n", argv[1]);
	exit(2);
    }

    lilyfile = fopen(argv[2], "w");
    if (!lilyfile) {
	printf("Could not open output file \"%s\".\n", argv[2]);
	exit(5);
    }

    /* accounting data */
    DBG("filename: %s\n", argv[1]);
    DBG("title: %s\n", sf_get_string(file, SF_STR_TITLE));
    DBG("copyright: %s\n", sf_get_string(file, SF_STR_COPYRIGHT));
    DBG("software: %s\n", sf_get_string(file, SF_STR_SOFTWARE));
    DBG("artist: %s\n", sf_get_string(file, SF_STR_ARTIST));
    DBG("comment: %s\n", sf_get_string(file, SF_STR_COMMENT));
    DBG("date: %s\n", sf_get_string(file, SF_STR_DATE));

    DBG("frames: %lld\n", wavinfo.frames);
    DBG("samplerate: %d\n", wavinfo.samplerate);
    DBG("channels: %d\n", wavinfo.channels);
    DBG("format: %d\n", wavinfo.format);
    DBG("sections: %d\n", wavinfo.sections);
    DBG("seekable: %d\n", wavinfo.seekable);

    if (wavinfo.channels != 1) {
	printf("Haven't thought about what to do with more than one channel!...\n");
	exit (3);
    }

    write_lilyhead(lilyfile, argv[1]);

    /* set sizes */
    setsize = 1.0/440.0*6.0 * wavinfo.samplerate;
    freqsize = 100;
    fft = fft_init(setsize, freqsize);
    music = fft.in;
    numfreqs = wavinfo.frames / setsize + 1;
    freqs = mymalloc(numfreqs * sizeof(double));

    /* read music */
    i = 0;
    frames = setsize;
    while (frames == setsize) {
	frames = sf_readf_double(file, music, setsize);

	f = get_frequency(fft, wavinfo.samplerate);
	freqs[i++] = f;
	if (!(note = get_str(&fns, f)))
	    note = lastnote;

	if ((note == lastnote) || (duration == 0)) {
	    duration++;
	    lastnote = note;
	} else {
	    /* print last note */
	    print_note(&durs, lilyfile, lastnote, duration);
	    duration = 0;
	    lastnote = note;
	}
    }
    /* print last note */
    if (lastnote != note) {
	printf("Darn, I don't understand this algorithm!\n");
	exit(6);
    }
    print_note(&durs, lilyfile, lastnote, duration);

    write_to_file("freqs.dat", numfreqs, freqs, (double)setsize / wavinfo.samplerate, 0.0);

    write_lilytail(lilyfile);

    /* synthesize frequencies */
    synthesize("synth.wav", freqs, numfreqs, setsize, wavinfo);

    /* free resources, close files */
    if ((status = sf_close(file)) != 0) {
	printf("Error closing file.\n");
	exit(status);
    }
    fft_destroy(fft);
    free(freqs);
    fclose(lilyfile);

    return status;
}
