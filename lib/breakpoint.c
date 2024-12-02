/* Project to expand and make breakpoint file functions more compatible with
** frequency sample generation functions with the possibility of ignoring
** them altogether if the breakpoints included have frequency values */
#include "sampgen.h"

int readnum(BREAKPOINT *bpt, char *line, int ftype, int cols) {
  // Assume breakpoint format of [time]\t[freq1 ... freqc]\t[amp1 ... ampc]\t[props1 ... propsc]\n with sections dependent on ftype
  int fg=0, ag=0, pg=0;
  note_type pval;
  char **tail, *strpos;
  double val;
  if(line[0]=='\n'||line[0]=='\0') return ERR_NONE; /* Case when file ends with empty line shouldn't be an error! */
  tail=&line;
  strpos=line;
  val=strtod(strpos, tail);
  if(**tail!='\t') return ERR_BRKPT_FORM;
  bpt->time=val;
  ++(*tail);
  strpos=*tail;

  if(ftype!=AMP_BRK&&ftype!=AMP_EXP) {
    // Use the strtod standard function to read values and get back pointer:
    while(fg<cols) {
      val=strtod(strpos, tail);
      if(**tail!='\t' && **tail!='\n' && **tail!='\0') return ERR_BRKPT_FORM;
      if(**tail=='\t') strpos=++(*tail);
      bpt->freq[fg++][0]=val;
      if(**tail=='\n' && fg<cols) return ERR_BRKPT_FORM;
    }
  }
  if(ftype!=FREQ_BRK&&ftype!=FREQ_EXP) {
    while(ag<cols) {
      val=strtod(strpos, tail);
      if(**tail!='\t' && **tail!='\n' && **tail!='\0') return ERR_BRKPT_FORM;
      if(**tail=='\t') strpos=++(*tail);
      bpt->amp[ag++]=val;
      if(**tail=='\n' && ag<cols) return ERR_BRKPT_FORM;
    }
  }
  if(ftype>ALL_BRK) {
    while(pg<cols) {
      pval=strtol(strpos, tail,10);
      if(**tail!='\t' && **tail!='\n' && **tail!='\0') return ERR_BRKPT_FORM;
      if(**tail=='\t') strpos=++*tail;
      bpt->props[pg++]=pval;
      if(**tail=='\n' && pg<cols) return ERR_BRKPT_FORM;
    }
  }
  if(**tail!='\n') return ERR_BRKPT_FORM;
  return ERR_NONE;
}

void free_breakpoint(BREAKPOINT *btable, int ftype, int cols, unsigned long npoints) {
  unsigned long i=0,j;
  if(btable) {
    while(i<npoints) {
      for(j=0;j<blocklen;j++) {
        if(ftype!=AMP_BRK&&ftype!=AMP_EXP) {
          for(int k=0;k<cols;k++) free(btable[i+j].freq[k]);
          free(btable[i+j].freq);
        }
        if(ftype!=FREQ_BRK && ftype!=FREQ_EXP) free(btable[i+j].amp);
        if(ftype>ALL_BRK) free(btable[i+j].props);
      }
      i+=j;
    }
    free(btable);
  }
}

BREAKPOINT *new_breakpoints(BREAKPOINT *btable, int ftype, int cols, unsigned long npoints) {
  unsigned long i;
  BREAKPOINT *temp;
  if(!btable) /* allocate new memory here */
    temp=malloc(sizeof(BREAKPOINT)*blocklen);
  else
    temp=realloc(btable, sizeof(BREAKPOINT)*(npoints+blocklen));
  if(temp) {
    for(i=0;i<blocklen;i++) {
      if(ftype!=AMP_BRK && ftype!=AMP_EXP) {
        temp[npoints+i].freq=malloc(sizeof(double*)*cols);
        for(int j=0;j<cols;j++) temp[npoints+i].freq[j]=malloc(sizeof(double));
      }
      if(ftype!=FREQ_BRK && ftype!=FREQ_EXP)
        temp[npoints+i].amp=malloc(sizeof(double)*cols);
      if(ftype>ALL_BRK)
        temp[npoints+i].props=malloc(sizeof(note_type)*cols);
    }
    if((ftype!=AMP_BRK && ftype!=AMP_EXP && temp[npoints+blocklen-1].freq==NULL) ||
    (ftype!=FREQ_BRK && ftype!=FREQ_EXP && temp[npoints+blocklen-1].amp==NULL) ||
    (ftype>ALL_BRK && temp[npoints+blocklen-1].props==NULL)) { free_breakpoint(temp, ftype, cols, npoints+blocklen);}
  }
  return temp;
}

