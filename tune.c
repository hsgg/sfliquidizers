#include <math.h>
#include "libc.h"
#include "tune.h"
#include "fpDEBUG.h"


static void print_mapping(Mapping *m)
{
    DBG("%p-> %-6s = %4d < %7.2lf < %4d, \t range %2d %+6.2lf %+6.2lf\n", m,
	    m->note, m->min, m->avg, m->max,
	    m->max - m->min, m->min - m->avg, m->max - m->avg);
}


MappingArray fns_tune()
{
    /* Mapping freqsandnotes[] = {
	{ 288, 293, 296, "d'" },
	{ 318, 325, 331, "e'" },
	{ 365, 367, 369, "fis'" },
	{ 384, 389, 395, "g'" },
	{ 438, 440, 443, "a'" },
	{ 480, 487, 491, "b'" },
	{ 550, 555, 561, "cis''" },
	{ 585, 587, 588, "d''" }
    }; */
    static Mapping const freqsandnotes[] = {
	{ 291, 297, 306, "d'" },
	{ 327, 336, 344, "e'" },
	{ 368, 377, 388, "fis'" },
	{ 386, 395, 404, "g'" },
	{ 436, 444, 452, "a'" },
	{ 492, 500, 508, "b'" },
	{ 516, 525, 534, "c''" },
	{ 586, 595, 604, "d''" },
	{ 659, 667, 675, "e''" },
    };

    DBG("Setting up note pitch ranges...\n");
    INCDBG;

    MappingArray fns = {};
    fns.size = sizeof(freqsandnotes) / sizeof(Mapping);
    fns.m = mymalloc(fns.size * sizeof(Mapping));

    int i = fns.size;
    while (i--) {
	fns.m[i] = freqsandnotes[i];
	print_mapping(&fns.m[i]);
    }

    DECDBG;
    return fns;
}


MappingArray dur_tune()
{
    static Mapping const dursanddurs[] = {
	{ 82, 92, 120, "2" },
	{ 36, 46, 65, "4" },
	{ 18, 23, 35, "8" },
	{ 10, 11, 16, "16" }
    };

    MappingArray durs = {};
    durs.size = sizeof(dursanddurs) / sizeof(Mapping);
    durs.m = mymalloc(durs.size * sizeof(Mapping));

    double const samplerate = 44100.0;
    double const setsize = 1.0/440.0*6.0 * samplerate;
    double const unit = setsize / samplerate;
    int i = durs.size;
    while (i--) {
	durs.m[i] = dursanddurs[i];
	durs.m[i].min *= unit;
	durs.m[i].avg *= unit;
	durs.m[i].max *= unit;
    }

    return durs;
}

/* dur_tune_metronome():
 * 	- quarter: The number of quarter notes per minute (like on a metonome). */
MappingArray dur_tune_metronome(double quarter)
{
    MappingArray durs = {};
    int length = 16;
    double lowermax;

    DBG("Calculating lengths with metronome mark %d...\n", (int)quarter);
    INCDBG;

    /* Length of a quarter note in seconds */
    quarter = 60.0 / quarter;

    lowermax = 4.0 / length * quarter * 0.75;

    do {
	int i = durs.size;
	durs.size += 2;
	durs.m = myrealloc(durs.m, durs.size * sizeof(Mapping));

	durs.m[i].avg = 4.0 / length * quarter;
	durs.m[i].min = lowermax;
	durs.m[i].max = lowermax = durs.m[i].avg * 1.10;
	durs.m[i].note = print2string(NULL, "%d", length);
	print_mapping(&durs.m[i]);

	i++;
	durs.m[i].avg = 1.5 * 4.0 / length * quarter;
	durs.m[i].min = lowermax;
	durs.m[i].max = lowermax = durs.m[i].avg * 1.10;
	durs.m[i].note = print2string(NULL, "%d.", length);
	print_mapping(&durs.m[i]);
    } while ((length /= 2) >= 1);

    DECDBG;
    return durs;
}



char *get_str(MappingArray *map, double value)
{
    char *str = NULL;
    int i = map->size;

    while (i--) {
	if ((map->m[i].min <= value) && (map->m[i].max >= value)) {
	    str = map->m[i].note;
	    break;
	}
    }

    return str;
}


int get_maximalmax(MappingArray *map)
{
    INCDBG;
    int max = 0;
    int i = map->size;

    while (i--) {
	Mapping m = map->m[i];

	DBG("%d: max = %d\n", i, max);
	print_mapping(&m);

	if (m.min > max)
	    max = m.min;

	if ((int)m.avg > max)
	    max = m.avg;

	if (m.max > max)
	    max = m.max;
    }

    DECDBG;
    return max;
}
