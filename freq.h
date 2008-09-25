#ifndef FREQ_H
#define FREQ_H

#include <complex.h> /* must be before fftw3.h */
#include <fftw3.h>

typedef struct tmp_fft {
    int size;
    double *in;

    fftw_complex *data;
    fftw_plan plan;

    int freqsize;
    double *afreq;
} tmp_fft;


tmp_fft fft_init(int setsize, int freqsize);
void fft_destroy(tmp_fft fft);
double get_frequency(tmp_fft fft, double samplerate);

#endif
