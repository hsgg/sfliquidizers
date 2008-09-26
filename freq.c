#include <math.h>
#include <complex.h> /* must be before fftw3.h */

#include "libc.h"
#include "freq.h"
#include "write_array.h"
#include "fpDEBUG.h"

struct tmp_fft {
    int size;
    double *in;

    int freqsize;
    double *afreq;
};


/******** fft *********/

tmp_fft *fft_init(int setsize, int freqsize)
{
    tmp_fft *fft = mymalloc(sizeof(tmp_fft));

    fft->size = setsize;
    fft->freqsize = freqsize;

    fft->in = mymalloc(setsize * sizeof(double));
    fft->afreq = mymalloc(freqsize * sizeof(double));

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
    free(fft);
}



double get_frequency(tmp_fft *fft, double samplerate)
{
    static int n = 0; //FIXME: just for debugging

    int setsize = fft->size;
    double *in = fft->in;
    int freqsize = fft->freqsize;
    double *afreq = fft->afreq;

    double avg, avg2, mass, stddev;
    int i, j, k;

    double const dt = 1.0 / (double)samplerate;
    double const df = 20.0;
    complex double const m_2piI_df_dt = 2 * M_PI * I * df * dt;
    double const m_1_sqrt_2pi_dt = 1.0 / sqrt(2 * M_PI) * dt;


    DBG("Calculating frequency...\n");

    /* calculate amplitude of each frequency */
    for (i = 0; i < freqsize; i++) {
	/* perform integral */
	afreq[i] = 0.0;
	for (j = 0; j < setsize; j++) {
	    register complex double const integrand = cexp(m_2piI_df_dt * i * j) * in[j];
	    afreq[i] += fabs(creal(integrand) + cimag(integrand));
	}
	afreq[i] *= m_1_sqrt_2pi_dt;
    }

    /*FIXME: only for debuggin */
    if (n++ == 1)
	write_to_file("fft.dat", freqsize, afreq, df, 0);


    /* average intensity, stddev */
    avg = 0.0;
    avg2 = 0.0;
    for (i = 0; i < freqsize; i++) {
	avg += afreq[i];
	avg2 += afreq[i] * afreq[i];
    }
    avg /= freqsize;
    avg2 /= freqsize;
    stddev = sqrt(avg2 - avg * avg);
    DBG("Average intensity: %lf\n", avg);
    DBG("Stddev: %lf\n", stddev);


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
    j = i;
    while (afreq[j] > 2.0 * stddev)
	j--;
    k = i;
    while (afreq[k] > 2.0 * stddev)
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
    DBG("Weighted average around first maximum: %lf +- %lf (%lf)\n", avg, stddev, mass);


    return avg;
}
