/*
 * Memory System Performance Characterization
 * ECT memperf - Extended Copy Transfer Characterization
 *
 * Thomas M. Stricker <tomstr@inf.ethz.ch>
 * Christian Kurmann  <kurmann@inf.ethz.ch>
 * http://www.cs.inf.ethz.ch/CoPs/ECT/
 *
 * Changes in Changelog
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <sys/shm.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>

#include "par.h"

#define DWORD unsigned int
#define WINAPI
#define __cdecl

#ifdef ARMV7ACOUNTER
#include "gettimearmv7a.h"
#define gettime getclock
#endif

#define VERSION "v0.10a"

#define mytime int

/* defaults / limits */
/* strides (-1=last entry) */
int strdr[]={1,2,3,4,5,6,7,8,12,15,16,24,31,32,48,63,64,96,127,128,192,-1};

#define MXBLSZ   24          /* 2^x max block size (* 8 bytes(double)) */
#define MXSTRDS  22          /* max/default number of stride tests for each size */
#define MXRUNSZ  20          /* 2^x default block size (* 8 bytes) */
#define MINRUNSZ 6           /* 2^x min block size (* 8 bytes) */
#define MXIT     16          /* number of iterations (default) */
#define NPES     1           /* number of processes (default) */
#define MXPROC   16          /* max number of parallel processes */
#define OPTASM   1           /* use special instructions per default */
#define MXREP    7           /* number of repetitions allowed */
#define NROFREP  3           /* default number of repetitions */

#define OUTLIERLIMIT 0.75    /* limit for delaring a result as outlier */
#define KORR     0           /* usec for correction of timer (0 iterations) */
#define TIC      1.0         /* default clock resolution, ticks per us */

/*#define SHAREDMEM             use same shared block of memory for each proc */
/*#define PENTIUMCOUNTER          use high resolution pentium counter */

#define DEBUG    0           /* save debug information in files */
#define CHART    0           /* save results from repetitions in files */
#define CHARTREV 0           /* reverse chart output -> output can be imported in Excel e.g. */
#define DEBUGP   0           /* print debug info on screen */

#define DEBUGOUT if (DEBUG)
#define CHARTOUT if (CHART)
#define DEBUGPRINT if (DEBUGP)

int chartrev; // chart reverse
double tic;
double *shared;
int nrofrep = NROFREP;

typedef struct
{
  int mxstrds;
  int mxsize;
  int minsize;
  int mxiters;
  int npes;
  int mode;
  int par_cid;
  int useoptasm;
  int currentrep;
} paramT;

#include "cpy.h"

#ifdef HAVEOPT
#include "cpy_opt.h"
#else
/*functions never to be called, just to get rid of later ifdefs...*/
int cpy_lsopt(double* a,int l,int mx,int it) { return (int) (*a+l+mx+it); }
int cpy_vsopt(double* a,int l,int mx,int it) { return (int) (*a+l+mx+it); }
int cpy_lcopt(double* a,double* c,int l,int mx,int it) { return (int) (*a+l+mx+it+*c); }
int cpy_csopt(double* a,double* c,int l,int mx,int it) { return (int) (*a+l+mx+it+*c); }
#endif

