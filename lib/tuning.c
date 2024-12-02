/*  Source file to define tuning systems to read notes with
*** Each tuning system needs a number of notes per scale and
*** note number analogous to the MIDI number for a system with
*** the same frequency range but given size of scale.
*** Functions are called during reading from breakpoint file
*** so that glissandos are stored as a string of frequencies
*** ending with a zero in a double** array.
*/

#include "sampgen.h"
double middle_c=220*pow(2, 0.25);
double ntet_tune(T_PROPS *props, int n) {
  /* Provide size of scale plus note number analogous to MIDI */
  double midnote=5.0*props->scale;
  double npower=(n-midnote)/props->scale;
  double val=middle_c*pow(2, npower);
  return val;
}

double kratios[12]={0.5,0.52734375,0.5625,16.0/27,0.625,2.0/3,0.703125,0.75,64/81.0,0.84375,8.0/9,0.9375};
double comma=pow((double) 80/81, 0.25);

double kirn3_tune(T_PROPS *props, int n) {
  double val=props->frequency;n-=60;
  while(n>11) {
    val*=2;
    n-=12;
  }
  while(n<0) {
    val*=0.5;
    n+=12;
  }
  if(n==2) val*=comma*comma;
  else if(n==7) val*=comma;
  else if(n==9) val*=comma*comma*comma;
  return val*kratios[n];
}

double javas_tune(T_PROPS *props, int n) { /* Slendro scale */
  double val=props->frequency;
  int interval=n-props->root;
  while(interval>4) {
    val*=props->ratios[5];
    interval-=5;
  }
  while(interval<0) {
    val/=props->ratios[5];
    interval+=5;
  }
  return val*props->ratios[interval];
}

double javap_tune(T_PROPS *props, int n) { /* Pelog scale */
  double val=props->frequency;
  int interval=n-props->root;
  while(interval>6) {
    val*=props->ratios[7];
    interval-=7;
  }
  while(interval<0) {
    val/=props->ratios[7];
    interval+=7;
  }
  return val*props->ratios[interval];
}
