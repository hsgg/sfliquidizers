/* vim: sts=4, sw=4 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sndfile.h>
#include <complex.h> /* must before fftw3.h */
#include <fftw3.h>

#include "libc.h"
#include "fpDEBUG.h"


typedef struct {
    double min;
    double freq;
    double max;
    char *note;
} Freq2Note;

typedef struct tmp_fft {
    int size;
    double *in;

    fftw_complex *data;
    fftw_plan plan;

    int freqsize;
    double *afreq;
} tmp_fft;


/********** lily *******/

char *get_note(double f)
{
    static char *note = NULL;
    static Freq2Note fns[] = {
	{ 288, 293, 296, "d'" },
	{ 318, 325, 331, "e'" },
	{ 365, 367, 369, "fis'" },
	{ 384, 389, 395, "g'" },
	{ 438, 440, 442, "a'" },
	{ 480, 487, 491, "b'" },
	{ 550, 555, 561, "cis''" },
	{ 585, 587, 588, "d''" }
    };
    int i = sizeof(fns) / sizeof(Freq2Note);

    while (i--) {
	if ((fns[i].min <= f) && (fns[i].max >= f)) {
	    note = fns[i].note;
	    break;
	}
    }

    /* If none is found, the last one will be used. */
    return note;
}


char *get_duration(int duration)
{
    INCDBG;
    static char *dur = NULL;
    static Freq2Note fns[] = {
	{ 82, 92, 100, "2" },
	{ 40, 46, 65, "4" },
	{ 20, 23, 25, "8" },
	{ 10, 11, 12, "16" },
	{  5,  6,  6, "32" },
	{  2,  3,  3, "64" }
    };
    int i = sizeof(fns) / sizeof(Freq2Note);

    while (i--) {
	DBG("Checking %lf ... %lf.\n", fns[i].min, fns[i].max);
	if ((fns[i].min <= (double)duration) && (fns[i].max >= (double)duration)) {
	    dur = fns[i].note;
	    break;
	}
    }

    if (i == -1) {
	printf("Duration %d not found!\n", duration);
    }

    /* If none is found, the last one will be used. */
    DECDBG;
    return dur;
}


void write_lilyhead(FILE *lilyfile,
	char *name)
{
    fprintf(lilyfile, "\\version \"2.11.52\"\n");

    fprintf(lilyfile, "\\header {\n");
    fprintf(lilyfile, "  title = \"%s\"\n", name);
    fprintf(lilyfile, "}\n\n");

    fprintf(lilyfile, "\"music\" = {\n");
}


void print_note(FILE *lilyfile, char *note, int duration)
{
    INCDBG;
    char *dur = get_duration(duration);

    DBG(">>> Printing duration is %d (1/%s)\n", duration, dur);
    DBG("=============>>> Printing %s%s.\n", note, dur);
    fprintf(lilyfile, "%s%s ", note, dur);

    DECDBG;
}

void write_lilytail(FILE *lilyfile)
{
    fprintf(lilyfile, "}\n\n");

    fprintf(lilyfile, "%% Score\n");
    fprintf(lilyfile, "\\score {\n");
    fprintf(lilyfile, "  \\new Staff = \"Tenor\" {\n");
    fprintf(lilyfile, "    \\clef alto\n");
    fprintf(lilyfile, "    \\time 4/4\n");
    fprintf(lilyfile, "    \\music\n");
    fprintf(lilyfile, "  }\n\n");

    fprintf(lilyfile, "  \\layout {\n");
    fprintf(lilyfile, "  }\n\n");

    fprintf(lilyfile, "  \\midi {\n");
    fprintf(lilyfile, "  }\n");

    fprintf(lilyfile, "}\n");
}


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


/******** fft *********/

tmp_fft fft_init(int setsize, int freqsize)
{
    INCDBG;

    tmp_fft fft = {};

    fft.size = setsize;
    fft.freqsize = freqsize;

    fft.in = mymalloc(setsize * sizeof(double));
    fft.afreq = mymalloc(freqsize * sizeof(double));

    /* create data, 'couse that's what's a plan's all about */
    DBG("Creating a plan...\n");
    fft.data = fftw_malloc((fft.size/2 + 1) * sizeof(fftw_complex));
    if (!fft.data) {
	printf("Error: Could not allocate 'fft.data'.\n");
	exit(-1);
    }
    fft.plan = fftw_plan_dft_r2c_1d(fft.size, fft.in, fft.data, FFTW_ESTIMATE); //TODO use FFTW_MEASURE

    DECDBG;
    return fft;
}