BREAKPOINTS *get_breakpoints(FILE *fp, int ftype, int cols, unsigned long *psize, int *err) {
  unsigned long npoints=0;
  char chunk[128];
  size_t len=sizeof(chunk);
  char *line=malloc(sizeof(double)*maxcols*3/sizeof(char)+1);
  BREAKPOINTS *points=(BREAKPOINTS *) malloc(sizeof(BREAKPOINTS));

  if(line==NULL || points==NULL || fp==NULL) *err=ERR_MEMORY;
  line[0]='\0';
  while(fgets(chunk, sizeof(chunk), fp) != NULL && *err==ERR_NONE) {
    if(chunk[0]=='\n') break; // End of file!
    // Allocate breakpoints if necessary
    if(npoints%blocklen==0) {
      points->table=new_breakpoints(points->table, ftype, cols, npoints);
      if(points->table==NULL) *err=ERR_MEMORY;
    }
    // Resize the line buffer if necessary
    size_t len_used = strlen(line);
    size_t chunk_used = strlen(chunk);

    if(len - len_used < chunk_used) {
      len *= 2;
      if((line = realloc(line, len)) == NULL) *err=ERR_MEMORY;
    }
    if(*err!=ERR_NONE) break;
    strncpy(line + len_used, chunk, len - len_used);
    len_used += chunk_used;
    *err=readnum(&(points->table[npoints]), line, ftype, cols);
    npoints++;
    line[0]='\0';
  }

  if(line) free(line);
  if(*err) {
    if(points) {free_breakpoint(points->table, ftype, cols, npoints);free(points);}
    return NULL;
  }
  *psize=npoints;
  points->cols=cols;
  points->ftype=ftype;
  return points;
}

int okbpts(BREAKPOINTS *points, int ftype, int cols, unsigned long npoints) {
  unsigned long i, j;
  BREAKPOINT *bp=points->table;
  double lasttime=bp[0].time, val;
  if(lasttime!=0.0) return ERR_BRKPT_TIME;
  for(i=1;i<npoints;i++) {
    if(bp[i].time<lasttime+minenv) return ERR_BRKPT_TIME;
    lasttime=bp[i].time;
  }
  for(i=0;i<npoints;i++) {
    for(j=0;j<cols;j++) {
      if(ftype!=FREQ_BRK && ftype!=FREQ_EXP) {
        val=bp[i].amp[j];
        if(val<minamp || val>maxamp) return ERR_BRKPT_AMP;
      }
      if(ftype==AMP_EXP && bp[i].props[j]>SLUR) return ERR_BRKPT_PROPS;
      if(i==npoints-1 && ftype>ALL_BRK && bp[i].props[j]!=NONE) return ERR_BRKPT_PROPS;
    }
  }
  return ERR_NONE;
}

int okfreq(BREAKPOINTS *points, int startcol, int endcol, unsigned long npoints) {
  unsigned long i, j;
  BREAKPOINT *bp=points->table;
  double val;

  for(i=0;i<npoints;i++) {
    for(j=startcol;j<endcol;j++) {
      val=bp[i].freq[j][0];
      if(val<minfreq ||val>maxfreq) return ERR_BRKPT_FREQ;
    }
  }
  return ERR_NONE;
}

void bps_free(BRKSTREAM *stream) {
  if(stream) {
    int cols=stream->points->cols, ftype=stream->points->ftype, i, j;
    if(stream->waves) {
      for(i=0;i<cols;i++) {
        for(j=i+1;j<cols;j++) {
          if(stream->waves[i] && stream->waves[j] && stream->waves[i]->gtable==stream->waves[j]->gtable) stream->waves[j]=NULL;
        }
        if(stream->waves[i]) {
          free(stream->waves[i]->gtable->table);
          free(stream->waves[i]->gtable);
          stream->waves[i]=NULL;
        }
      }
      for(i=0;i<cols;i++) free(stream->waves[i]);
      free(stream->waves);
    }
    if(stream->envelopes) {
      for(i=0;i<cols;i++) {
        if(stream->envelopes[i]) {
          free(stream->envelopes[i]->amp);
          free(stream->envelopes[i]->fac);
          free(stream->envelopes[i]->time);
          free(stream->envelopes[i]);
        }
      }
      free(stream->envelopes);
    }
    if(stream->tuned) free(stream->tuned);
    free_breakpoint(stream->points->table, ftype, cols, stream->npoints);
    free(stream->points);
    free(stream);
  }
}