DWORD WINAPI memop(paramT *p)
{
  double *a,*c; // buffer
  int i,j,l;
  float t,t1,t2,t4,t8,topt;
  int li,nel,mx,effiters;

  double bandwith[MXBLSZ][MXSTRDS];
  char name[80], namebwf[80], number[10];
  FILE *stream, *bwfout;

  int x=0,y=0;
  /*printf("address %d size %d, iter %d, Process %d\n",p,p->mxsize, p->mxiters, p->par_cid);
  fflush(stdout);
  */
#ifdef SHAREDMEM
  a=shared;
#else
  a=(double *)malloc(sizeof(double)*p->mxsize);
#endif
  c=(double *)malloc(sizeof(double)*p->mxsize+8192);
  t1=t2=t4=t8=topt=0;

  /* Dump Debug-Info in outputfile */
  DEBUGOUT {
    strcpy(name,"debug");
    strcpy(number,".x");
    number[1]= '0';
    strcat(name,number);
    strcat(name,".out");

    stream = fopen( name, "w" );

    if (stream==NULL) {
      printf("Can't open %s\n",name);
      exit(0);
    }
  }

  /* initialize memory segment */
  for (i=0; i<p->mxsize; i++) {
    // for longlong type accumulate
    ((long long int*)a)[i] = 1;
    ((long long int*)c)[i] = 1;
    //((float *)a)[i * 2] = 1.f; ((float *)a)[i * 2 + 1] = 1.f;
    //((float *)c)[i * 2] = 1.f; ((float *)c)[i * 2 + 1] = 1.f;
  }

  /* stride loop */
  for (li=0, y=1; (strdr[li]!=-1) && (li<p->mxstrds); li++, y++) {
    l=strdr[li];
    bandwith[0][y] = l;

    barrier();

    DEBUGOUT {
      if (p->par_cid==0) {
        fprintf( stream, "\nMemory Op: ");
        switch (p->mode) {
        case 0:
          fprintf( stream, "Load sum\n");
          break;
        case 1:
          fprintf( stream, "Const store\n");
          break;
        case 2:
          fprintf( stream, "Load copy\n");
          break;
        case 3:
          fprintf( stream, "Copy store\n");
          break;
        }

        fprintf( stream, "%4s %6s %6s %16s %16s %16s %10s\n",
          "proc","stride","iters","blk size",
          "eff. size","exec.time","bandwidth");

        fprintf( stream, "%4s %6s %6s %16s %16s %16s %10s\n",
          "#","bytes","#","bytes",
          "bytes","clocks","MB/sec");
      }
    }

    barrier();

    /* block size loop */
    for (x=1, mx=p->minsize; mx<=p->mxsize; mx*=2, x++) {
      bandwith[x][0] = mx*8;
      barrier();

      /* set number of iterations */
      effiters = p->mxiters;
      if (effiters>1) {
        if (mx<128)  effiters=p->mxiters*16;
        if ((mx>=128) && (mx<=1024)) effiters=p->mxiters*4;
        if ((mx>=256*256) && (mx<=512*512)) effiters=4;
        if (mx>512*512) effiters=1;
      }

      /* warm up memory */
      // TODO: store test
      long long int sum = 0;
      for (i=0; i<mx; i++) {
        sum += ((long long int *)a)[i];// c[i] += 1.0;
      }
      //printf("warm up memory sum = %lld\n", sum);

      barrier();

      t=-KORR;  /* initialize timer */

      /* perform test */
      switch (p->mode) {
      case 0:
        if (p->useoptasm != 1) {
          t1=cpy_ls1(&a[0],l,mx,effiters);
          t2=cpy_ls2(&a[0],l,mx,effiters);
          t4=cpy_ls4(&a[0],l,mx,effiters);
          t8=cpy_ls8(&a[0],l,mx,effiters);
        }
        if (p->useoptasm != 0) {
          topt=cpy_lsopt(&a[0],l,mx,effiters);
        }
        break;
      case 1:
        if (p->useoptasm != 1) {
          t1=cpy_vs1(&a[0],l,mx,effiters);
          t2=cpy_vs2(&a[0],l,mx,effiters);
          t4=cpy_vs4(&a[0],l,mx,effiters);
          t8=cpy_vs8(&a[0],l,mx,effiters);
        }
        if (p->useoptasm != 0) {
          topt=cpy_vsopt(&a[0],l,mx,effiters);
        }
        break;
      case 2:
        if (p->useoptasm != 1) {
          t1=cpy_lc1(&a[0],&c[0],l,mx,effiters);
          t2=cpy_lc2(&a[0],&c[0],l,mx,effiters);
          t4=cpy_lc4(&a[0],&c[0],l,mx,effiters);
          t8=cpy_lc8(&a[0],&c[0],l,mx,effiters);
        }
        if (p->useoptasm != 0) {
          topt=cpy_lcopt(&a[0],&c[0],l,mx,effiters);
        }
        break;
      case 3:
        if (p->useoptasm != 1) {
          t1=cpy_cs1(&a[0],&c[0],l,mx,effiters);
          t2=cpy_cs2(&a[0],&c[0],l,mx,effiters);
          t4=cpy_cs4(&a[0],&c[0],l,mx,effiters);
          t8=cpy_cs8(&a[0],&c[0],l,mx,effiters);
        }
        if (p->useoptasm != 0) {
          topt=cpy_csopt(&a[0],&c[0],l,mx,effiters);
        }
        break;
      }
      switch (p->useoptasm) {
      case 0:
        DEBUGOUT printf("mx %d: t1 %f, t2 %f, t4 %f, t8 %f  \n ",mx,t1,t2,t4,t8);
        t1=t1<t2?t1:t2;t1=t1<t4?t1:t4;t1=t1<t8?t1:t8;
        break;
      case 1:
        DEBUGOUT printf("mx %d: topt %f \n ",mx, topt);
        t1=topt;
        break;
      case 2:
        DEBUGOUT printf("mx %d: t1 %f, t2 %f, t4 %f, t8 %f, topt %f  \n ",mx,t1,t2,t4,t8,topt);
        t1=t1<t2?t1:t2;t1=t1<t4?t1:t4;t1=t1<t8?t1:t8;t1=t1<topt?t1:topt;
        break;
      }

      t+=t1;
      nel = 8 * l * (mx/l) * effiters;

      DEBUGOUT {
        if (p->par_cid==0)
          fprintf( stream, "p%3d %6d %6d %16d %16d %8.2f %8.2f\n",
          p->npes,l,effiters,mx*8,nel,t,(float)nel / 1024 / (t / tic)); // KB / us
      }

      bandwith[x][y] = (float) nel/1024./(t/tic); // KB / us TODO:
    }
  }

  DEBUGOUT fclose( stream );

  /* always output the float values */
  strcpy(name,"chart");
  strcpy(number,".m0");
  number[2]= '0'+ (char) p->mode;
  strcat(name,number);
  strcpy(number,".p0");
  number[2]= '0'+p->npes;
  strcat(name,number);

#ifdef SHAREDMEM
  strcat(name,".shared");
#endif
  strcat(name,".out");
  strcpy(number,".r0");
  number[2]= '0'+p->currentrep;
  strcat(name,number);
  strcpy(number,".0");
  number[1]= '0'+p->par_cid+1;
  strcat(name,number);

  strcpy(namebwf,name);
  strcat(namebwf,"f");

  if (p->par_cid!=0) {
    usleep(p->par_cid*10000);
  }
  /* we need the "b" option in fopen for windows only, hopefully all other systems will ignore it */
  bwfout = fopen(namebwf, "wb");
  if (bwfout==NULL) {
    printf("Can't open %s\n",namebwf);
    exit(0);
  }
  if (fwrite(bandwith,sizeof (double),MXBLSZ*MXSTRDS,bwfout)!=MXBLSZ*MXSTRDS)
    printf("Couldn't output temp data\n");
  fclose(bwfout);

  /* Dump result in outputfile */
  CHARTOUT {
    if (chartrev)
      printf("                  Reversed results in: %s\n",name);
    else
      printf("                  Results in: %s\n",name);

    stream = fopen( name, "w" );

    if (stream==NULL) {
      printf("Can't open %s\n",name);
      exit(0);
    }

    switch (p->mode) {
    case 0:
      fprintf( stream, "Load sum   ");
      break;
    case 1:
      fprintf( stream, "Const store");
      break;
    case 2:
      fprintf( stream, "Load copy  ");
      break;
    case 3:
      fprintf( stream, "Copy store ");
      break;
    }

    if (chartrev) {
      for (i=x-1;i>0;i--)	{
        if (bandwith[i][0]<1024)
          fprintf(stream,"%6.1f K",bandwith[i][0]/1024);
        else if (bandwith[i][0]<1024*1024)
          fprintf(stream,"%6d K",(int) bandwith[i][0]/1024);
        else
          fprintf(stream,"%6d M",(int) bandwith[i][0]/(1024*1024));
      }
      fprintf(stream,"\n");
      for (j=1;j<y;j++)	{
        fprintf(stream,"%8d   ",(int)bandwith[0][j]);
        for (i=x-1;i>0;i--)	{
          fprintf(stream,"%8.2f",bandwith[i][j]);
        }
        fprintf(stream,"\n");
      }
    } else {
      for (i=1;i<x;i++)	{
        if (bandwith[i][0]<1024)
          fprintf(stream,"%6.1f K",bandwith[i][0]/1024);
        else if (bandwith[i][0]<1024*1024)
          fprintf(stream,"%6d K",(int) bandwith[i][0]/1024);
        else
          fprintf(stream,"%6d M",(int) bandwith[i][0]/(1024*1024));
      }
      fprintf(stream,"\n");
      for (j=1;j<y;j++)	{
        fprintf(stream,"%8d   ",(int)bandwith[0][j]);
        for (i=1;i<x;i++)	{
          fprintf(stream,"%8.2f",bandwith[i][j]);
        }
        fprintf(stream,"\n");
      }
    }
    fclose( stream );
  }


#ifndef SHAREDMEM
  free( a );
#endif
  free( c );
  return 0;
}


