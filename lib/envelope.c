// Put all amplitude envelope generating functions and objects here:
#include "sampgen.h"

ENV *env_new(unsigned long *time, double *amp, unsigned long srate) {
  int i=0;
  ENV *env=malloc(sizeof(ENV));
  if(env==NULL)
    return NULL;
  env->fac=malloc(sizeof(double)*5);
  if(env->fac==NULL) {
    free(env);
    return NULL;
  }
  for(i=0;i<6;i++) if(time[i]<0) return NULL;
  for(i=0;i<6;i++) if(amp[i]<0.0||amp[i]>1.0) return NULL;
  env->time=time;
  env->amp=amp;
  env->pos=0;
  env->morepts=1;
  i=0;
  while(env->time[i]==0) {
    if(++i==5) {env->morepts=0;break;}
  }
  env->section=i-1;
  env->start=i-1; // So it doesn't have to be found again in env->reset!
  env->val=1.0e-4; //Start with small nonzero value
  //Now initialise fac values:
  env->fac[0]=1.0;
  for(int i=1;i<5;i++) {
    env->fac[i]=pow(amp[i+1]/amp[i], (double) 1.0/(time[i+1]-time[i]));
  }
  return env;
}

ENV *env_harsh(double len, unsigned long srate) {
  unsigned long *time=malloc(sizeof(unsigned long)*6);
  double *amp=malloc(sizeof(double)*6), dtime;
  if(time==NULL ||amp==NULL) return NULL;
  time[0]=0;
  time[1]=0;
  dtime=0.03*srate+0.5;
  time[2]=dtime;
  dtime=0.04*srate+0.5;
  time[3]=dtime;
  dtime=srate*(len-0.1)+0.5;
  time[4]=dtime;
  dtime=srate*len+0.5;
  time[5]=dtime;
  amp[0]=1.0e-4;
  amp[1]=1.0e-4;
  amp[2]=1-1.0e-4;
  amp[3]=0.85;
  amp[4]=0.65;
  amp[5]=1.0e-4;
  return env_new(time, amp, srate);
}

ENV *env_sustained(double len, unsigned long srate) {
  unsigned long *time=malloc(sizeof(unsigned long)*6);
  double *amp=malloc(sizeof(double)*6), dtime;
  if(time==NULL ||amp==NULL) return NULL;
  time[0]=0;
  time[1]=0;
  dtime=0.07*srate+0.5;
  time[2]=dtime;
  dtime=0.1*srate+0.5;
  time[3]=dtime;
  dtime=srate*(len-0.2)+0.5;
  time[4]=dtime;
  dtime=srate*len+0.5;
  time[5]=dtime;
  amp[0]=1.0e-4;
  amp[1]=1.0e-4;
  amp[2]=1-1.0e-4;
  amp[3]=0.9;
  amp[4]=0.6;
  amp[5]=1.0e-4;
  return env_new(time, amp, srate);
}

double env_tick(ENV *env) { /*Any checks for morepts are taken out of the tick function and used in bps*/
  int i=env->section;
  env->val*=env->fac[i];
  env->pos++;
  if(env->pos==env->time[i+1]) {
    env->section++;
    if(env->section==5) env->morepts=0;
  }
  return env->val;
}

void env_reset(ENV *env, double newlen, unsigned long srate) {
  unsigned long tsus=env->time[4], tdelay=env->time[5];
  double dtime=srate*newlen+0.5;
  if(tsus==tdelay) {
    /*Code for when there is no release in the envelope*/
    env->time[4]=dtime;
    env->time[5]=dtime;
  }
  else {
    env->time[4]=dtime+env->time[4]-env->time[5];
    env->time[5]=dtime;
  }
  env->fac[3]=pow(env->amp[4]/env->amp[3], 1.0/(env->time[4]-env->time[3]));
  env->section=env->start;
  env->pos=0;
  env->morepts=1;
  env->val=1.0e-4;
}
