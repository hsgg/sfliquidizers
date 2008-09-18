#include <stdio.h>
#include <string.h>

#include "libc.h"


/* mycalloc(), malloc(), myrealloc() that fail fatally: */
void *mycalloc(size_t nmemb, size_t size)
{
    void *result = calloc(nmemb, size);
    if (!result) {
        printf("mycalloc() failed!\n");
        exit(-2);
    }
    return result;
}
void *mymalloc(size_t size)
{
    void *result = malloc(size);
    if (!result) {
        printf("mymalloc() failed!\n");
        exit(-2);
    }
    return result;
}
void *myrealloc(void *ptr, size_t size)
{
    void *result = realloc(ptr, size);
    if (!result) {
        printf("myrealloc() failed!\n");
        exit(-2);
    }
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