void fft_destroy(tmp_fft fft)
{
    /* free */
    free(fft.in);
    free(fft.afreq);
    fftw_destroy_plan(fft.plan);
    fftw_free(fft.data);
    fftw_cleanup();
}



double get_frequency(tmp_fft fft, double samplerate)
{
    INCDBG;
    int setsize = fft.size;
    int freqsize = fft.freqsize;
    double *afreq = fft.afreq;

    double avg, avg2, mass, stddev;
    int i, j, k;


    /* fft */
    DBG("Executing the plan...\n");
    fftw_execute(fft.plan);

    /* convert to real freqs */
    for (i = 0; (i < freqsize) && (i < fft.size/2 + 1); i++)
	afreq[i] = cabs(fft.data[i]) / sqrt(fft.size);
    while (i < freqsize)
	afreq[i++] = 0.0;


    /* average intensity, stddev */
    avg = 0.0;
    avg2 = 0.0;
    for (i = 0; i < freqsize; i++) {
	avg += afreq[i];
	avg2 += afreq[i] * afreq[i];
    }
    avg /= freqsize;
    avg2 /= freqsize;
    stddev = sqrt(avg2 - avg * avg);
    DBG("Average intensity: %lf\n", avg);
    DBG("Stddev: %lf\n", stddev);


    /* first maximum above 2 * stddev */
    mass = 0.0;
    i = 0;
    while (i < freqsize) {
	if (afreq[i] > mass)
	    mass = afreq[i];
	else if (mass > 2.0 * stddev) {
	    i--;
	    break;
	}
	i++;
    }

    /* average around first maximum */
    j = i;
    while (afreq[j] > 2.0 * stddev)
	j--;
    k = i;
    while (afreq[k] > 2.0 * stddev)
	k++;
    avg = 0.0;
    avg2 = 0.0;
    mass = 0.0;
    for (i = j + 1; i < k; i++) {
	DBG("Using in average: %lf (%lf)\n", i * samplerate / setsize, afreq[i]);
	avg += afreq[i] * i;
	avg2 += afreq[i] * i * i;
	mass += afreq[i];
    }
    avg /= mass;
    avg2 /= mass;
    avg *= samplerate / setsize;
    avg2 *= (samplerate / setsize) * (samplerate / setsize);
    stddev = sqrt(avg2 - avg * avg);
    DBG("Weighted average around first maximum: %lf +- %lf (%lf)\n", avg, stddev, mass);


    DECDBG;
    return avg;
}


/********* synth ***********/

void synthesize(char *filename, double *freqs, int numfreqs, int setsize, SF_INFO wavinfo)
{
    INCDBG;
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
	if (abs(yprime - synprime) > 0.2 * f / dt) {
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

    DECDBG;
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
    INCDBG;
    i = 0;
    frames = setsize;
    while (frames == setsize) {
	frames = sf_readf_double(file, music, setsize);

	f = get_frequency(fft, wavinfo.samplerate);
	DBG("i = %d (%lfsec), numfreqs = %d\n", i, i * (double)setsize / wavinfo.samplerate, numfreqs);
	freqs[i++] = f;
	DBG(">>> Frequency is %lf.\n", f);
	note = get_note(f);
	DBG(">>> Note is \"%s\" (%p).\n", note, note);

	if ((note == lastnote) || (duration == 0)) {
	    DBG("Same note as last time. duration = %d\n", duration);
	    duration++;
	    lastnote = note;
	} else {
	    /* print last note */
	    print_note(lilyfile, lastnote, duration);
	    duration = 0;
	    lastnote = note;
	}
    }
    /* print last note */
    if (lastnote != note) {
	printf("Darn, I don't understand this algorithm!\n");
	exit(6);
    }
    print_note(lilyfile, lastnote, duration);
    DECDBG;

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
