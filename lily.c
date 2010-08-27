#include "fpDEBUG.h"
#include "lily.h"



/********** lily *******/

void write_lilyhead(FILE *lilyfile,
	char *name)
{
    fprintf(lilyfile, "\\version \"2.11.55\"\n");

    fprintf(lilyfile, "\\header {\n");
    fprintf(lilyfile, "  title = \"%s\"\n", name);
    fprintf(lilyfile, "}\n\n");

    fprintf(lilyfile, "\"music\" = {\n");
}


void print_note(MappingArray *durs, FILE *lilyfile, char *note, double duration)
{
    char *dur = get_str(durs, duration);

    if (dur && note) {
	DBG(">>> dur = %lf >>> Printing %s%s\n", duration, note, dur);
	fprintf(lilyfile, "%s%s ", note, dur);
    } else {
	DBG("<<< dur = %lf <<< NOT printing %s%s\n", duration, note, dur);
    }
}

void write_lilytail(FILE *lilyfile, int metronome)
{
    fprintf(lilyfile, "\n");
    fprintf(lilyfile, "\\bar \"|.\"\n");
    fprintf(lilyfile, "}\n\n");

    fprintf(lilyfile, "%% Score\n");
    fprintf(lilyfile, "\\score {\n");
    fprintf(lilyfile, "  \\new Staff = \"Tenor\" {\n");
    fprintf(lilyfile, "    \\clef treble\n");
    fprintf(lilyfile, "    \\time 3/4\n");
    fprintf(lilyfile, "    \\music\n");
    fprintf(lilyfile, "  }\n\n");

    fprintf(lilyfile, "  \\layout {\n");
    fprintf(lilyfile, "  }\n\n");

    fprintf(lilyfile, "  \\midi {\n");
    fprintf(lilyfile, "    \\context {\n");
    fprintf(lilyfile, "      \\Score\n");
    fprintf(lilyfile, "      tempoWholesPerMinute = #(ly:make-moment %d 4)\n",
	    metronome);
    fprintf(lilyfile, "    }\n");
    fprintf(lilyfile, "  }\n");

    fprintf(lilyfile, "}\n");
}
