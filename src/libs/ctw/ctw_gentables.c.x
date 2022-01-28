/* ctw_gentables: program to generate the tables used by the CTW algorithm, to make it possible
   to statically link these tables into the program binary instead of calculating them
   each time again. This program outputs two files: ctwmath-tables.h and ctwlarc-tables.h */
#include <stdio.h>
#include <math.h>
#include "ctw-settings.h"
#include "ctwmath.h"	/* only included to get some constants: MAXCOUTNS, LOGENTRIES, JACENTRIES and ACCURACY */
#include "ctwlarc.h"	/* only included to get one constant: ARENTRIES */

#define FN_CTWMATH_TBL "ctwmath-tables.h"
#define FN_CTWLARC_TBL "ctwlarc-tables.h"

/* arrays to store the tables to be calculated. For a description of the purpose of the tables,
   refer to ctwmath.c and ctwlarc.c */
/* ctwmath.c tables */
int CTWlogar[LOGENTRIES];
int CTWzlogpmax[MAXCOUNTS],CTWzlogpmin[MAXCOUNTS];
int CTWjacar[JACENTRIES]; 
/* ctwlarc.c tables */
int ARexp[ARENTRIES];
int ARlog[ARENTRIES];


/* calc_ctwmath_tables generates tables for ctwmath: CTWlogar, CTWzlogpmax, CTWzlogpmin and
   CTWjacar */
void calc_ctwmath_tables(void)
{ int loge,a,jace;
  double pea0,peaplus10;

  /* generate CTWlogar: logarithmic table */
  CTWlogar[loge] = 0;
  for (loge=1; loge<LOGENTRIES; loge++)
    CTWlogar[loge] = ACCURACY*log(loge)/log(2.0) + 0.5;
             
  /* generate CTWzlogpmax and CTWzlogpmin: tables needed for the zero-redundancy estimator */
  CTWzlogpmax[0]=-ACCURACY;
  CTWzlogpmin[0]=-ACCURACY;
  pea0=0.5;
  for (a=1; a<MAXCOUNTS; a++)
    { peaplus10=pea0*(a+0.5)/(a+1.0);
      CTWzlogpmax[a] = ACCURACY*log( (0.5*peaplus10 +0.25)/
                                   (0.5*pea0+0.25)        )/log(2.0) - 0.5;
      CTWzlogpmin[a] = ACCURACY*log( (0.5*(pea0-peaplus10))/
                                   (0.5*pea0+0.25)        )/log(2.0) - 0.5;
      pea0=peaplus10;
    }

  /* generate CTWjacar: jacobian table */
  for (jace=0; jace<JACENTRIES; jace++)
    CTWjacar[jace]= ACCURACY*log(1.0+exp(-log(2.0)*jace/ACCURACY))/log(2.0)
                     + 0.5;
}


/* calc_ctwlarc_tables generates tables for ctwlarc: ARlog and ARexp */
void calc_ctwlarc_tables(void)
{ int are,lastexp,bestare;
  double diff,bestdiff;

  /* calculate ARexp (the exp-table for the arithmetic encoder and decoder */
  for (are=0; are<ARENTRIES-1; are++)
    ARexp[are]=ARENTRIES*exp(-(are+1)*log(2.0)/ARENTRIES)+0.5; 
  ARexp[ARENTRIES-1]=ARENTRIES/2;

  /* calculate ARlog (the log-table for the arithmetic encoder and decoder
     this table is the inverse of ARexp */
  lastexp=ARENTRIES/2;
  bestare=ARENTRIES-1;
  bestdiff= 0.0; 
  for (are=ARENTRIES-2; are>=0; are--)
  { if (ARexp[are]==lastexp)
      { diff=fabs(1.0+ARENTRIES*log((double)lastexp/ARENTRIES)/log(2.0) + are);
        if (diff<bestdiff) 
          { bestare=are;
            bestdiff=diff; } }
    else
      { ARlog[lastexp]=bestare;
        lastexp=ARexp[are];
        bestare=are;
        bestdiff=fabs(are + 1.0 + ARENTRIES*log((double)lastexp/ARENTRIES)/log(2.0));}
  }
  ARlog[lastexp]=bestare;

  for (are=ARENTRIES/2-1; are>=1; are--)
    ARlog[are]=ARlog[2*are]+ARENTRIES;
}


/* write_C_table writes table "tab" with size "size" in C format using name "name" */
write_C_table(FILE *f,int tab[],int size,char *name)
{
  int i,linecount;
  fprintf(f, "int %s = {\n", name);
  fprintf(f, "%6d",tab[0]);
  for (i = linecount = 1; i < size; linecount++, i++)
  {
    fprintf(f,",");
    if (linecount>11) /* after 12 outputted values we start a new line */
    {
      fprintf(f, "\n");
      linecount=0;
    }
    fprintf(f, "%6d",tab[i]);
  }
  fprintf(f, "\n};\n\n");
}


int main(void)
{
  FILE *f;
  
  calc_ctwmath_tables();
  calc_ctwlarc_tables();
 
  printf("Creating %s...\n",FN_CTWMATH_TBL);
  if ((f = fopen(FN_CTWMATH_TBL,"w")) == NULL)
  {
    printf("ctw_gentables: error creating %s\n",FN_CTWMATH_TBL);
    return 1;
  }
  fprintf(f,"/* %s -\n   contains declaration and initialization of all tables needed by ctwmath.c */\n\n",FN_CTWMATH_TBL);
  write_C_table(f,CTWlogar,LOGENTRIES,"CTWlogar[LOGENTRIES]");
  write_C_table(f,CTWzlogpmax,MAXCOUNTS,"CTWzlogpmax[MAXCOUNTS]");
  write_C_table(f,CTWzlogpmin,MAXCOUNTS,"CTWzlogpmin[MAXCOUNTS]");
  write_C_table(f,CTWjacar,JACENTRIES,"CTWjacar[JACENTRIES]");
  fclose(f);

  printf("Creating %s...\n",FN_CTWLARC_TBL);
  if ((f = fopen(FN_CTWLARC_TBL,"w")) == NULL)
  {
    printf("ctw_gentables: error creating %s\n",FN_CTWLARC_TBL);
    return 1;
  }
  fprintf(f,"/* %s -\n   contains declaration and initialization of all tables needed by ctwlarc.c */\n\n",FN_CTWLARC_TBL);
  write_C_table(f,ARexp,ARENTRIES,"ARexp[ARENTRIES]");
  write_C_table(f,ARlog,ARENTRIES,"ARlog[ARENTRIES]");
  fclose(f);
  
  printf("Done.\n");
}