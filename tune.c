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
	{ 82, 92, 100, "2" },
	{ 36, 46, 65, "4" },
	{ 20, 23, 25, "8" },
	{ 10, 11, 12, "16" }
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


