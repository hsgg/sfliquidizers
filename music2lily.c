#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sndfile.h>

#include "libc.h"
#include "tune.h"
#include "freq.h"
#include "lily.h"
#include "write_array.h"
#include "fpDEBUG.h"


int main (int argc, char *argv[])
{
    int status = 0;
    int i, j;
    SNDFILE *file = NULL;
    SF_INFO wavinfo = {};
    int setsize = 0;
    int freqsize = 0;
    long long int frames = 0;
    double *music = NULL;
    int numfreqs = 0;
    double *freqs = NULL;
    double *lengths = NULL;
    double f;
    char *note = NULL;
    char *lastnote = NULL;
    int duration = 0;
    FILE *lilyfile = NULL;
    fft_cache *fft;
    int metronome = 87;


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

    DBG("frames: %ld\n", wavinfo.frames);
    DBG("samplerate: %d\n", wavinfo.samplerate);
    DBG("channels: %d\n", wavinfo.channels);
    DBG("format: %d\n", wavinfo.format);
    DBG("sections: %d\n", wavinfo.sections);
    DBG("seekable: %d\n", wavinfo.seekable);

    if (wavinfo.channels != 1) {
	printf("Haven't thought about what to do with more than one channel!...\n");
	exit (3);
    }


    /* set sizes */
    DBG("Set sizes...\n");
    {
	INCDBG;
	setsize = 1.0/440.0*6.0 * wavinfo.samplerate;
	freqsize = 100;
	DBG("setsize = %d\n", setsize);
	DBG("freqsize = %d\n", freqsize);
	double const df = wavinfo.samplerate / (double)setsize;
	double const dt = 1.0 / (double)wavinfo.samplerate;
	double const delta_t = dt * setsize;
	DBG("Frequency granularity: df = %.2lf Hz (%.2lf ms)\n", df, 1000.0 / df);
	DBG("Time granularity: dt = %lf ms (%.2lf Hz)\n", 1000.0 * dt, 1.0 / dt);
	DBG("Time interval: %lf ms (%.2lf Hz)\n", 1000.0 * delta_t, 1.0 / delta_t);
	fft = fft_init(setsize, freqsize);
	music = fft_inptr(fft);
	numfreqs = wavinfo.frames / setsize + 1;
	freqs = mymalloc(numfreqs * sizeof(double));
	lengths = mymalloc(numfreqs * sizeof(double));
	DBG("metronome = %d\n", metronome);
	DECDBG;
    }

    /* frequencies */
    MappingArray fns = fns_tune();

    /* durations */
    MappingArray durs = dur_tune_metronome(setsize / (double)wavinfo.samplerate,
	    metronome);


    /* read music */
    DBG("Analysing music...\n");
    i = 0;
    j = 0;
    write_lilyhead(lilyfile, argv[1]);
    frames = setsize;
    while (frames == setsize) {
	frames = sf_readf_double(file, music, setsize);

	f = freqs[i++] = get_frequency(fft, wavinfo.samplerate);

        double const d = setsize / (double)wavinfo.samplerate;
        printf("d=%lf f=%lf a=0.5\n", d, f);

	if (!(note = get_str(&fns, f)))
	    note = lastnote;

	if ((note == lastnote) || (duration == 0)) {
	    duration++;
	} else {
	    /* print last note */
	    print_note(&durs, lilyfile, lastnote, duration);
	    lengths[j++] = duration;
	    duration = 0;
	}
	lastnote = note;
    }
    /* print last note */
    if (lastnote != note) {
	printf("Darn, I don't understand this algorithm!\n");
	exit(6);
    }
    print_note(&durs, lilyfile, lastnote, duration);
    lengths[j++] = duration;
    write_lilytail(lilyfile, metronome);

    /* save detected frequencies and durations */
    if (i != numfreqs) {
	printf("Darn, I really don't understand this algorithm!\n");
	exit(7);
    }
    write_to_file("freqs.dat", numfreqs, freqs,
	    setsize / (double)wavinfo.samplerate, 0.0);
    write_to_file("durs.dat", j, lengths, 1, 0);
    write_histogram("durs_histo.dat", j, lengths, 5, get_maximalmax(&durs));


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

/* vim: set sts=4 sw=4 et: */
