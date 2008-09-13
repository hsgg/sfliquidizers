#ifndef TUNE_H
#define TUNE_H


typedef struct {
    double min;
    double avg;
    double max;
    char *note;
} Mapping;

typedef struct {
    int size;
    Mapping *m;
} MappingArray;


MappingArray fns_tune();
MappingArray dur_tune();


#endif
