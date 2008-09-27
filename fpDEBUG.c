#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "fpDEBUG.h"


#ifdef DEBUG

static char * debuggingname_ = NULL;

/*************** debugginglevel_ ************/

int debugginglevel_(int const diff)
{
  static __thread int level = 0;
  if ((level > 99) || (level < -99))
    fprintf(stderr, "%s: WARNING: Runaway debugging level %d?\n",
        debuggingname_, level);
  return (level += diff);
}



/*********** debuggingname_ **********/


/* free allocated memory */
static void debuggingname_free_()
{
  free(debuggingname_);
}

/* Copy the name to be displayed infront of every message */
char * debuggingname_set_(char const * const name)
{
  char *newname = NULL;

  if (!name)
    return debuggingname_;

  /* free allocated memory at exit */
  if (debuggingname_ == NULL)
  {
    if(atexit(debuggingname_free_))
    {
      fprintf(stderr, "fpDEBUG.c: ERROR: '%s' cannot register for atexit "
          "deallocation of memory!\n", name);
      exit(-1);
    }
  }

  /* allocate memory, copy string */
  newname = realloc(debuggingname_, (strlen(name) + 1) * sizeof(char));
  if (!newname)
  {
    fprintf(stderr, "fpDEBUG.c: ERROR: Cannot allocate memory for %s!\n", name);
    exit(-1);
  }
  else
  {
    DBG("Set program name from %s to %s, DEBUG level is %d.\n", debuggingname_,
	name, DEBUG);
    debuggingname_ = newname;
    strcpy(debuggingname_, name);
  }

  return debuggingname_;
}


/* get or set how much room is reserved for the programname */
static int debuggingnamelen_(int const len)
{
  static int length = 15;

  if (len < 0)
    return length;
  else
    return (length = len);
}


/* calculate indentation */
int debuggingindent_()
{
  if (debuggingname_)
    return (debuggingnamelen_(-1) > strlen(debuggingname_)
        ? debuggingnamelen_(-1) - strlen(debuggingname_)
        : 0)
      + 4 * debugginglevel_(0)
      + strlen(" ");
  else
    return 4 * debugginglevel_(0);
}


/* Plan 1: Use commenting.
 * Plan 2: Use simple macro.
 * Plan 3: Use macro with debugging level and indentation as parameters.
 * Plan 4: Use macro with dynamic debugging level and indentation;
 * Plan 5: Use macro with dynamic debugging level, indentation, and program
 *      name in static variables.
 * Plan 6: Use function with variadic arguments.
 */
/* print out a formatted debugging message, if 'level' is high enough */
int debugging_msg_(char const * const msg, ...)
{
  int retval;
  va_list ag, ap;

  if (debugginglevel_(0) > DEBUG)
    return 0;

  va_start(ag, msg);
  va_copy(ap, ag);

  if (va_arg(ap, char *))
    retval = vfprintf(stdout, msg, ag);
  else
    retval = vfprintf(stdout, msg + strlen("%s:"), ap);

  va_end(ap);
  va_end(ag);
  return retval;
}


#endif
