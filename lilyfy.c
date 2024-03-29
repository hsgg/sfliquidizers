#include <err.h>

#include "libc.h"
#include "lily.h"
#include "write_array.h"
#include "fpDEBUG.h"


int main (int argc, char *argv[])
{
    int j;
    double *lengths = NULL;
    double dur = 1.0, freq = 440, amp = 0.5;
    char *note = NULL;
    char *lastnote = NULL;
    double duration = 0.0;
    int metronome = 87;

    if (argc != 2)
	errx(1, "Usage: %s <title>\n", argv[0]);

    MappingArray fns = fns_tune();

    /* durations */
    MappingArray durs = dur_tune_metronome(metronome);


    /* read music */
    DBG("Analysing music...\n");
    j = 0;
    write_lilyhead(stdout, argv[1]);
    char *line = NULL;
    size_t maxlinelen = 0;
    int lineno = 0;
    while (mygetline(&line, &maxlinelen, stdin) != -1) {
        lineno++;
        if (line[0] == '#')
            continue;

        /* TODO: Fix parsing, and merge with synth.c parsing. */
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
        }

	if (!(note = get_str(&fns, freq)))
	    note = lastnote;

	if ((note == lastnote) || (duration == 0.0)) {
	    duration += dur;
	} else {
	    /* print last note */
	    print_note(&durs, stdout, lastnote, duration);
            lengths = myrealloc(lengths, (j + 1) * sizeof(*lengths));
	    lengths[j++] = duration;
	    duration = 0.0;
	}
	lastnote = note;
    }
    /* print last note */
    if (lastnote != note)
	errx(6, "Darn, I don't understand this algorithm!\n");
    print_note(&durs, stdout, lastnote, duration);
    lengths = myrealloc(lengths, (j + 1) * sizeof(*lengths));
    lengths[j++] = duration;
    write_lilytail(stdout, metronome);

    /* save detected frequencies and durations */
    write_to_file("durs.dat", j, lengths, 1, 0);
    write_histogram("durs_histo.dat", j, lengths, 5, get_maximalmax(&durs));

    free(lengths);

    return 0;
}

/* vim: set sts=4 sw=4 et: */
