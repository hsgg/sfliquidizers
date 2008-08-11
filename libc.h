#include <stdlib.h>

/* mycalloc(), malloc(), myrealloc() that fail fatally: */
void *mycalloc(size_t nmemb, size_t size);
void *mymalloc(size_t size);
void *myrealloc(void *ptr, size_t size);

/* realloc_strcpy() convenience function */
char *realloc_strcpy(char **dest, char *src);
char *realloc_strcat(char **dest, char *src);
