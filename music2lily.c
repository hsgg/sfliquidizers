/* vim: sts=4, sw=4 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sndfile.h>
#include <complex.h> /* must before fftw3.h */
#include <fftw3.h>

#include "libc.h"


typedef struct {
    double min;
    double freq;
    double max;
    char *note;
} Freq2Note;



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
	printf("Checking %lf ... %lf.\n", fns[i].min, fns[i].max);
	if ((fns[i].min <= (double)duration) && (fns[i].max >= (double)duration)) {
	    dur = fns[i].note;
	    break;
	}
    }

    if (i == -1) {
	printf("Duration %d not found!\n", duration);
    }

    /* If none is found, the last one will be used. */
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
    char *dur = get_duration(duration);

    printf(">>> Printing duration is %d (1/%s)\n", duration, dur);
    printf("=============>>> Printing %s%s.\n", note, dur);
    fprintf(lilyfile, "%s%s ", note, dur);
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
/*    static int chunk = 0;*/
/*    static char fname[1000];*/
    int freqsize = 100;
    complex *freq = mymalloc(freqsize * sizeof(complex));
    double *afreq = mymalloc(freqsize * sizeof(double));

    double avg, avg2, mass, stddev;
    int i, j, k;

    double result = 0.0;

    fft(music, setsize, freq, freqsize);

    /* convert to real freqs */
    for (i = 0; i < freqsize; i++) {
	afreq[i] = 1.0 / sqrt(setsize) * cabs(freq[i]);
    }
/*    snprintf(fname, 999, "fft%.2d.dat", chunk++);*/
/*    write_to_file(fname, freqsize, afreq, samplerate / setsize);*/


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

    /* first maximum above 2 * stddev */
    mass = 0.0;
    i = 0;
    while (i < freqsize) {
	if (afreq[i] > mass)
	    mass = afreq[i];
	else if (mass > 2.0 * stddev) {
	    i--;
	    printf("First maximum above 2 * stddev: %lf (%lf)\n", i * (double)samplerate / setsize, afreq[i]);
	    result = i * (double)samplerate / setsize;
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
	printf("Using in average: %lf (%lf)\n", i * samplerate / setsize, afreq[i]);
	avg += afreq[i] * i;
	avg2 += afreq[i] * i * i;
	mass += afreq[i];
    }
    avg /= mass;
    avg2 /= mass;
    avg *= samplerate / setsize;
    avg2 *= (samplerate / setsize) * (samplerate / setsize);
    stddev = sqrt(avg2 - avg * avg);
    printf("Weighted average around first maximum: %lf +- %lf (%lf)\n", avg, stddev, mass);

    free(freq);
    free(afreq);

    //return result;
    return avg;
}



void synthesize(char *filename, double *freqs, int numfreqs, double xfactor, SF_INFO wavinfo)
{
    SNDFILE *file = NULL;
    int i, n;
    int setsize = xfactor * wavinfo.samplerate;
    double *synth = NULL;
    double k = 2.0 * M_PI / wavinfo.samplerate;
    double phase = 0.0;
    double x1 = 0.0;
    double x2 = 0.0;
    double dt = k / (2.0 * M_PI);
    double t1 = -2.0 * dt;
    double t2 = -1.0 * dt;
    double f = 0.0;

    printf("Synthesizing to file \"%s\"...\n", filename);

    if (wavinfo.channels != 1) {
	printf("Error: Number of channels must be 1, currently.\n");
	exit(7);
    }

    /* alloc, open */
    synth = mymalloc(setsize * sizeof(double));
    file = sf_open(filename, SFM_WRITE, &wavinfo);
    if (!file) {
	printf("Could not open file \"%s\".\n", filename);
	exit(2);
    }

    /* synthesize */
    for (n = 0; n < numfreqs; n++) {
	f = k * freqs[n];
	phase = asin(2.0 * x2) + f * t2;
	if (abs((x2-x1)/dt - 0.5 * cos(f * (t2+t1)/2.0 + phase)) > 0.1) {
	    phase = M_PI - phase;
	}

	for (i = 0; i < setsize; i++) {
	    synth[i] = 0.5 * sin(f * i + phase);
	}

	x1 = synth[i - 2];
	x2 = synth[i - 1];

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
    long long int setsize = 0;
    long long int frames = 0;
    double *music = NULL;
    int numfreqs = 0;
    double *freqs = NULL;
    double f;
    char *note = NULL;
    char *lastnote = NULL;
    int duration = 0;
    FILE *lilyfile = NULL;


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

    write_lilyhead(lilyfile, argv[1]);

    /* set sizes */
    setsize = 1.0/440.0*6.0 * wavinfo.samplerate;
    music = mymalloc(setsize * sizeof(double));
    numfreqs = wavinfo.frames / setsize + 1;
    freqs = mymalloc(numfreqs * sizeof(double));

    /* read music */
    i = 0;
    frames = setsize;
    while (frames == setsize) {
	frames = sf_readf_double(file, music, setsize);

	f = get_frequency(music, frames, wavinfo.samplerate);
	printf("i = %d (%lfsec), numfreqs = %d\n", i, i * (double)setsize / wavinfo.samplerate, numfreqs);
	freqs[i++] = f;
	printf(">>> Frequency is %lf.\n", f);
	note = get_note(f);
	printf(">>> Note is \"%s\" (%p).\n", note, note);

	if ((note == lastnote) || (duration == 0)) {
	    printf("Same note as last time. duration = %d\n", duration);
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

    write_to_file("freqs.dat", numfreqs, freqs, (double)setsize / wavinfo.samplerate);

    write_lilytail(lilyfile);

    /* synthesize frequencies */
    synthesize("synth.wav", freqs, numfreqs, (double)setsize / wavinfo.samplerate, wavinfo);

    /* free resources, close files */
    if ((status = sf_close(file)) != 0) {
	printf("Error closing file.\n");
	exit(status);
    }
    free(music);
    free(freqs);
    fclose(lilyfile);

    return status;
}
