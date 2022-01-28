/* ctwmath: contains the data constants, type definitions and functions used for the mathematics
   in the CTW algorithm. These functions are used by the encoder and decoder in CTWENCDEC. */

/* preprocessor commands ================================================== */ 
#include <stdio.h>
#include <stdlib.h>
#include "ctw-settings.h"
#include "ctwmath.h"

/* global constants ======================================================== */
struct CTWRecord CTW_DATA_ZERO = { {0,0}, 0};

/* variables =============================================================== */
/* for ctw-arithm. routines =============================================
   The following tables are declared and initialized in ctwmath-tables.h:
   int CTWlogar[LOGENTRIES]:
   	contains logarithms (base 2) of values 0 <= i < LOGENTRIES, log values in
   	table are multiplied by ACCURACY, to get the desired accuracy.
   int CTWzlogpmax[MAXCOUNTS],CTWzlogpmin[MAXCOUNTS]:
   	needed for zero-redundancy estimator: when a sequence containing only zeros or ones
   	occurs, we use these tables to find the log of the smallest and largest conditional
   	zero-redundancy estimated probability [EIDMA par 5.5]
   int CTWjacar[JACENTRIES]:
   	contains jacobian logarithms, defined as J = log(1 + 2^x), used to add probabilities
   	that are presented logarithmic.
   	Because we use integer values, values are multiplied by ACCURACY:
   	so CTWjacar[i] = round(ACCURACY * log(1 + 2^(-i/ACCURACY)))  (log has base 2) */
#include "ctwmath-tables.h"

/* for ctw-arithm. routines ================================================ */


/* CTWjac is a local function that calculates the jacobian logarithm using CTWjacar.
   Valid range for ent: ent <= 0 */
int CTWjac(int ent)
{ if (-ent<JACENTRIES)
    return CTWjacar[-ent];
  else 
    /* values of -ent >= JACENTRIES are not in table because they are almost equal to zero */
    return 0;
}


struct CTWRecord CTW_data_from_one_count(unsigned int bit) {
  struct CTWRecord temp;

  temp.cnt[bit] = 1;
  temp.cnt[1-bit] = 0;
  temp.logbeta = 0;
  return(temp);
}


void CTWProcess(int curdepth, struct CTWRecord ctwinfo[],
                struct CTWProb *ctwprob,
                struct CTWRecord dummy0info[],
                struct CTWRecord dummy1info[])
{ int c0,c1,loggamma;
  int logpw0,logpw1;
  int logpe0,logpe1,logcsumplusacc;
  int logbetatimespe0,logbetatimespe1,diff0,diff1;
  int nom,den;
  int logbetanew0,logbetanew1;
  int enddepth;
  struct CTWRecord data;   

  /* start with deepest node (should be a leaf) */
  curdepth--;
  data = ctwinfo[curdepth];
  c0 = data.cnt[0];
  c1 = data.cnt[1];
  /* loggamma represents the log of the quotient of the weighted conditional probabilities
     of zero and one [EIDMA par. 5.2, where eta is used instead of gamma!]. We use the
     zero-redundancy estimator [EIDMA par 5.5] or the KT-estimator [EIDMA par 3.5] to get
     this quotient for this node */
  if (settings.use_zeroredundancy && (c1==0 || c0==0))
    /* "special cases" of the zero redundancy estimator (c1 or c0 is zero) */
    if (c1==0)
      loggamma=CTWzlogpmax[c0]-CTWzlogpmin[c0];
    else /* otherwise, c0 must be 0 */
      loggamma=CTWzlogpmin[c1]-CTWzlogpmax[c1];
  else
    /* Krichevski-Trofimov-estimator, or zero redundancy if no "special case" occured */
    loggamma = CTWlogar[2*c0 + 1] - CTWlogar[2*c1 + 1];
  
  /* update counts for dummy0 and dummy1; if one count exceeds range (must be < MAXCOUNTS,
     [EIDMA par 5.3]) both counts are divided by 2 */
  if (c0==(MAXCOUNTS-1))
    { dummy0info[curdepth].cnt[0] = MAXCOUNTS/2;
      dummy0info[curdepth].cnt[1] = (c1+1)/2; }
  else 
    { dummy0info[curdepth].cnt[0] = c0+1;
      dummy0info[curdepth].cnt[1] = c1; }

  if (c1==(MAXCOUNTS-1))
    { dummy1info[curdepth].cnt[0] = (c0+1)/2;
      dummy1info[curdepth].cnt[1] = MAXCOUNTS/2; }
  else 
    { dummy1info[curdepth].cnt[0] = c0;
      dummy1info[curdepth].cnt[1] = c1+1; }

