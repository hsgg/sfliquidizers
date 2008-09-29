#ifndef TUNE_H
#define TUNE_H


typedef struct {
    int min;
    double avg;
    int max;
    char *note;
} Mapping;

typedef struct {
    int size;
    Mapping *m;
} MappingArray;


MappingArray fns_tune();
MappingArray dur_tune();
MappingArray dur_tune_metronome(double const unit, double quarter);

char *get_str(MappingArray *map, double value);
int get_maximalmax(MappingArray *map);


#endif
