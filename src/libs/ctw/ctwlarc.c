/* preprocessor commands ==================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ctw-settings.h"
#include "ctwlarc.h"

/* local function prototypes (not required) ================================================= */
/* explanation of these functions can be found below */
void    pushinit();
void    pushbit(boolean bit);
void    pushblk(int blk, int bitsinblk);
void    pushexit(void);
int     pushedbits(void);
void    pullinit();
boolean pullbit(void);
int     pullblk(int bitsinblk);
void    pullexit(void);
int     pulledbits(void);

/* variables ================================================================================ */
/* for push and pull routines ============================================================ */
/* pu_ means push for push routines and pull for pull routines */

unsigned char *Pu_FileEnd;
unsigned char *Pu_File;

int ctw_fwrite(unsigned char *buff, int len, int num, unsigned char *unused) {
    if(Pu_File >= Pu_FileEnd) return(-1);
    len *= num;
    if((Pu_File + len) > Pu_FileEnd) len = Pu_FileEnd - Pu_File;
    memcpy(Pu_File, buff, len);
    Pu_File += len;
    return(len);
}
int ctw_fread(unsigned char *buff, int len, int num, unsigned char *unused) {
    if(Pu_File >= Pu_FileEnd) return(-1);
    len *= num;
    if((Pu_File + len) > Pu_FileEnd) len = Pu_FileEnd - Pu_File;
    memcpy(buff, Pu_File, len);
    Pu_File += len;
    return(len);
}

unsigned int  pu_slack;
int           notpu_edbits;
int           notpu_edbytes;
int           pu_edbufs;
unsigned char Pu_buf[PPBUFSIZ];
/* for arithm. cod. routines ============================================================= */
int           delayreg, accum, intpntr, coddelayreg, codaccum;
byte          skipbits; /* used because the first DELAYREGSIZE bits of the delayreg are
                           always zero at the beginning of the encoding process */

/* the following tables are declared and initialized in ctwlarc-tables.h:
   int           ARexp[ARENTRIES];
   int           ARlog[ARENTRIES]; */
#include "ctwlarc-tables.h"


/* functions ================================================================================ */

/* for push routines ======================================================================== */
/* Initialize the push routines. A file is opened and all variables are initialized. */
void pushinit() {
  pu_edbufs = 0;
  notpu_edbytes = PPBUFSIZ;
  notpu_edbits = 8;
  pu_slack = 0;
  skipbits = DELAYREGSIZE;
}

/* Push one bit. These bits are collected in pu_slack until a whole byte is pushed and then
   this byte is copied to Pu_buf. When Pu_buf is full, the contents are written to a file. */
void pushbit(boolean  bit) {
  if (skipbits)
    skipbits--;
  else {
    if (bit)
      pu_slack = (pu_slack << 1) | 1;
    else
      pu_slack <<= 1;
    notpu_edbits--;
    if(notpu_edbits == 0) {
      Pu_buf[PPBUFSIZ - notpu_edbytes] = pu_slack & 0x0ff;
      notpu_edbytes--;
      if(notpu_edbytes == 0) {
        ctw_fwrite(Pu_buf, sizeof(unsigned char), PPBUFSIZ, Pu_File);
        pu_edbufs++;
        notpu_edbytes = PPBUFSIZ;
      }
      notpu_edbits = 8;
      pu_slack = 0;
    }
  }
}

/* Push a block of bitsinblk bits. These bits are collected in pu_slack and then every whole
   byte is copied to Pu_buf. When Pu_buf is full, the contents are written to a file. */
void pushblk(int blk, int bitsinblk) {
  unsigned char byt;

  if (skipbits)
    if (bitsinblk <= skipbits) {
      skipbits -= bitsinblk;
      return;
    }
    else {
      bitsinblk -= skipbits;
      skipbits = 0;
    }
  pu_slack = (pu_slack << bitsinblk) | blk;
  notpu_edbits = notpu_edbits - bitsinblk;
  while(notpu_edbits <= 0) {
    byt = pu_slack >> (-notpu_edbits);
    Pu_buf[PPBUFSIZ - notpu_edbytes] = byt;
    notpu_edbytes--;
    if(notpu_edbytes == 0) {
      ctw_fwrite(Pu_buf, sizeof(unsigned char), PPBUFSIZ, Pu_File);
      pu_edbufs++;
      notpu_edbytes = PPBUFSIZ;
    }
    pu_slack = pu_slack ^ (byt << (-notpu_edbits));
    notpu_edbits = notpu_edbits + 8;
  }
}

/* Copy the remaining bits in pu_slack to Pu_buf. Incomplete bytes are extended with zeros.
   Then copy the contents of Pu_buf to a file and close this file. */
