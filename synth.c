/* vim: set sts=4 sw=4 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sndfile.h>
#include <err.h>

#include "libc.h"
#include "fpDEBUG.h"

/*
 * output format: The output format is a line-oriented format.  Each
 * line is a semicolon-separated list of the following fields.
 * 	d=<duration> f=<freq> a=<amplitude>;
 * Each field can be specified optionally with deviations, e.g.
 * 440.27+4.1-3.5Hz.
 * If a field is left out, then the last value is taken, irrespective
 * of where it was specified.  The note is only printed, when the
 * semicolon is reached.
 * Example:
 * 	d=0.2s f=440.27Hz a=1.0; f=440.28Hz a=0.9;
 * This specifies two concurrent tones each lasting 0.2 seconds, at
 * almost the same frequency, the latter being slightly less loud.
 */

int main(int argc, char *argv[])
{
    SF_INFO auinfo = {
        .samplerate = 44100,
        .channels = 1,
        .format = SF_FORMAT_AU | SF_FORMAT_PCM_16,
        .seekable = 0
    };
    int i;
    int setsize;
    double *synth = NULL;
    double k = 2.0 * M_PI / auinfo.samplerate;
    double phase = 0.0;
    double y1 = 0.0;
    double y2 = 0.0;
    double dt = k / (2.0 * M_PI);
    double f = 0.0;


    SNDFILE *file = sf_open_fd(fileno(stdout), SFM_WRITE, &auinfo, SF_TRUE);
    if (!file)
	err(2, "libsndfile could not open stdout");

    double freq = 440.0, dur = 1.0, amp = 1.0;

    char *line = NULL;
    size_t maxlinelen = 0;
    int lineno = 0;
    while (mygetline(&line, &maxlinelen, stdin) != -1) {
        lineno++;
        if (line[0] == '#')
            continue;

        char *currfreq, *nextfreq = line;
        while ((currfreq = strsep(&nextfreq, ";\n")) != NULL) {
            char *currspec, *nextspec = currfreq;
            while ((currspec = strsep(&nextspec, " \t;\n")) != NULL) {
                char *name = strsep(&currspec, "=");
                char *value = currspec;
                switch (*name) {
                case 'd':
                    dur = atof(value);
                    break;
                case 'f':
                    freq = atof(value);
                    break;
                case 'a':
                    amp = atof(value);
                    break;
		case '\0':
		    break;
                default:
                    warnx("WARNING on line %d: Unrecognized specification "
                            "%s=%s", lineno, name, value);
                }
            }

            /* TODO: Use all of the input tones. That means re-thinking the
             * mathematics for converting frequencies to sine-waves. */
        }

	/* synthesize */
        f = k * freq;

        phase = asin(y2) - f * (-1);

        double yprime = (y2 - y1) / dt;
        double synprime = f / dt * cos(f * (-1.0 - 2.0) / 2.0 + phase);
        if (abs(yprime - synprime) > 0.01 * f / dt) {
            phase = M_PI - phase - f * 2 * (-1);
        }

        setsize = dur * auinfo.samplerate;
        synth = myrealloc(synth, setsize * sizeof(*synth));
        for (i = 0; i < setsize; i++)
            synth[i] = amp * sin(f * i + phase);

        if (setsize >= 2) {
            y1 = sin(f * (setsize - 2) + phase);
            y2 = sin(f * (setsize - 1) + phase);
        }

        /* write to file */
        if (sf_write_double(file, synth, setsize) != setsize)
            err(8, "Couldn't write all frames!");
    }


    if (sf_close(file) != 0)
	err(9, "Error closing file");
    free(synth);
    free(line);

    return 0;
}
