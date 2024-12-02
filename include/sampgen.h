#ifndef SAMPLEN_H_INCLUDED
#define SAMPGEN_H_INCLUDED
#endif // SAMPGEN_H_INCLUDED
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define TWOPI (6.283185307)
#define tablen (1048576) /* length of a gtable */
#define blocklen (16) /* block size for BREAKPOINTs */
#define maxcols (8)
#define minfreq (10.0)
#define maxfreq (10000.0)
#define minamp (0.0)
#define maxamp (1.0)
#define minenv (0.125) /* minimum duration of an envelope */
#define defaultfreq (440.0)
#define defaultamp (1-1.0e-4)
#define harmonics (24)

typedef enum {NONE, SLUR, GLISS, ALLPROPS} note_type; // Define note properties

typedef struct {
  double time;
  double **freq;
  double *amp;
  note_type *props;
} BREAKPOINT;

typedef enum {FREQ_BRK, AMP_BRK, ALL_BRK, FREQ_EXP, AMP_EXP, ALL_EXP} brk_type; // breakpoint file types

typedef struct {
  int cols;
  int tcols; // Indicates number of cols where frequencies are tuned with bps_inittune
  brk_type ftype;
  BREAKPOINT *table;
} BREAKPOINTS;

/* Use the gtable structure to store wavetables but we will restructure the functions to
** not use both oscilt and brkstream in functions.
*/
typedef struct t_gtable {
  double *table;        /* ptr to array containing the waveform */
  unsigned long length; /* excluding guard point */
} GTABLE;

typedef struct t_oscil {
  double setfreq;
  double setphase;
  double incr;
} OSCIL;

typedef struct t_toscil {
  OSCIL osc;
  GTABLE *gtable;
  double dtablen;
  double sizeovrsr;
} OSCILT;

typedef struct t_env {
  unsigned long *time, pos;
  double *amp;
  double *fac;
  double val;
  int section;
  int start;
  int morepts;
} ENV;

typedef GTABLE* (*waveformfunc)(unsigned long len, unsigned long nharms);
typedef ENV* (*ampfunc)(double len, unsigned long srate);
typedef enum {HARSH, SUSTAINED} env_type;
typedef enum {SINE, TRIANGLE, SQUARE, UPSAW, DOWNSAW} wav_type;

typedef struct {
  BREAKPOINTS *points;
  BREAKPOINT left, right;
  OSCILT **waves; /* samples produced for given waveform/harmonics no. can be from multiple wavetables! */
  ENV **envelopes;
  double pos;
  unsigned long npoints, ileft, iright,srate;
  double incr, width;
  int morepts, *tuned; /* tuned says whetherglissando changes frequency with linear interpolation not from discrete set of notes, morepts=current breakpoint isn't last one*/
} BRKSTREAM;

typedef enum {UNTUNED, NTET, KIRN3, JAVAS, JAVAP} tsys_type; // Select tuning system from tfuncs

/* T_PROPS organises information for a tuning system to be combined with tfuncs when creating a breakstream */
typedef struct t_props {
  tsys_type tuning;
  int scale; // Num. notes in a scale
  int root; // Positive note number to locate the tuning frequency in total range of notes of tuning system(eg middle C=60 in 12-tet MIDI numbers 0-127)
  double frequency; // Corresponding frequency in Hz to root note number above
  double *ratios; // Array of ratios to determine scales - specifically array of 6 numbers for Javanese slendro, 8 for Javanese pelog scales including octave
} T_PROPS;

typedef double (*tuningfunc)(T_PROPS *props, int newcols);

/* BRK_PROPS organises info for bps_newstream */
typedef struct brk_props {
  unsigned long srate;
  int cols;
  brk_type ftype;
} BRK_PROPS;

enum {
  ERR_NONE,
  ERR_MEMORY, // No memory
  ERR_ARGS, // Failed to get stream: bad srate, ftype, cols etc.
  ERR_WAV, // Invalid waveformfuncs
  ERR_ENV, // Invalid ampfuncs
  ERR_T_PROPS, // Invalid t_props(tuning system info)
  ERR_BRKPT_FORM, // Invalid breakpoint file formatting
  ERR_BRKPT_TIME, // Invalid breakpoint timestamps
  ERR_BRKPT_AMP, // Invalid breakpoint amps
  ERR_BRKPT_FREQ, // Invalid frequencies
  ERR_BRKPT_PROPS // Invalid note props
};

GTABLE *new_sine(unsigned long len, unsigned long nharms);
GTABLE *new_triangle(unsigned long len, unsigned long nharms);
GTABLE *new_square(unsigned long len, unsigned long nharms);
GTABLE *new_upsaw(unsigned long len, unsigned long nharms);
GTABLE *new_downsaw(unsigned long len, unsigned long nharms);
OSCILT *new_oscilt(unsigned long srate, GTABLE *gtable, double phase);
double tabtick(OSCILT *p_osc, double freq);

ENV *env_harsh(double len, unsigned long srate);
ENV *env_sustained(double len, unsigned long srate);
double env_tick(ENV *env);
void env_reset(ENV *env, double newlen, unsigned long srate);

double ntet_tune(T_PROPS *props, int n);
double kirn3_tune(T_PROPS *props, int n);
double javas_tune(T_PROPS *props, int n);
double javap_tune(T_PROPS *props, int n);

BRKSTREAM *bps_newstream(FILE *fp, env_type *envs, wav_type *waves, BRK_PROPS *props, unsigned long *psize, int *err);
int bps_inittune(BRKSTREAM *stream, T_PROPS *props, int newcols);
void bps_free(BRKSTREAM *stream);
double bps_freq_tick(BRKSTREAM *stream);
double bps_amp_tick(BRKSTREAM *stream);
double bps_all_tick(BRKSTREAM *stream);
double bps_freqexp_tick(BRKSTREAM *stream);
double bps_ampexp_tick(BRKSTREAM *stream);
double bps_allexp_tick(BRKSTREAM *stream);
