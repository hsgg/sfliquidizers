#include <math.h>
#include <complex.h>

#include "libc.h"
#include "freq.h"
#include "write_array.h"
#include "fpDEBUG.h"

struct tmp_fft {
    int size;
    double *in;

    complex double *cfreq;

    int freqsize;
    double *afreq;
};


/***** create, destroy *****/

tmp_fft *fft_init(int setsize, int freqsize)
{
    tmp_fft *fft = mymalloc(sizeof(tmp_fft));

    fft->size = setsize;
    fft->freqsize = freqsize;

    fft->in = mymalloc(setsize * sizeof(double));
    fft->cfreq = mymalloc(freqsize * sizeof(complex double));
    fft->afreq = mymalloc(freqsize * sizeof(double));

    return fft;
}

void fft_destroy(tmp_fft *fft)
{
    /* free */
    free(fft->in);
    free(fft->cfreq);
    free(fft->afreq);
    free(fft);
}


/***** get elements *****/

double *fft_inptr(tmp_fft *fft)
{
    return fft->in;
}

static int get_fftsize(tmp_fft *fft)
{
    return fft->freqsize;
}


static double *get_fft_intg(tmp_fft *fft, double const df, double const dt)
{
    INCDBG;
    int setsize = fft->size;
    double *in = fft->in;
    complex double *cfreq = fft->cfreq;
    int freqsize = fft->freqsize;
    double *afreq = fft->afreq;

    int i, j;

    complex double const m_2piI_df_dt = 2 * M_PI * I * df * dt;
    complex double const m_1_sqrt_2pi_dt = 1.0 / sqrt(2 * M_PI) * dt;


    /* calculate amplitude of each frequency */
    for (i = 0; i < freqsize; i++) {
	/* perform integral */
	cfreq[i] = 0.0;
	for (j = 0; j < setsize; j++) {
	    cfreq[i] += cexp(m_2piI_df_dt * i * j) * in[j];
	}
    }

    /* convert to real freqs */
    for (i = 0; i < freqsize; i++)
	afreq[i] = cabs(cfreq[i] * m_1_sqrt_2pi_dt);


    DECDBG;
    return afreq;
}


/****** calculate frequency *****/

double get_frequency(tmp_fft *fft, double samplerate)
{
    INCDBG;

    double avg, avg2, mass, stddev;
    int i, j, k;

    double const dt = 1.0 / (double)samplerate;
    double const df = 5.0;
    int const freqsize = get_fftsize(fft);
    double * const afreq = get_fft_intg(fft, df, dt);

#   ifdef DEBUG
    static int n = 0;
    if (++n == 1)
	write_to_file("fft.dat", freqsize, afreq, df, 0);
#   endif


    /* average intensity, stddev */
    INCDBG;
    avg = 0.0;
    avg2 = 0.0;
    for (i = 0; i < freqsize; i++) {
	avg += afreq[i];
	avg2 += afreq[i] * afreq[i];
    }
    avg /= freqsize;
    avg2 /= freqsize;
    stddev = sqrt(avg2 - avg * avg);
    DBG("Average intensity: %lf +- %lf\n", avg, stddev);
    DECDBG;


    /* first maximum above 2 * stddev */
    mass = 0.0;
    i = 0;
    while (i < freqsize) {
	if (afreq[i] > mass)
	    mass = afreq[i];
	else if (mass > 2.0 * stddev) {
	    i--;
	    break;
	}
	i++;
    }

    /* average around first maximum */
    j = (i < freqsize) ? i : freqsize - 1;
    while ((j >= 0) && (afreq[j] > 2.0 * stddev))
	j--;
    k = (i < freqsize) ? i : freqsize - 1;
    while ((k < freqsize) && (afreq[k] > 2.0 * stddev))
	k++;
    avg = 0.0;
    avg2 = 0.0;
    mass = 0.0;
    INCDBG;
    for (i = j + 1; i < k; i++) {
	DBG("Using in average: %lf (%lf)\n", i * df, afreq[i]);
	avg += afreq[i] * i;
	avg2 += afreq[i] * i * i;
	mass += afreq[i];
    }
    DECDBG;
    avg /= mass;
    avg2 /= mass;
    avg *= df;
    avg2 *= df * df;
    stddev = sqrt(avg2 - avg * avg);
    DBG("%d: Weighted average around first maximum: %.2lf +- %.2lf (%lf)\n", n, avg, stddev, mass);

    DECDBG;
    return avg;
}
