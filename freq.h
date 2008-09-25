#ifndef FREQ_H
#define FREQ_H

typedef struct tmp_fft tmp_fft;


tmp_fft *fft_init(int setsize, int freqsize);
double *fft_inptr(tmp_fft *fft);
void fft_destroy(tmp_fft *fft);
double get_frequency(tmp_fft *fft, double samplerate);

#endif