waveformfunc wfuncs[]={&new_sine,&new_triangle,&new_square,&new_upsaw,&new_downsaw};
ampfunc efuncs[]={&env_harsh,&env_sustained};

BRKSTREAM *bps_newstream(FILE *fp, env_type *envs, wav_type *waves, BRK_PROPS *props, unsigned long *psize, int *err) {
  BRKSTREAM *stream;
  BREAKPOINTS *bpts;
  *err=ERR_NONE;

  if(props==NULL || envs==NULL || waves==NULL) {
    *err=ERR_ARGS;
    return NULL;
  }
  unsigned long i, j, npoints, srate=props->srate;
  int ftype=props->ftype, cols=props->cols;
  if(srate==0||cols<1||cols>maxcols) {
    *err=ERR_ARGS;
    return NULL;
  }
  bpts=get_breakpoints(fp, ftype, cols,  &npoints, err);
  if(*err!=ERR_NONE) return NULL;
  *err=okbpts(bpts, ftype, cols, npoints);
  if(*err!=ERR_NONE) {
    free_breakpoint(bpts->table, ftype, cols, npoints);
    free(bpts);
    return NULL;
  }
  stream=(BRKSTREAM *) malloc(sizeof(BRKSTREAM));
  if(stream==NULL) {
    *err=ERR_MEMORY;
    free_breakpoint(bpts->table, ftype, cols, npoints);
    free(bpts);
    return NULL;
  }
  stream->waves=(OSCILT **) malloc(sizeof(OSCILT*)*cols);
  stream->envelopes=(ENV **) malloc(sizeof(ENV*)*cols);
  stream->tuned=malloc(sizeof(int)*cols);
  if(stream->waves==NULL||stream->envelopes==NULL||stream->tuned==NULL) {
    *err=ERR_MEMORY;
    bps_free(stream);
    free(stream);
    return NULL;
  }
  for(i=0;i<cols;i++) { /* Set envelopes/waves: */
    double len; //local variable for envelope durations
    int endslurred=0, prop; //local variables to check note properties and where slurred note ends
    GTABLE *gtable;
    stream->tuned[i]=1;
    stream->waves[i]=NULL;
    for(j=0;j<i;j++) {
      if(waves[j]==waves[i]) {
        stream->waves[i]=new_oscilt(srate, stream->waves[j]->gtable, 0.0);
        break;
      }
    }
    if(stream->waves[i]==NULL) {
      gtable=(wfuncs[waves[i]])(tablen, harmonics);
      stream->waves[i]=new_oscilt(srate, gtable, 0.0);
    }
    if(stream->waves[i]==NULL) {
      *err=ERR_MEMORY;
      bps_free(stream);
      free(stream);
      return NULL;
    }
    if(ftype<=ALL_BRK) { /* Filetype without note properties */
      len=bpts->table[1].time;
    }
    else {
      do {
        prop=bpts->table[endslurred].props[i];
        if(endslurred==npoints-1) break;
        endslurred++;
      } while(prop==SLUR || prop==ALLPROPS);
      len=bpts->table[endslurred].time;
    }
    stream->envelopes[i]=(efuncs[envs[i]])(len, srate);
    if(stream->envelopes[i]==NULL) {
      *err=ERR_MEMORY;
      bps_free(stream);
      free(stream);
      return NULL;
    }
  }
  stream->pos=0.0;
  stream->npoints=npoints;
  stream->srate=srate;
  stream->points=bpts;
  stream->points->tcols=0; // Number cols tuned by bps_inittune
  stream->incr=1.0/srate;
  stream->ileft=0;
  stream->iright=1;
  stream->left=stream->points->table[stream->ileft];
  stream->right=stream->points->table[stream->iright];
  stream->width=(stream->right.time-stream->left.time);
  stream->morepts=1;
  if(psize)
    *psize=stream->npoints;
  return stream;
}

tuningfunc tfuncs[5]={NULL, &ntet_tune, &kirn3_tune, &javas_tune, &javap_tune}; // array of all tuning function addresses