void pushexit(void) {
  if(notpu_edbits < 8) {
    Pu_buf[PPBUFSIZ - notpu_edbytes] = (pu_slack << notpu_edbits) & 0x0ff;
    notpu_edbytes--;
    notpu_edbits = notpu_edbits + 8;
  }
  ctw_fwrite(Pu_buf, sizeof(unsigned char), PPBUFSIZ - notpu_edbytes, Pu_File);
}

/* Return the total number of bits that has been pushed. Bits that are not written to a file yet
   are included in this count. */
int pushedbits(void) {
  return((pu_edbufs * PPBUFSIZ + PPBUFSIZ - notpu_edbytes) * 8 + (8 - notpu_edbits));
}

/* for pull routines ======================================================================== */
/* Initialize the pull routines. A file is opened and all variables are initialized. */
void pullinit() {
  pu_edbufs = 0;
  notpu_edbytes = 0;
  notpu_edbits = 0;
  pu_slack = 0;
}

/* Pull one bit. These bits come from pu_slack and if pu_slack is empty a whole byte is read
   from Pu_buf. If Pu_buf is empty PPBUFSIZ bytes are read from a file. */
boolean pullbit(void) {
  boolean bit;

  if(notpu_edbits == 0) {
    if(notpu_edbytes == 0) {
      ctw_fread(Pu_buf, sizeof(unsigned char), PPBUFSIZ, Pu_File);
      pu_edbufs++;
      notpu_edbytes = PPBUFSIZ;
    }
    pu_slack = Pu_buf[PPBUFSIZ - notpu_edbytes];
    notpu_edbytes--;
    notpu_edbits = 8;
  }
  notpu_edbits--;
  bit = (pu_slack >> notpu_edbits) & 1;
  pu_slack = pu_slack ^ (bit << notpu_edbits);
  return(bit);
}

/* Pull a block of bitsinblk bits. These bits come from pu_slack and if pu_slack is empty a
   whole byte is read from Pu_buf. If Pu_buf is empty PPBUFSIZ bytes are read from a file. */
int pullblk(int bitsinblk) {
  int blk;

  notpu_edbits = notpu_edbits - bitsinblk;
  while(notpu_edbits < 0) {
    if(notpu_edbytes == 0) {
      ctw_fread(Pu_buf, sizeof(unsigned char), PPBUFSIZ, Pu_File);
      pu_edbufs++;
      notpu_edbytes = PPBUFSIZ;
    }
    pu_slack = (pu_slack << 8) | (Pu_buf[PPBUFSIZ-notpu_edbytes]);
    notpu_edbytes--;
    notpu_edbits = notpu_edbits + 8;
  }
  blk = pu_slack >> notpu_edbits;
  pu_slack = pu_slack ^ (blk << notpu_edbits);
  return(blk);
}

/* Return the total number of bits that has been pulled. Bits that are read from a file, but are
   not pulled yet, are not included in this count. */
int pulledbits(void) {
  return((pu_edbufs * PPBUFSIZ - notpu_edbytes) * 8 - notpu_edbits);
}



/* for arithmetic encoding routines ========================================================= */
/* more information on aritmetic encoding routines can be found in [EIDMA] par 6.5.1 */

/* arencinit: initialize arithmetic encoder. Encoder will write to CodeFile */
void arencinit(unsigned char *CodeFile, unsigned int CodeSize)
{
  Pu_File = CodeFile;
  Pu_FileEnd = CodeFile + CodeSize;
  delayreg = 0;
  accum = 0;
  intpntr = 0;
  pushinit();
}

/* arenc: encodes bit symbsmall. instep is a measure of the logarithm of the probability of 
   the most probable symbol */
void arenc(int instep, boolean symbsmall)
{ int shifts,blk,bit;
  int bigpntr;

  if (intpntr >= ARENTRIES)
  { shifts = intpntr / ARENTRIES;
    
    blk = accum >> (ACCUMSIZE - shifts);
    accum = (accum << shifts) & ACCUMMASK;
    pushblk(delayreg >> (DELAYREGSIZE - shifts), shifts );
    delayreg = ((delayreg << shifts) & DELAYREGMASK) | blk;

    intpntr = intpntr - shifts * ARENTRIES; 
  }
  
  while (delayreg == DELAYREGMASK)
  { bit = accum >> (ACCUMSIZE - 1);
    accum = (accum << 1) & ACCUMMASK;
    pushbit(delayreg >> (DELAYREGSIZE - 1)); 
    delayreg = ((delayreg << 1) & DELAYREGMASK) | bit;
  }

  bigpntr = intpntr + instep;
  if (symbsmall)
  { if (bigpntr < ARENTRIES)
    { accum = accum + 2 * ARexp[bigpntr];
      if (accum > ACCUMMASK) 
      { accum = accum & ACCUMMASK;
        delayreg = delayreg + 1;
      }
      intpntr = ARlog[ARexp[intpntr] - ARexp[bigpntr]];
    }
    else
    { bigpntr = bigpntr - ARENTRIES;
      accum = accum + ARexp[bigpntr];
      if (accum > ACCUMMASK)
      { accum = accum & ACCUMMASK;
        delayreg = delayreg + 1;
      }
      intpntr = ARlog[2 * ARexp[intpntr] - ARexp[bigpntr]] + ARENTRIES;
    }
  }
  else intpntr = bigpntr;
}

