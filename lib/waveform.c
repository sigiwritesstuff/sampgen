#include "sampgen.h"

// contains the functions for wavetables and generating samples of different frequencies and waveforms from them
GTABLE *new_gtable(unsigned long len) {
  unsigned long i;
  GTABLE *gtable=NULL;

  if(len==0)
    return NULL;
  gtable=(GTABLE*) malloc(sizeof(GTABLE));
  if(gtable==NULL)
    return NULL;
  gtable->table=(double*) malloc((len+1)*sizeof(double));
  if(gtable->table==NULL) {
    free(gtable);
    return NULL;
  }
  /* i<=len as the condition because we want to initialise the guard point as well.*/
  for(i=0;i<=len;i++)
    gtable->table[i]=0.0;
  gtable->length=len;
  return gtable;
}

void gtable_norm(GTABLE *gtable) {
  unsigned long i;
  double max=0.0;
  /* Normalise table using maxamp: */
  for(i=0;i<gtable->length;i++) {
    if(gtable->table[i]>=max)
      max=gtable->table[i];
  }
  max=1.0/max;
  for(i=0;i<=gtable->length;i++) { gtable->table[i]*=max;}
}

OSCILT *new_oscilt(unsigned long srate, GTABLE *gtable, double phase) {
  OSCILT *p_osc;
  /* Error check gtable: */
  if(gtable==NULL)
    return NULL;
  p_osc=(OSCILT *)malloc(sizeof(OSCILT));
  if(p_osc==NULL)
    return NULL;
  /* Initiate oscillator: */
  p_osc->osc.setfreq=0.0;
  p_osc->osc.setphase=gtable->length*phase;
  p_osc->osc.incr=0.0;
  /* Set gtable specific things: */
  p_osc->gtable=gtable;
  p_osc->dtablen=(double) gtable->length;
  p_osc->sizeovrsr=p_osc->dtablen / (double) srate;
  return p_osc;
}

GTABLE *new_sine(unsigned long len, unsigned long nharms) {
  unsigned long i;
  double step;
  GTABLE *gtable=new_gtable(len);

  if(gtable==NULL)
    return NULL;
  step=TWOPI/len;
  for(i=0;i<len;i++) {gtable->table[i]=sin(step * i);}
  gtable->table[i]=gtable->table[0];
  return gtable;
}

GTABLE *new_triangle(unsigned long len, unsigned long nharms) {
  unsigned long i, j, h=1;
  double step, amp;
  GTABLE *gtable=new_gtable(len);

  /* Check arguments, nharms below Nyquist frequency, allocate memory: */
  if(gtable==NULL)
    return NULL;
  if(nharms==0||(2*nharms-1)>=len/2)
    return NULL;
  step=TWOPI/len;
  /* Nested for loops for the harmonics-h is current harmonic, for each harmonic increment table entries by cos values: */
  for(i=0;i<nharms;i++) {
    amp=1.0/(h*h);
    for(j=0;j<len;j++) {gtable->table[j]+=amp*cos(step * j * h);}
    h+=2;
  }
  gtable->table[len]=gtable->table[0];
  /* Normalise */
  gtable_norm(gtable);
  return gtable;
}

GTABLE *new_square(unsigned long len, unsigned long nharms) {
  unsigned long i, j, h=1;
  double step, amp;
  GTABLE *gtable=new_gtable(len);

  /* Check arguments, nharms below Nyquist frequency, allocate memory: */
  if(gtable==NULL)
    return NULL;
  if(nharms==0||(2*nharms-1)>=len/2)
    return NULL;
  step=TWOPI/len;
  /* Nested for loops for the harmonics-h is current harmonic, for each harmonic increment table entries by cos values: */
  for(i=0;i<nharms;i++) {
    amp=1.0/h;
    for(j=0;j<len;j++) {gtable->table[j]+=amp*sin(step * j * h);}
    h+=2;
  }
  gtable->table[len]=gtable->table[0];
  /* Normalise */
  gtable_norm(gtable);
  return gtable;
}

GTABLE *new_downsaw(unsigned long len, unsigned long nharms) {
  unsigned long i, j, h=1;
  double step, amp;
  GTABLE *gtable=new_gtable(len);

  /* Check arguments, nharms below Nyquist frequency, allocate memory: */
  if(gtable==NULL)
    return NULL;
  if(nharms==0||nharms>=len/2)
    return NULL;
  step=TWOPI/len;
  /* Nested for loops for the harmonics-h is current harmonic, for each harmonic increment table entries by cos values: */
  for(i=0;i<nharms;i++) {
    amp=1.0/h;
    for(j=0;j<len;j++) {gtable->table[j]+=amp*sin(step * j * h);}
    h++;
  }
  gtable->table[len]=gtable->table[0];
  /* Normalise */
  gtable_norm(gtable);
  return gtable;
}

GTABLE *new_upsaw(unsigned long len, unsigned long nharms) {
  unsigned long i;
  GTABLE *gtable=new_downsaw(len, nharms);

  if(gtable==NULL)
    return NULL;
  for(i=0;i<=gtable->length;i++) {
    gtable->table[i]*=-1;
  }
  return gtable;
}

double tabtick(OSCILT *p_osc, double freq) {
  int index=(int) p_osc->osc.setphase;
  double dtablen=p_osc->dtablen, curphase=p_osc->osc.setphase;
  double *table=p_osc->gtable->table;
  if(p_osc->osc.setfreq!=freq) {
    p_osc->osc.setfreq=freq;
    p_osc->osc.incr=p_osc->osc.setfreq * p_osc->sizeovrsr;
  }
  curphase+=p_osc->osc.incr;
  while(curphase>=dtablen) {curphase-=dtablen;}
  while(curphase<0.0) {curphase+=dtablen;}
  p_osc->osc.setphase=curphase;
  return table[index];
}