  /* we don't do anything with logbeta here */
  dummy0info[curdepth].logbeta = data.logbeta;
  dummy1info[curdepth].logbeta = data.logbeta;

  /* if rootweighting is enabled the weighting also takes place for the root node */
  if (settings.rootweighting) enddepth = 0; else enddepth = 1;
  
  while (curdepth-- > enddepth)
  { 
    data = ctwinfo[curdepth];
    /* calculate the logs of the "incoming" conditional weighted probabilites from the
       incoming loggamma, using equations: loggamma = logpw0 - logpw1 and log(pw0+pw1) = 0 
       [EIDMA 5.2 and 5.3, eq 65 and 71] */
    if (loggamma>=0)
      { logpw0 = - CTWjac(-loggamma);
        logpw1 = logpw0  - loggamma;  }
    else 
      { logpw1 = - CTWjac(loggamma);
        logpw0 = logpw1 + loggamma;   }

    /* calculate logs of conditional estimated probabilities from the counts in this node,
       using the zero redundancy estimator [EIDMA par 5.5] or the KT-estimator [EIDMA par 3.5] */
    c0 = data.cnt[0];
    c1 = data.cnt[1];
    if (settings.use_zeroredundancy && (c1==0 || c0==0))
      /* "special cases" of the zero redundancy estimator (c1 or c0 is zero) */
      if (c1==0) {
        logpe0 = CTWzlogpmax[c0];
        logpe1 = CTWzlogpmin[c0]; }
      else { /* otherwise, c0 must be 0 */
        logpe0 = CTWzlogpmin[c1];
        logpe1 = CTWzlogpmax[c1]; }
    else {
      /* Krichevski-Trofimov-estimator or zero redundancy if no "special case" occured
         calculate logpe0 and logpe1 according to [EIDMA eq (72)] */
      logcsumplusacc = CTWlogar[c0 + c1 + 1] + ACCURACY;
      logpe0 = CTWlogar[2*c0 + 1] - logcsumplusacc;
      logpe1 = CTWlogar[2*c1 + 1] - logcsumplusacc; 
    }

    /* calculate new value of loggamma using [EIDMA eq (67) and eq (71)] */
    logbetatimespe0 = data.logbeta + logpe0;
    diff0 =  logbetatimespe0 - logpw0;
    if (diff0>=0)
      nom = logbetatimespe0 + CTWjac(-diff0);
    else
      nom = logpw0 + CTWjac(diff0);

    logbetatimespe1 = data.logbeta + logpe1;
    diff1 =  logbetatimespe1 - logpw1;
    if (diff1>=0)
      den = logbetatimespe1 + CTWjac(-diff1);
    else
      den = logpw1 + CTWjac(diff1);

    loggamma = nom - den;  

    /* update counts for dummy0 and dummy1 */
    if (c0==(MAXCOUNTS-1))
    { dummy0info[curdepth].cnt[0] = MAXCOUNTS/2;
      dummy0info[curdepth].cnt[1] = (c1+1)/2; }
    else 
    { dummy0info[curdepth].cnt[0] = c0+1;
      dummy0info[curdepth].cnt[1] = c1; }

    if (c1==(MAXCOUNTS-1))
    { dummy1info[curdepth].cnt[0] = (c0+1)/2;
      dummy1info[curdepth].cnt[1] = MAXCOUNTS/2; }
    else 
    { dummy1info[curdepth].cnt[0] = c0;
      dummy1info[curdepth].cnt[1] = c1+1; }

    /* calcultate logbeta's for dummy0 and dummy1 using [EIDMA eq (69)] */
    logbetanew0 = logbetatimespe0-logpw0;
    if (logbetanew0> settings.maxlogbeta) logbetanew0= settings.maxlogbeta;
    if (logbetanew0<-settings.maxlogbeta) logbetanew0=-settings.maxlogbeta;
    dummy0info[curdepth].logbeta = logbetanew0;
 
    logbetanew1 = logbetatimespe1-logpw1;
    if (logbetanew1> settings.maxlogbeta) logbetanew1= settings.maxlogbeta;
    if (logbetanew1<-settings.maxlogbeta) logbetanew1=-settings.maxlogbeta;
    dummy1info[curdepth].logbeta = logbetanew1;
  }   

  /* put the weighted probability on the most probable symbol (in the root of the CTW tree)
     and the most probable symbol in ctwprob */
  if (loggamma>=0) 
  { ctwprob->logpwroot = - CTWjac(-loggamma);
    ctwprob->predsymb = 0; }
  else 
  { ctwprob->logpwroot = - CTWjac(loggamma);
    ctwprob->predsymb = 1; }
}