void analyze_rep(paramT *p) {

  int ploop,itloop,i,j,mx,maxrow,maxcol,outliers,bwcount,z;
  double bwmax,bwmean,corrbwmean,stddev,corrstddev,devmean,corrdevmean;
  char name[80], namebw[80], number[10];
  double bandwith[MXREP][MXPROC][MXBLSZ][MXSTRDS];
  FILE *stream, *resin;


  for (maxrow=0; (strdr[maxrow]!=-1) && (maxrow<p->mxstrds); maxrow++);
  for (maxcol=0, mx=p->minsize; mx<=p->mxsize; mx*=2, maxcol++);

  for (itloop=0; itloop<nrofrep; itloop++) {
    for (ploop=0; ploop<p->npes;ploop++) {
      strcpy(namebw,"chart");
      strcpy(number,".m0");
      number[2]= '0'+ (char) p->mode;
      strcat(namebw,number);
      strcpy(number,".p0");
      number[2]= '0'+p->npes;
      strcat(namebw,number);

#ifdef SHAREDMEM
      strcat(namebw,".shared");
#endif
      strcat(namebw,".out");
      strcpy(number,".r0");
      number[2]= '0'+itloop+1;
      strcat(namebw,number);
      strcpy(number,".0");
      number[1]= '0'+ploop+1;
      strcat(namebw,number);
      strcat(namebw,"f");
    /* we need the "b" option in fopen for windows only, hopefully all other systems will ignore it */
      resin = fopen(namebw, "rb");
      if (resin==NULL) {
        printf("Can't open input file for reading %s\n",namebw);
        exit(0);
      }
      if (fread(bandwith[itloop][ploop],sizeof (double),MXBLSZ*MXSTRDS,resin)!=MXBLSZ*MXSTRDS)
        printf("Couldn't read temp data\n");
      fclose(resin);
      remove(namebw);
    }
  }

  strcpy(name,"chart");
  strcpy(number,".m0");
  number[2]= '0'+ (char) p->mode;
  strcat(name,number);
  strcpy(number,".p0");
  number[2]= '0'+p->npes;
  strcat(name,number);

#ifdef SHAREDMEM
  strcat(name,".shared");
#endif
  strcat(name,".max");

  if (chartrev)
    printf("          Maximum reversed results in: %s\n",name);
  else
    printf("          Maximum results in: %s\n",name);

  stream = fopen( name, "w" );

  if (stream==NULL) {
    printf("Can't open %s\n",name);
    exit(0);
  }

  switch (p->mode) {
  case 0:
    fprintf( stream, "LoadSum\t");
    break;
  case 1:
    fprintf( stream, "ConstStore\t");
    break;
  case 2:
    fprintf( stream, "LoadCopy\t");
    break;
  case 3:
    fprintf( stream, "CopyStore\t");
    break;
  }
  outliers=0;
  devmean=0;
  corrdevmean=0;
  if (chartrev) i=maxcol;
  else i=1;
  for (z=1;z<=maxcol;z++) {
    if (bandwith[0][0][i][0]<1024)
      fprintf(stream,"%1.1fK\t",bandwith[0][0][i][0]/1024);
    else if (bandwith[0][0][i][0]<1024*1024)
      fprintf(stream,"%dK\t",(int) bandwith[0][0][i][0]/1024);
    else
      fprintf(stream,"%dM\t",(int) bandwith[0][0][i][0]/(1024*1024));
    if (chartrev) i--;
    else i++;
  }
  fprintf(stream,"\n");

  for (j=1;j<=maxrow;j++) {
    fprintf(stream,"%3d\t",(int)bandwith[0][0][0][j]);
    z=maxcol;
    if (chartrev) i=maxcol;
    else i=1;
    for (z=1;z<=maxcol;z++) {
      bwmax = 0;
      bwmean = 0;
      corrbwmean = 0;
      bwcount = 0;
      stddev = 0.0;
      corrstddev = 0.0;
      for (itloop=0;itloop<nrofrep;itloop++) {
        for (ploop=0;ploop<p->npes;ploop++) {
          bwmax=bwmax>bandwith[itloop][ploop][i][j]?bwmax:bandwith[itloop][ploop][i][j];
          bwmean += bandwith[itloop][ploop][i][j];
        }
      }
      bwmean = bwmean/(nrofrep*p->npes);
      for (itloop=0;itloop<nrofrep;itloop++) {
        for (ploop=0;ploop<p->npes;ploop++) {
          if (bandwith[itloop][ploop][i][j]<(OUTLIERLIMIT*bwmax))
            outliers += 1;
          else {
            bwcount += 1;
            corrbwmean += bandwith[itloop][ploop][i][j];
          }
        }
      }
      corrbwmean = corrbwmean/bwcount;
      for (itloop=0;itloop<nrofrep;itloop++) {
        for (ploop=0;ploop<p->npes;ploop++) {
          stddev += (bwmean - bandwith[itloop][ploop][i][j])*(bwmean - bandwith[itloop][ploop][i][j]);
          if (bandwith[itloop][ploop][i][j]>=(OUTLIERLIMIT*bwmax))
            corrstddev += (corrbwmean - bandwith[itloop][ploop][i][j])*(corrbwmean - bandwith[itloop][ploop][i][j]);
        }
      }
      if (bwmean != 0) {
        stddev = (sqrt(stddev/(nrofrep*p->npes)))/bwmean;
        corrstddev = (sqrt(corrstddev/bwcount))/corrbwmean;
      }
      else {
        stddev = 0;
        corrstddev = 0;
      }
      devmean += stddev;
      corrdevmean += corrstddev;
      fprintf(stream,"%8.8f\t",bwmax);
      if (chartrev) i--;
      else i++;
    }
    fprintf(stream,"\n");
  }
  fclose( stream );

  if (nrofrep*p->npes > 1) {
    printf("          %d outliers (values lower than %1.2f*max), out of %d values\n",
      outliers, OUTLIERLIMIT, maxrow*maxcol*nrofrep*p->npes);
    printf("          arithmetic mean of all (percentage) standard deviations: %4.2f %%\n",100*devmean/(maxcol*maxrow));
    printf("          after outlier exclusion: %4.2f %%\n",100*corrdevmean/(maxcol*maxrow));
  }
  else
    printf("          warning: no repetitions, results likely contain badly wrong values\n");
}
;

