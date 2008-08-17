#ifndef FP_DEBUG_H
#define FP_DEBUG_H


#define DEBUG 0



# ifndef DEBUG
/* set all debugging stuff to absolutely nothing */

#  define SETPROGDBG(name)
#  define GETPROGDBG

#  define INCDBG
#  define DECDBG
#  define GETDBG
#  define SETDBG(level)

#  define DBG(msg, x...)
#  define DBGEMPTYLINE



# else
/* set all debugging stuff on */


#  include <stdio.h>
#  include <string.h>

#ifndef __GNUC__
#  define __attribute__(x) /* nothing, or can we find a subtitute? (TODO) */
#  define __thread /* nothing, or can we substitute? (TODO) */
#endif

int debugginglevel_(int const diff);

char * debuggingname_set_(char const * const name);
int debuggingindent_();

int debugging_msg_(char const * const msg, ...)
    __attribute__((format(printf,1,2)));


/* Use these macros to set the name of the program. */
#  define SETPROGDBG(name) debuggingname_set_(name)
#  define GETPROGDBG debuggingname_set_(NULL)

/* Use these macros to set the debugging level. */
#  define INCDBG debugginglevel_(1)
#  define DECDBG debugginglevel_(-1)
#  define GETDBG debugginglevel_(0)
#  define SETDBG(level) debugginglevel_((level) - GETDBG)

/* Use this macro as a printf statement. */
#  define DBG(msg, x...) debugging_msg_("%s:%*s" msg, \
        GETPROGDBG, debuggingindent_(), "", ## x)

/* Use this to print an empty line. */
#  define DBGEMPTYLINE DBG("\n");


# endif


#endif