int bps_inittune(BRKSTREAM *stream, T_PROPS *props, int newcols) {
  if(stream==NULL||props==NULL) return ERR_ARGS;
  int firstcol=stream->points->tcols, lastcol=firstcol+newcols, i, j, k, p, grange, ftype;
  if(lastcol>stream->points->cols) return ERR_ARGS;
  /* Check props struct based on tuning system: */
  switch(props->tuning) {
  case UNTUNED:
    for(int i=firstcol;i<lastcol;i++) stream->tuned[i]=0;
    break;
  case NTET:
    if(props->scale<0) return ERR_T_PROPS;
    break;
  case KIRN3:
    double c5=440*pow(2, 0.25);
    if(props->root!=72||props->frequency<=80/81*c5||props->frequency>=81/80*c5) return ERR_T_PROPS;
    break;
  case JAVAS:
  case JAVAP:
    if(props->root<0 || props->frequency<minfreq || props->frequency>maxfreq || props->ratios==NULL) return ERR_T_PROPS;
    break;
  }
  /* Apply tuning: */
  double val;
  ftype=stream->points->ftype;
  BREAKPOINT *bpts=stream->points->table;
  if(props->tuning!=UNTUNED && ftype!=AMP_BRK && ftype!=AMP_EXP) {
    for(j=firstcol;j<lastcol;j++) {
      for(i=0;i<stream->npoints;i++) {
        /* Check for non integer note numbers: */
        val=bpts[i].freq[j][0]-(int) bpts[i].freq[j][0];
        if(val!=0) return ERR_BRKPT_FREQ;
        /* grange stores number of frequencies in reallocated bpts[i].freq[j] array: */
        if(ftype<=ALL_BRK) grange=0;
        else {
          p=bpts[i].props[j];
          if(p!=GLISS&&p!=ALLPROPS) grange=0;
          else grange=bpts[i+1].freq[j][0]-bpts[i].freq[j][0];
        }
        if(grange==0) {
          val=(tfuncs[props->tuning])(props, bpts[i].freq[j][0]);
          bpts[i].freq[j][0]=val;
        }
        else {
          int l=fabs(grange)+1;
          double *temp=malloc(sizeof(double)*l);
          bpts[i].freq[j]=realloc(bpts[i].freq[j], sizeof(double)*l);
          if(temp==NULL||bpts[i].freq[j]==NULL) return ERR_MEMORY;
          for(k=0;k<l-1;k++) {
            if(grange>0) temp[k]=(tfuncs[props->tuning])(props, bpts[i].freq[j][0]+k);
            else temp[k]=(tfuncs[props->tuning])(props, bpts[i].freq[j][0]-k);
          }
          temp[k]=0;
          bpts[i].freq[j]=temp;
        }
      }
      stream->points->tcols++;
    }
  }
  /* Now run okbpts over new values*/
  return okfreq(stream->points, firstcol, lastcol-1, stream->npoints);
}

void changeenv(BRKSTREAM *stream, int ftype, int cols, unsigned long srate) {
  int i,endenv,prop, ir=stream->iright; /*endenv stores the end of the next note of envelope, prop stores whether the current note is slurred */
  /* Change envelopes if stream->envelopes[i]->morepts==0 or if breakpts are all unslurred: */
  for(i=0;i<cols;i++) {
    if(stream->envelopes[i]->morepts==0) {
      if(ftype<=ALL_BRK)
        env_reset(stream->envelopes[i], (stream->points->table[ir+1].time-stream->pos), srate);
      else {
        /* Need to find endenv */
        endenv=ir;
        do {
          prop=stream->points->table[endenv].props[i];
          if(endenv==stream->npoints-1) break;
          endenv++;
        } while(prop==SLUR || prop==ALLPROPS);
        env_reset(stream->envelopes[i], (stream->points->table[endenv].time-stream->pos), srate);
      }
    }
  }
}

void changepts(BRKSTREAM *stream) {
  stream->pos+=stream->incr;
  if(stream->pos>=stream->right.time) {
    stream->ileft++;
    stream->iright++;
    if(stream->iright==stream->npoints) {
      stream->morepts=0;
    }
    else {
      stream->left=stream->points->table[stream->ileft];
      stream->right=stream->points->table[stream->iright];
      stream->width=stream->right.time-stream->left.time;
    }
  }
}

double bps_freq_tick(BRKSTREAM *stream) {
  int c=stream->points->cols;
  double val=0, cfrac=1.0/c;
  if(stream->morepts) {
    changeenv(stream, stream->points->ftype, c, stream->srate);
    for(int i=0;i<c;i++) {
      val+=cfrac*defaultamp*env_tick(stream->envelopes[i])*tabtick(stream->waves[i], stream->left.freq[i][0]);
    }
  }
  else {
    for(int i=0;i<c;i++) {
      val+=cfrac*1.0e-4*tabtick(stream->waves[i], stream->right.freq[i][0]);
    }
  }
  changepts(stream);
  return val;
}