int __cdecl main(int argc,char *argv[]) {
  chartrev = CHARTREV;
  tic = TIC;
  int i, arg, currit, modes, modenr;
  arg = 1;
  modes = 0;

#ifdef SHAREDMEM
  int shmid;
#endif

  /* default initialisation */
  paramT *p = NULL;
  p = (paramT*) malloc(MXPROC*sizeof(paramT));
  p[0].mode = 0; // load sum
  p[0].npes = NPES; // number of process
  p[0].mxsize = MXRUNSZ; // max size
  p[0].minsize = MINRUNSZ; // min size
  p[0].mxstrds = MXSTRDS; // max number of stride
  p[0].mxiters = MXIT; // max iter
  p[0].currentrep = 1; // repeat
#ifdef HAVEOPT
  p[0].useoptasm = OPTASM;
#else
  p[0].useoptasm = 0;
#endif

  printf("\nECT memperf - Extended Copy Transfer Characterization\n");
  printf("Memory Performance Characterization Toolkit %s\n\n", VERSION);

  if (argc < 2) {
#ifdef HAVEOPT
    printf("Usage: %s -m <mode> [-p] [-s] [-n] [-r] [-i] [-t] [-c] [-a] [-o]\n", argv[0]);
#else
    printf("Usage: %s -m <mode> [-p] [-s] [-n] [-r] [-i] [-t] [-c] [-o]\n", argv[0]);
#endif // HAVEOPT
    printf("       -m <mode>    :   0 = load sum test\n");
    printf("                        1 = const store test\n");
    printf("                        2 = load copy test\n");
    printf("                        3 = copy store test\n");
    printf("                        9 = all tests\n");
    printf("       [-c <nofrep> ]   Default: %d\n", NROFREP);
#ifdef HAVEOPT
    printf("       [-a <optasm> ]   0/1 = use/don't use special instructions\n");
    printf("                        2 = both methods, Default: %d\n", OPTASM);
#endif
    printf("       [-p <nProc>  ]   Default: %d\n", NPES);
    printf("       [-s <mxstrds>]   Default: %d\n", MXSTRDS);
    printf("       [-n <mxsize> ]   Default: %d (%d Bytes)\n", MXRUNSZ, 8<<MXRUNSZ);
    printf("       [-r <minsize>]   Default: %d (%d Bytes)\n", MINRUNSZ, 8<<MINRUNSZ);
    printf("       [-i <mxiters>]   Default: %d\n", MXIT);
    printf("       [-o          ]   Reversed Output\n");
    printf("       [-t <tics/us>]   Default: %.2f\n", tic);
    exit(2);
  }

  while (arg < argc) {
    if (argv[arg][0] =='-') {
      if (argv[arg][1] =='p' && (arg+1)<argc) {
        arg++; p[0].npes = atoi(argv[arg]);
      }
      if (argv[arg][1] =='s' && (arg+1)<argc) {
        arg++; p[0].mxstrds = atoi(argv[arg]);
      }
      if (argv[arg][1] =='m' && (arg+1)<argc) {
        arg++; modes = atoi(argv[arg]);
      }
      if (argv[arg][1] =='n' && (arg+1)<argc) {
        arg++; p[0].mxsize = atoi(argv[arg]);
      }
      if (argv[arg][1] =='r' && (arg+1)<argc) {
        arg++; p[0].minsize = atoi(argv[arg]);
      }
      if (argv[arg][1] =='i' && (arg+1)<argc) {
        arg++; p[0].mxiters = atoi(argv[arg]);
      }
      if (argv[arg][1] =='o') {
        chartrev = 1;
      }
      if (argv[arg][1] =='a' && (arg+1)<argc) {
        arg++;
#ifdef HAVEOPT
        p[0].useoptasm = atoi(argv[arg]);
#endif
      }
      if (argv[arg][1] =='c' && (arg+1)<argc) {
        arg++; nrofrep = atoi(argv[arg]);
      }
      if (argv[arg][1] == 't' && (arg+1)<argc) {
        arg++; tic = atof(argv[arg]);
      }
    }
    arg++;
  }

  p[0].mxsize = 1 << p[0].mxsize;
  p[0].minsize = 1 << p[0].minsize;

  /* set high process priority */
  // setpriority(PRIO_PROCESS, 0,-20);

  printf("Specified/Evaluated Clock Resolution (Option -t): %.2f MHz\n\n", tic);

  /* allocate shared memory (only once for all repetitions/modes */
#ifdef SHAREDMEM
  printf("Allocation of Shared Memory!\n");
  if ((shmid = shmget(IPC_PRIVATE, sizeof(double)*p[0].mxsize, IPC_CREAT | 0666))
    ==-1) {perror("shmget");exit(99);}
  if ((shared = (double *) :(shmid, 0, SHM_RND)) == (double *) -1) {
    perror("shmat\n"); exit(99);
  }
#endif /* SHAREDMEM */

  for (modenr=0; modenr<=3; modenr++) {
    if ((modenr==modes) || (modes==9)) {
      p[0].mode=modenr;
      for (currit=1; currit<=nrofrep; currit++) {
        p[0].currentrep=currit;
        for (i=1; i<MXPROC; i++) {
          memcpy (&p[i], &p[0], sizeof(paramT));
        }

        switch (p[0].mode) {
        case 0:
          printf("Load Sum (%d)    - Working Set: %.0f KByte; Strides: %d\n",currit, (float) p[0].mxsize*8/1024,p[0].mxstrds);
          break;
        case 1:
          printf("Const Store (%d) - Working Set: %.0f KByte; Strides: %d\n",currit, (float) p[0].mxsize*8/1024,p[0].mxstrds);
          break;
        case 2:
          printf("Load Copy (%d)   - Working Set: 2*%.0f KByte; Strides: %d\n",currit, (float) p[0].mxsize*8/1024,p[0].mxstrds);
          break;
        case 3:
          printf("Copy Store (%d)  - Working Set: 2*%.0f KByte; Strides: %d\n",currit, (float) p[0].mxsize*8/1024,p[0].mxstrds);
          break;
        }
        if (p[0].npes==1)
          printf("                  Starting 1 process...\n                  Test running...\n");
        else
          printf("                  Starting %d processes...\n                  Tests running...\n", p[0].npes );

        /* start parallel execution */
        /* init semaphores for barrier */
        sem_init();

        begin_parallel(p[0].npes);
        p[par_cid].par_cid=par_cid;

        /*
        printf("Proc: %d, parcid: %d\n",par_cid,p[par_cid].par_cid);
        printf("Address %d\n",&p[par_cid]);
        */

        memop(&p[par_cid]);
        barrier();
        end_parallel();

        sem_deinit();
      }
    analyze_rep(&p[0]);
    }
  }

  /* detach & destroy shared memory */
#ifdef SHAREDMEM
  if ((shmdt(shared))==-1) {
    perror("shmdt\n"); exit(99);
  }
  if ((shmctl(shmid,IPC_RMID,0))==-1) {
    perror("ShmRelease"); exit(99);
  }
#endif /* SHAREDMEM */

  free(p);
  return 0;
}
