#include "libc.h"
#include "tune.h"
#include "fpDEBUG.h"


MappingArray fns_tune()
{
    Mapping freqsandnotes[] = {
	{ 288, 293, 296, "d'" },
	{ 318, 325, 331, "e'" },
	{ 365, 367, 369, "fis'" },
	{ 384, 389, 395, "g'" },
	{ 438, 440, 442, "a'" },
	{ 480, 487, 491, "b'" },
	{ 550, 555, 561, "cis''" },
	{ 585, 587, 588, "d''" }
    };

    MappingArray fns = {};
    fns.size = sizeof(freqsandnotes) / sizeof(Mapping);
    fns.m = mymalloc(fns.size * sizeof(Mapping));

    int i = fns.size;
    while (i--) {
	fns.m[i] = freqsandnotes[i];
	DBG("note[%d] = %s\n", i, freqsandnotes[i].note);
	/* The string needs to be copied explicitly: */
	fns.m[i].note = realloc_strcpy(NULL, freqsandnotes[i].note);
    }

    return fns;
}


MappingArray dur_tune()
{
    Mapping dursanddurs[] = {
	{ 82, 92, 120, "2" },
	{ 36, 46, 65, "4" },
	{ 18, 23, 35, "8" },
	{ 10, 11, 16, "16" }
    };

    MappingArray durs = {};
    durs.size = sizeof(dursanddurs) / sizeof(Mapping);
    durs.m = mymalloc(durs.size * sizeof(Mapping));

    int i = durs.size;
    while (i--) {
	durs.m[i] = dursanddurs[i];
	/* The string needs to be copied explicitly: */
	durs.m[i].note = realloc_strcpy(NULL, dursanddurs[i].note);
    }

    return durs;
}

void print_mapping(Mapping *m)
{
    DBG("%p->min = %d\n", m, m->min);
    DBG("%p->avg = %f\n", m, m->avg);
    DBG("%p->max = %d\n", m, m->max);
    DBG("%p->note = \"%s\"\n", m, m->note);
    DBG("\n");
}

/* dur_tune_metronome():
 * 	- unit: The length of an elementary unit.
 * 	- quarter: The number of quarter notes per minute (like on a metonome). */
MappingArray dur_tune_metronome(double const unit, double quarter)
{
    MappingArray durs = {};
    int length = 16;
    int lowermax = 4.0 / length * quarter / unit * 0.75;

    /* Length of a quarter note in seconds */
    quarter = 60.0 / quarter;

    do {
	int i = durs.size;
	durs.size += 2;
	durs.m = myrealloc(durs.m, durs.size * sizeof(Mapping));

	durs.m[i].avg = 4.0 / length * quarter / unit;
	durs.m[i].min = lowermax;
	durs.m[i].max = lowermax = durs.m[i].avg * 1.10;
	durs.m[i].note = print2string(NULL, "%d", length);
	print_mapping(&durs.m[i]);

	i++;
	durs.m[i].avg = 1.5 * 4.0 / length * quarter / unit;
	durs.m[i].min = lowermax;
	durs.m[i].max = lowermax = durs.m[i].avg * 1.10;
	durs.m[i].note = print2string(NULL, "%d.", length);
	print_mapping(&durs.m[i]);
    } while ((length /= 2) >= 1);

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