double bps_amp_tick(BRKSTREAM *stream) {
  int c=stream->points->cols;
  double val=0, cfrac=1.0/c;
  if(stream->morepts) {
    changeenv(stream, stream->points->ftype, c, stream->srate);
    for(int i=0;i<c;i++) {
      val+=cfrac*stream->left.amp[i]*env_tick(stream->envelopes[i])*tabtick(stream->waves[i], defaultfreq);
    }
  }
  else {
    for(int i=0;i<c;i++) {
      val+=cfrac*1.0e-4*tabtick(stream->waves[i], defaultfreq);
    }
  }
  changepts(stream);
  return val;
}

double bps_all_tick(BRKSTREAM *stream) {
  int c=stream->points->cols;
  double val=0, cfrac=1.0/c;
  if(stream->morepts) {
    changeenv(stream, stream->points->ftype, c, stream->srate);
    for(int i=0;i<c;i++) {
      val+=cfrac*stream->left.amp[i]*env_tick(stream->envelopes[i])*tabtick(stream->waves[i], stream->left.freq[i][0]);
    }
  }
  else {
    for(int i=0;i<c;i++) {
      val+=cfrac*1.0e-4*tabtick(stream->waves[i], stream->right.freq[i][0]);
    }
  }
  changepts(stream);
  return val;
}

double bps_freqexp_tick(BRKSTREAM *stream) {
  int c=stream->points->cols, p;
  double val=0, cfrac=1.0/c, newfreq;
  if(stream->morepts) {
    changeenv(stream, stream->points->ftype, c, stream->srate);
    double bptfrac=(stream->pos-stream->left.time)/stream->width;
    for(int i=0;i<c;i++) {
      p=stream->left.props[i];
      newfreq=stream->left.freq[i][0];
      if(p==GLISS||p==ALLPROPS) {
        if(stream->tuned[i]) {
          int glen=0;
          while(stream->left.freq[i][glen++]);
          newfreq=stream->left.freq[i][(int) (bptfrac*(glen-1))];
        }
        else {
          newfreq=stream->left.freq[i][0];
          newfreq+=bptfrac*(stream->right.freq[i][0]-stream->left.freq[i][0]);
        }
      }
      val+=cfrac*defaultamp*env_tick(stream->envelopes[i])*tabtick(stream->waves[i], newfreq);
    }
  }
  else {
    for(int i=0;i<c;i++) {
      val+=cfrac*1.0e-4*tabtick(stream->waves[i], stream->right.freq[i][0]);
    }
  }
  changepts(stream);
  return val;
}

double bps_ampexp_tick(BRKSTREAM *stream) {
  int c=stream->points->cols;
  double val=0, cfrac=1.0/c;
  if(stream->morepts) {
    changeenv(stream, stream->points->ftype, c, stream->srate);
    for(int i=0;i<c;i++) {
      val+=cfrac*stream->left.amp[i]*env_tick(stream->envelopes[i])*tabtick(stream->waves[i], defaultfreq);
    }
  }
  else {
    for(int i=0;i<c;i++) {
      val+=cfrac*1.0e-4*tabtick(stream->waves[i], defaultfreq);
    }
  }
  changepts(stream);
  return val;
}

double bps_allexp_tick(BRKSTREAM *stream) {
  int c=stream->points->cols, p;
  double val=0, cfrac=1.0/c, newfreq;
  if(stream->morepts) {
    changeenv(stream, stream->points->ftype, c, stream->srate);
    double bptfrac=(stream->pos-stream->left.time)/stream->width;
    for(int i=0;i<c;i++) {
      p=stream->left.props[i];
      newfreq=stream->left.freq[i][0];
      if(p==GLISS||p==ALLPROPS) {
        if(stream->tuned[i]) {
          int glen=0;
          while(stream->left.freq[i][glen++]);
          newfreq=stream->left.freq[i][(int) (bptfrac*(glen-1))];
        }
        else {
          newfreq=stream->left.freq[i][0];
          newfreq+=bptfrac*(stream->right.freq[i][0]-stream->left.freq[i][0]);
        }
      }
      val+=cfrac*stream->left.amp[i]*env_tick(stream->envelopes[i])*tabtick(stream->waves[i], newfreq);
    }
  }
  else {
    for(int i=0;i<c;i++) {
      val+=cfrac*1.0e-4*tabtick(stream->waves[i], stream->right.freq[i][0]);
    }
  }
  changepts(stream);
  return val;
}
