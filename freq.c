#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <complex.h> /* must be before fftw3.h */

#ifdef USE_FFTW3
# include <fftw3.h>
#else
  /* define dummies */
  typedef complex double fftw_complex;
  typedef int fftw_plan;
# define fftw_malloc malloc
# define fftw_plan_dft_r2c_1d(a, b, c, d) 0
# define fftw_destroy_plan(a) /* nothing */
# define fftw_free(a) /* nothing */
# define fftw_cleanup() /* nothing */
#endif

#include "libc.h"
#include "freq.h"
#include "write_array.h"
#include "fpDEBUG.h"


struct tmp_fft {
    int size;
    double *in;

    complex double *cfreq;
    fftw_complex *data;
    fftw_plan plan;

    int freqsize;
    double *afreq;
};


/***** create, destroy *****/

tmp_fft *fft_init(int setsize, int freqsize)
{
    INCDBG;

    tmp_fft *fft = mymalloc(sizeof(tmp_fft));

    fft->size = setsize;
    fft->freqsize = freqsize;

    fft->in = mymalloc(setsize * sizeof(double));
    fft->cfreq = mymalloc(freqsize * sizeof(complex double));
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






void fft_destroy(tmp_fft *fft)
{
    /* free */
    free(fft->in);
    free(fft->cfreq);
    free(fft->afreq);
    fftw_destroy_plan(fft->plan);
    fftw_free(fft->data);
    fftw_cleanup();
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


#ifdef USE_FFTW3
static double *get_fft_fftw3(tmp_fft *fft)
{
    INCDBG;
    int setsize = fft->size;
    int freqsize = fft->freqsize;
    double *afreq = fft->afreq;

    int i;


    /* fft */
    DBG("Executing the plan...\n");
    fftw_execute(fft->plan);

    /* convert to real freqs */
    for (i = 0; (i < freqsize) && (i < setsize/2 + 1); i++)
	afreq[i] = cabs(fft->data[i]) / sqrt(setsize);
    while (i < freqsize)
	afreq[i++] = 0.0;

    DECDBG;
    return afreq;
}
#else
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
    complex double const m_1_sqrt_2pi_dt = dt;


    /* calculate amplitude of each frequency */
    for (i = 0; i < freqsize; i++) {
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
#endif


/****** calculate frequency *****/

double get_frequency(tmp_fft *fft, double samplerate)
{
    INCDBG;

    double const sigma = 2.0;

    double avg, avg2, mass, stddev, freqstddev;
    int i, j, k;

    double const dt = 1.0 / (double)samplerate;


#   ifdef USE_FFTW3
    double const df = samplerate / (double)fft->size;
    double * const afreq = get_fft_fftw3(fft);
#   else
    double const df = 10.0;
    double * const afreq = get_fft_intg(fft, df, dt);
#   endif

    int const freqsize = get_fftsize(fft);

#   ifdef DEBUG
    static int n = 0;
    n++;
    char *filename = print2string(NULL, "fft/fft%4.4d", n);
    if (access("fft", W_OK) != 0)
	mkdir("fft", 0777);
    write_to_file(filename, freqsize, afreq, df, 0);
    free(filename);
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
	else if (mass > sigma * stddev) {
	    i--;
	    break;
	}
	i++;
    }

    /* average around first maximum */
    j = (i < freqsize) ? i : freqsize - 1;
    while ((j >= 0) && (afreq[j] > sigma * stddev))
	j--;
    k = (i < freqsize) ? i : freqsize - 1;
    while ((k < freqsize) && (afreq[k] > sigma * stddev))
	k++;
    avg = 0.0;
    avg2 = 0.0;
    mass = 0.0;
    INCDBG;
    for (i = j + 1; i < k; i++) {
	register double const weight = afreq[i];
	DBG("Using in average: %lf (%lf)\n", i * df, afreq[i]);
	avg += weight * i;
	avg2 += weight * i * i;
	mass += weight;
    }
    DECDBG;
    avg /= mass;
    avg2 /= mass;
    avg *= df;
    avg2 *= df * df;
    freqstddev = sqrt(avg2 - avg * avg);
    DBG("%3d (%.2lf sec): freq = %.2lf +- %5.2lf (mass = %.2le, stddev = %.2le)\n",
	    n, n * dt * fft->size, avg, freqstddev, mass, stddev);

    DECDBG;
    return avg;
}
