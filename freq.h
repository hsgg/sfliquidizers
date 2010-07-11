#ifndef FREQ_H
#define FREQ_H

typedef struct fft_cache fft_cache;


fft_cache *fft_init(int setsize, int freqsize);
void fft_destroy(fft_cache *fft);
double *fft_inptr(fft_cache *fft);
double get_frequency(fft_cache *fft, double samplerate);

#endif
