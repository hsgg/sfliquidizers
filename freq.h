#ifndef FREQ_H
#define FREQ_H

typedef struct tmp_fft tmp_fft;


tmp_fft *fft_init(int setsize, int freqsize);
void fft_destroy(tmp_fft *fft);
double *fft_inptr(tmp_fft *fft);
double get_frequency(tmp_fft *fft, double samplerate);

#endif