/* arencexit: terminates the encoding process. Bits in the accumulator and delay register are
   stored and the size of the encoded file in bits is returned */
void arencexit(unsigned int *codelength)
{ pushblk(delayreg, DELAYREGSIZE);
  pushblk(accum, ACCUMSIZE); 
  *codelength = pushedbits();
  pushexit();
}

/* for arithmetic decoding routines ========================================================= */
/* more information on aritmetic decoding routines can be found in [EIDMA] par 6.5.2 */

/* ardecinit: initialize arithmetic decoder. Decoder will read from CodeFile */
void ardecinit(unsigned char *CodeFile, unsigned int CodeSize)
{ accum = 0;
  intpntr = 0;
  Pu_File = CodeFile;
  Pu_FileEnd = CodeFile + CodeSize;
  pullinit();
  codaccum = pullblk(ACCUMSIZE);
  delayreg = coddelayreg = 0; 
}

/* ardec: decodes one bit, using instep as measure of the logarithm of the probability of the
   most probable symbol */
boolean ardec(int instep)
{ int shifts, blk, codblk, bit, codbit;
  int thrdelayreg, thraccum;
  int bigpntr;
  boolean symbsmall;

  if (intpntr >= ARENTRIES)
  { shifts = intpntr / ARENTRIES;

    blk = accum >> (ACCUMSIZE - shifts);
    accum = (accum << shifts) & ACCUMMASK;
    delayreg = ((delayreg << shifts) & DELAYREGMASK) | blk;

    codblk = codaccum >> (ACCUMSIZE - shifts);
    codaccum = ((codaccum << shifts) & ACCUMMASK) | pullblk(shifts);
    coddelayreg = ((coddelayreg << shifts) & DELAYREGMASK) | codblk;
 
    intpntr = intpntr - shifts*ARENTRIES;
  }

  while (delayreg == DELAYREGMASK) 
  { bit = accum >> (ACCUMSIZE - 1);
    accum = (accum << 1) & ACCUMMASK;
    delayreg = ((delayreg << 1) & DELAYREGMASK) | bit; 

    codbit = codaccum >> (ACCUMSIZE - 1);
    codaccum = ((codaccum << 1) & ACCUMMASK) | pullbit();
    coddelayreg = ((coddelayreg << 1) & DELAYREGMASK) | codbit; 
  }

  bigpntr = intpntr + instep;
 
  if (bigpntr < ARENTRIES)
  { thraccum = accum + 2 * ARexp[bigpntr];
    thrdelayreg = delayreg;  
    if (thraccum > ACCUMMASK)
    { thraccum = thraccum & ACCUMMASK;
      thrdelayreg = thrdelayreg + 1;
    } 

    symbsmall = (coddelayreg > thrdelayreg) || 
                ((coddelayreg == thrdelayreg) && (codaccum >= thraccum));

    if (symbsmall) 
    { accum = thraccum;
      delayreg = thrdelayreg;  
      intpntr = ARlog[ARexp[intpntr] - ARexp[bigpntr]];
    }
    else intpntr = bigpntr;
  }
  else 
  { bigpntr = bigpntr - ARENTRIES;
    thraccum = accum + ARexp[bigpntr];
    thrdelayreg = delayreg; 
    if (thraccum > ACCUMMASK)
    { thraccum = thraccum & ACCUMMASK;
      thrdelayreg = thrdelayreg + 1;
    } 

    symbsmall = (coddelayreg > thrdelayreg) ||
                ((coddelayreg == thrdelayreg) && (codaccum >= thraccum));
      
    if (symbsmall)
    { accum = thraccum;
      delayreg = thrdelayreg;
      intpntr = ARlog[2 * ARexp[intpntr] - ARexp[bigpntr]] + ARENTRIES;
    }
    else intpntr = bigpntr + ARENTRIES;
  }
  return symbsmall;
}

/* ardecexit: terminates the decoding process. Size of the compressed file in bits is returned */
void ardecexit(unsigned int *codelength)
{ 
  *codelength = pulledbits();
}