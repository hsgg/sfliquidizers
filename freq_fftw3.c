#include <math.h>
#include <complex.h> /* must be before fftw3.h */
#include <fftw3.h>

#include "libc.h"
#include "freq.h"
#include "write_array.h"
#include "fpDEBUG.h"

struct tmp_fft {
    int size;
    double *in;

    fftw_complex *data;
    fftw_plan plan;

    int freqsize;
    double *afreq;
};


/******** fft *********/

tmp_fft *fft_init(int setsize, int freqsize)
{
    INCDBG;

    tmp_fft *fft = mymalloc(sizeof(tmp_fft));

    fft->size = setsize;
    fft->freqsize = freqsize;

    fft->in = mymalloc(setsize * sizeof(double));
    fft->afreq = mymalloc(freqsize * sizeof(double));

    /* create data, 'couse that's what's a plan's all about */
    DBG("Creating a plan...\n");
    fft->data = fftw_malloc((fft->size/2 + 1) * sizeof(fftw_complex));
    if (!fft->data) {
	printf("Error: Could not allocate 'fft->data'.\n");
	exit(-1);
    }
    fft->plan = fftw_plan_dft_r2c_1d(fft->size, fft->in, fft->data, FFTW_ESTIMATE); //TODO use FFTW_MEASURE

    DECDBG;
    return fft;
}

double *fft_inptr(tmp_fft *fft)
{
    return fft->in;
}

void fft_destroy(tmp_fft *fft)
{
    /* free */
    free(fft->in);
    free(fft->afreq);
    fftw_destroy_plan(fft->plan);
    fftw_free(fft->data);
    fftw_cleanup();
    free(fft);
}



double *get_fft_fftw3(tmp_fft *fft, double samplerate)
{
    INCDBG;
    int setsize = fft->size;
    int freqsize = fft->freqsize;
    double *afreq = fft->afreq;

    double avg, avg2, mass, stddev;
    int i, j, k;


    /* fft */
    DBG("Executing the plan...\n");
    fftw_execute(fft->plan);

    /* convert to real freqs */
    for (i = 0; (i < freqsize) && (i < fft->size/2 + 1); i++)
	afreq[i] = cabs(fft->data[i]) / sqrt(fft->size);
    while (i < freqsize)
	afreq[i++] = 0.0;

    /*FIXME: only for debuggin */
    static int n = 0;
    if (n++ == 0)
	write_to_file("fft.dat", freqsize, afreq, samplerate / setsize, 0);


    DECDBG;
    return avg;
}
