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
# define fftw_free(p) free(p)
# define fftw_cleanup() /* nothing */
#endif

#include "libc.h"
#include "freq.h"
#include "write_array.h"
#include "fpDEBUG.h"


struct fft_cache {
    int insize;
    double *in;

    int fftsize;
    fftw_plan plan;

    fftw_complex *cfreq;

    int freqsize;
    double *afreq;
    double *pfreq;
};

static void *my_aligned_malloc(size_t const nbytes)
{
    void *ptr;
    ptr = fftw_malloc(nbytes);
    if (!ptr) {
	printf("Error: Could not allocate %zu bytes of aligned memory.\n",
		nbytes);
	exit(-1);
    }
    return ptr;
}


/***** create, destroy *****/

fft_cache *fft_init(int insize, int freqsize)
{
    INCDBG;

    fft_cache *fft = mymalloc(sizeof(*fft));

    fft->insize = insize;
    fft->fftsize = insize / 2 + 1;
    fft->freqsize = freqsize;
    int cfreqsize = (freqsize > fft->fftsize ? freqsize : fft->fftsize);

    fft->in = my_aligned_malloc(insize * sizeof(double));
    fft->cfreq = my_aligned_malloc(cfreqsize * sizeof(fftw_complex));
    fft->afreq = my_aligned_malloc(freqsize * sizeof(double));
    fft->pfreq = my_aligned_malloc(freqsize * sizeof(double));

    /* create data, 'couse that's what's a plan's all about */
    DBG("Creating a plan...\n");
    fft->plan = fftw_plan_dft_r2c_1d(fft->insize, fft->in, fft->cfreq,
	    FFTW_ESTIMATE); //TODO use FFTW_MEASURE

    DECDBG;
    return fft;
}

void fft_destroy(fft_cache *fft)
{
    /* free */
    fftw_free(fft->in);
    fftw_free(fft->cfreq);
    fftw_free(fft->afreq);
    fftw_free(fft->pfreq);
    fftw_destroy_plan(fft->plan);
    fftw_cleanup();
    free(fft);
}


/***** get elements *****/

double *fft_inptr(fft_cache *fft)
{
    return fft->in;
}


#ifdef USE_FFTW3
static void get_fft_fftw3(fft_cache *fft)
{
    INCDBG;
    int insize = fft->insize;
    int fftsize = fft->fftsize;
    int freqsize = fft->freqsize;
    int i;
    double const sqrt_insize = sqrt(insize);


    /* fft */
    DBG("Executing the plan...\n");
    fftw_execute(fft->plan);

    /* scale */
    for (i = 0; (i < freqsize) && (i < fftsize); i++)
	fft->cfreq[i] = fft->cfreq[i] / sqrt_insize;
    while(i < freqsize)
	fft->cfreq[i++] = 0.0;

    DECDBG;
}
#else
static void get_fft_intg(fft_cache *fft, double const df, double const dt)
{
    INCDBG;
    int insize = fft->insize;
    double *in = fft->in;
    complex double *cfreq = fft->cfreq;
    int freqsize = fft->freqsize;

    int i, j;

    complex double const m_2piI_df_dt = 2 * M_PI * I * df * dt;
    complex double const m_1_sqrt_2pi_dt = dt;


    /* calculate amplitude of each frequency */
    for (i = 0; i < freqsize; i++) {
	cfreq[i] = 0.0;
	for (j = 0; j < insize; j++) {
	    cfreq[i] += cexp(m_2piI_df_dt * i * j) * in[j];
	}
	cfreq[i] *= m_1_sqrt_2pi_dt;
    }

    DECDBG;
}
#endif


/****** calculate frequency *****/

double get_frequency(fft_cache *fft, double samplerate)
{
    INCDBG;

    double const sigma = 2.0;

    double avg, avg2, mass, stddev, freqstddev;
    int i, j, k;

    double const dt = 1.0 / (double)samplerate;


#   ifdef USE_FFTW3
    double const df = samplerate / (double)fft->insize;
    get_fft_fftw3(fft);
#   else
    //double const df = 10.0;
    double const df = samplerate / (double)fft->insize;
    get_fft_intg(fft, df, dt);
#   endif

    int const freqsize = fft->freqsize;
    fftw_complex * const cfreq = fft->cfreq;
    double * const afreq = fft->afreq;
    double * const pfreq = fft->pfreq;

    /* convert to real freqs and real phase shifts */
    for (i = 0; i < freqsize; i++) {
	afreq[i] = cabs(cfreq[i]);
	pfreq[i] = asin(cimag(cfreq[i]) / afreq[i]);
    }


#   ifdef DEBUG
    static int n = 0;
    n++;
    char *filename = print2string(NULL, "fft/fft%4.4d", n);
    char *phasename = print2string(NULL, "fft/phase%4.4d", n);
    if (access("fft", W_OK) != 0)
	mkdir("fft", 0777);
    write_to_file(filename, freqsize, afreq, df, 0);
    write_to_file(phasename, freqsize, pfreq, df, 0);
    free(filename);
    free(phasename);
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
	    n, n * dt * fft->insize, avg, freqstddev, mass, stddev);

    DECDBG;
    return avg;
}
