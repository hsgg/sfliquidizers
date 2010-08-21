#include <stdio.h>
#include <string.h>
#include <err.h>

#include "libc.h"


/* mycalloc(), malloc(), myrealloc() that fail fatally: */
void *mycalloc(size_t nmemb, size_t size)
{
    void *result = calloc(nmemb, size);
    if (!result)
        err(-2, "mycalloc() failed!");
    return result;
}
void *mymalloc(size_t size)
{
    void *result = malloc(size);
    if (!result)
        err(-2, "mymalloc() failed!");
    return result;
}
void *myrealloc(void *ptr, size_t size)
{
    void *result = realloc(ptr, size);
    if (!result)
        err(-2, "myrealloc() failed!");
    return result;
}


/* realloc_strcpy() convenience function */
char *realloc_strcpy(char **dest, char *src)
{
    char *new = (dest ? *dest : NULL);
    new = myrealloc(new, (strlen(src) + 1) * sizeof(char));
    if (dest)
        *dest = new;
    return strcpy(new, src);
}

/* realloc_strcat() convenience function */
char *realloc_strcat(char **dest, char *src)
{
    if (dest == NULL || *dest == NULL)
        return realloc_strcpy(dest, src);
    else {
        *dest = myrealloc(*dest, (strlen(*dest) + strlen(src) + 1) * sizeof(char));
        return strcat(*dest, src);
    }
}

/* Reallocate the destination string and print the string given by the format
 * string and additional arguments.
 * 'destp' must be NULL or previously mallocated. */
char *print2string(char *dest, char *fmt, ...)
{
    int totsize;
    va_list va;
    char *newdest = NULL;

    /* get length of resulting string */
    va_start(va, fmt);
    totsize = vsnprintf(newdest, 0, fmt, va);
    va_end(va);

    /* create string */
    va_start(va, fmt);
    newdest = mymalloc((totsize + 1) * sizeof(char));
    if (vsnprintf(newdest, totsize + 1, fmt, va) < 0) {
        fprintf(stderr, "Very bad error occurred in print2string().\n"
                "Couldn't print the following to a string:\n");
        vfprintf(stderr, fmt, va);
        exit(-3);
    }
    va_end(va);

    free(dest);
    return newdest;
}


/*
 * mygetline():
 *      Do the same as getline(3), except strip the trailing newline
 *      character.
 */
ssize_t mygetline(char **lineptr, size_t *n, FILE *stream)
{
    ssize_t k = getline(lineptr, n, stream);

    if (k < 1)
        return k;

    if ((*lineptr)[k - 1] == '\n')
        (*lineptr)[--k] = '\0';

    return k;
}
