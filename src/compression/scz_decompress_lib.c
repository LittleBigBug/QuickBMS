/****************************************************************************/ 
/* SCZ_Decompress_lib.c - Decompress what SCZ_Compress.c compresses.
   Simple decompression algorithms for SCZ compressed data.

   This file contains the following user-callable routines:
     Scz_Decompress_File( *infilename, *outfilename );
     Scz_Decompress_File2Buffer( *infilename, **outbuffer, *M );
     Scz_Decompress_Buffer2Buffer( *inbuffer, *N, **outbuffer, *M );
  See below for formal definitions and comments.

 SCZ_Compress - LGPL License:
  Copyright (C) 2001, Carl Kindman
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  For updates/info, see:  http://sourceforge.net/projects/scz-compress/

  Carl Kindman 11-21-2004     carl_kindman@yahoo.com
                7-5-2005        Added checksums and blocking.
		9-14-2006	Freeing fixes by David Senterfitt.
*****************************************************************************/

#ifndef SCZ_DECOMPRESSLIB_DEFD
#define SCZ_DECOMPRESSLIB_DEFD

#include "scz_core.h"


/****************************************************************/
/* Decompress - Decompress a buffer.  Returns 1 on success, 0 	*/
/*  if failure.							*/
/****************************************************************/
int Scz_Decompress_Seg( struct scz_item **buffer0_hd )
{
 unsigned char forcingchar, marker[257], phrase[256][256];
 int j, nreplaced, iterations, iter, markerlist[256];
 struct scz_item *bufpt, *tmpbuf, *oldptr;
 unsigned char ch;

 /* Expect magic number(s). */
 bufpt = *buffer0_hd;
 if ((bufpt==0) || (bufpt->ch!=101))
  {printf("Error1a: This does not look like a compressed file.\n"); return 0;}
 oldptr = bufpt;   bufpt = bufpt->nxt;   sczfree(oldptr);
 if ((bufpt==0) || (bufpt->ch!=98))
  {printf("Error2a: This does not look like a compressed file.\n");  return 0;}
 oldptr = bufpt;   bufpt = bufpt->nxt;    sczfree(oldptr);

 /* Get the compression iterations count. */
 iterations = bufpt->ch;
 oldptr = bufpt;   bufpt = bufpt->nxt;   sczfree(oldptr);
 *buffer0_hd = bufpt;

 for (iter=0; iter<iterations; iter++)
  { /*iter*/
    bufpt = *buffer0_hd;
    forcingchar = bufpt->ch;	oldptr = bufpt;  bufpt = bufpt->nxt;  sczfree(oldptr);
    nreplaced = bufpt->ch;	oldptr = bufpt;  bufpt = bufpt->nxt;  sczfree(oldptr);
    for (j=0; j<nreplaced; j++)   /* Accept the markers and phrases. */
     {
      marker[j] =    bufpt->ch;	  oldptr = bufpt;  bufpt = bufpt->nxt;  sczfree(oldptr);
      phrase[j][0] = bufpt->ch;	  oldptr = bufpt;  bufpt = bufpt->nxt;  sczfree(oldptr);
      phrase[j][1] = bufpt->ch;	  oldptr = bufpt;  bufpt = bufpt->nxt;  sczfree(oldptr);
     }
    ch = bufpt->ch; 		oldptr = bufpt;  bufpt = bufpt->nxt;  sczfree(oldptr);
    if (ch!=91) /* Boundary marker. */
     {
      printf("Error3: Corrupted compressed file. (%d)\n", ch); 
      return 0;
     }

    for (j=0; j!=256; j++) markerlist[j] = nreplaced;
    for (j=0; j!=nreplaced; j++) markerlist[ marker[j] ] = j;


    /* Remove the header. */
    *buffer0_hd = bufpt;

   /* Replace chars. */
   if (nreplaced>0)
    while (bufpt!=0)
    {
     if (bufpt->ch == forcingchar)
      {
	bufpt->ch = bufpt->nxt->ch;	/* Remove the forcing char. */
	oldptr = bufpt->nxt;   bufpt->nxt = bufpt->nxt->nxt;   sczfree(oldptr);
	bufpt = bufpt->nxt;
      }
     else
      { /* Check for match to marker characters. */
       j = markerlist[ bufpt->ch ];
       if (j<nreplaced)
        {	/* If match, insert the phrase. */
  	  bufpt->ch = phrase[j][0];
          tmpbuf = new_scz_item();
          tmpbuf->ch = phrase[j][1];
          tmpbuf->nxt = bufpt->nxt;
          bufpt->nxt = tmpbuf;
          bufpt = tmpbuf->nxt;
        }
       else bufpt = bufpt->nxt;
      }
    }
  } /*iter*/
 return 1;
}



/************************************************************************/
/* Scz_Decompress_File - Decompresses input file to output file.        */
/*  First argument is input file name.  Second argument is output file  */
/*  name.  The two file names must be different.                        */
/*                                                                      */
/************************************************************************/
int Scz_Decompress_File( char *infilename, char *outfilename )
{
 int k, success, sz1=0, sz2=0, buflen, continuation;
 unsigned char ch, chksum, chksum0;
 struct scz_item *buffer0_hd, *buffer0_tl, *bufpt, *bufprv;
 FILE *infile=0, *outfile;

 infile = fopen(infilename,"rb");
 if (infile==0) {printf("ERROR: Cannot open input file '%s'.  Exiting\n", infilename);  return 0;}

 printf("\n Writing output to file %s\n", outfilename);
 outfile = fopen(outfilename,"wb");
 if (outfile==0) {printf("ERROR: Cannot open output file '%s' for writing.  Exiting\n", outfilename); return 0;}

 do 
  { /*Segment*/

   /* Read file segment into linked list for expansion. */
   /* Get the 6-byte header to find the number of chars to read in this segment. */
   /*  (magic number (101), magic number (98), iter-count, seg-size (MBS), seg-size, seg-size (LSB). */ 
   buffer0_hd = 0;  buffer0_tl = 0;  bufprv = 0;

   ch = getc(infile);   sz1++;		/* Byte 1, expect magic numeral 101. */
   if (feof(infile)) printf("Premature eof\n");
   if (ch!=101) {printf("Error1: This does not look like a compressed file. %d\n", ch); return 0;}
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = getc(infile);  sz1++;		/* Byte 2, expect magic numeral 98. */
   if ((feof(infile)) || (ch!=98)) {printf("Error2: This does not look like a compressed file. (%d)\n", ch); return 0;}
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = getc(infile);  sz1++;		/* Byte 3, iteration-count. */
   if (feof(infile)) {printf("Error3: This does not look like a compressed file.\n"); return 0;}
   bufprv = buffer0_tl;
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = getc(infile);			/* Byte 4, MSB of segment buffer length. */
   if (feof(infile)) {printf("Error3: This does not look like a compressed file.\n"); return 0;}
   buflen = ch << 16;
   ch = getc(infile);			/* Byte 5, middle byte of segment buffer length. */
   if (feof(infile)) {printf("Error3: This does not look like a compressed file.\n"); return 0;}
   buflen = (ch << 8) | buflen;
   ch = getc(infile);			/* Byte 6, LSB of segment buffer length. */
   if (feof(infile)) {printf("Error3: This does not look like a compressed file.\n"); return 0;}
   buflen = ch | buflen;

   k = 0;
   ch = getc(infile);
   while ((!feof(infile)) && (k<buflen))
    {
     bufprv = buffer0_tl;
     scz_add_item( &buffer0_hd, &buffer0_tl, ch );
     sz1++;  k++;
     ch = getc(infile);
    }

   chksum0 = ch;
   ch = getc(infile);
   // printf("End ch = '%c'\n", ch);
   if (k<buflen) {printf("Error: Unexpectedly short file.\n");}
   /* Decode the 'end-marker'. */
   if (ch==']') continuation = 0;
   else
   if (ch=='[') continuation = 1;
   else {printf("Error4: Reading compressed file. (%d)\n", ch);  return 0; }

   success = Scz_Decompress_Seg( &buffer0_hd );	/* Decompress the buffer !!! */
   if (!success) return 0;

   /* Write the decompressed file out. */
   chksum = 0;
   bufpt = buffer0_hd;
   while (bufpt!=0)
    {
     fputc( bufpt->ch, outfile );
     sz2++;   chksum += bufpt->ch;
     bufpt = bufpt->nxt;
    }
   if (chksum != chksum0) {printf("Error: Checksum mismatch (%dvs%d)\n", chksum, chksum0);}

  } /*Segment*/
 while (continuation);

 fclose(infile);
 fclose(outfile);
 printf("Decompression ratio = %g\n", (float)sz2 / (float)sz1 );
 return 1;
}



/***************************************************************************/
/* Scz_Decompress_File2Buffer - Decompresses input file to output buffer.  */
/*  This routine is handy for applications wishing to read compressed data */
/*  directly from disk and to uncompress the input directly while reading  */
/*  from the disk.             						   */
/*  First argument is input file name.  Second argument is output buffer   */
/*  with return variable for array length. 				   */
/*  This routine allocates the output array and passes back the size.      */ 
/*                                                                         */
/**************************************************************************/
int Scz_Decompress_File2Buffer( char *infilename, char **outbuffer, int *M )
{
 int k, success, sz1=0, sz2=0, buflen, continuation, totalin=0, totalout=0;
 unsigned char ch, chksum, chksum0;
 struct scz_item *buffer0_hd, *buffer0_tl, *bufpt, *bufprv, *sumlst_hd=0, *sumlst_tl=0;
 FILE *infile=0;

 infile = fopen(infilename,"rb");
 if (infile==0) {printf("ERROR: Cannot open input file '%s'.  Exiting\n", infilename);  return 0;}

 do 
  { /*Segment*/

   /* Read file segment into linked list for expansion. */
   /* Get the 6-byte header to find the number of chars to read in this segment. */
   /*  (magic number (101), magic number (98), iter-count, seg-size (MBS), seg-size, seg-size (LSB). */ 
   buffer0_hd = 0;  buffer0_tl = 0;

   ch = getc(infile);   sz1++;		/* Byte 1, expect magic numeral 101. */
   if ((feof(infile)) || (ch!=101)) {printf("Error1: This does not look like a compressed file.\n"); return 0;}
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = getc(infile);  sz1++;		/* Byte 2, expect magic numeral 98. */
   if ((feof(infile)) || (ch!=98)) {printf("Error2: This does not look like a compressed file. (%d)\n", ch); return 0;}
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = getc(infile);  sz1++;		/* Byte 3, iteration-count. */
   if (feof(infile)) {printf("Error3: This does not look like a compressed file.\n"); return 0;}
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = getc(infile);			/* Byte 4, MSB of segment buffer length. */
   if (feof(infile)) {printf("Error3: This does not look like a compressed file.\n"); return 0;}
   buflen = ch << 16;
   ch = getc(infile);			/* Byte 5, middle byte of segment buffer length. */
   if (feof(infile)) {printf("Error3: This does not look like a compressed file.\n"); return 0;}
   buflen = (ch << 8) | buflen;
   ch = getc(infile);			/* Byte 6, LSB of segment buffer length. */
   if (feof(infile)) {printf("Error3: This does not look like a compressed file.\n"); return 0;}
   buflen = ch | buflen;
 
   k = 0;
   ch = getc(infile);
   while ((!feof(infile)) && (k<buflen))
    {
     scz_add_item( &buffer0_hd, &buffer0_tl, ch );
     sz1++;  k++;
     ch = getc(infile);
    }

   chksum0 = ch;
   ch = getc(infile);
   // printf("End ch = '%c'\n", ch);
 
   if (k<buflen) {printf("Error: Unexpectedly short file.\n");}
   totalin = totalin + sz1 + 4;		/* (+4, because chksum+3buflen chars not counted above.) */
   /* Decode the 'end-marker'. */
   if (ch==']') continuation = 0;
   else
   if (ch=='[') continuation = 1;
   else {printf("Error4: Reading compressed file. (%d)\n", ch);  return 0; }

   success = Scz_Decompress_Seg( &buffer0_hd );	/* Decompress the buffer !!! */
   if (!success) return 0;

   /* Check checksum and sum length. */
   sz2 = 0;       /* Get buffer size. */
   bufpt = buffer0_hd;
   chksum = 0;
   bufprv = 0;
   while (bufpt!=0)
    {
     chksum += bufpt->ch;
     sz2++;  bufprv = bufpt;
     bufpt = bufpt->nxt;
    }
   if (chksum != chksum0) {printf("Error: Checksum mismatch (%dvs%d)\n", chksum, chksum0);}

   /* Attach to tail of main list. */
   totalout = totalout + sz2;
   if (sumlst_hd==0) sumlst_hd = buffer0_hd;
   else sumlst_tl->nxt = buffer0_hd;
   sumlst_tl = bufprv;

  } /*Segment*/
 while (continuation);

 /* Convert list into buffer. */
 *outbuffer = (char *)malloc((totalout)*sizeof(char));
 sz2 = 0;
 while (sumlst_hd!=0)
  {
   bufpt = sumlst_hd;
   (*outbuffer)[sz2++] = bufpt->ch;
   sumlst_hd = bufpt->nxt;
   sczfree(bufpt);
  }
 if (sz2 > totalout) printf("Unexpected overrun error\n");

 *M = totalout;

 fclose(infile);
 printf("Decompression ratio = %g\n", (float)totalout / (float)totalin );
 return 1;
}





/*******************************************************************************/
/* Scz_Decompress_Buffer2Buffer - Decompresses input buffer to output buffer.  */
/*  This routine is handy for applications wishing to decompress data on the   */
/*  fly, perhaps to reduce dynamic memory or network traffic between other     */
/*  applications.                         				       */
/*  First argument is input array, second is the array's length, the third is  */
/*  the output buffer array, with return variable for array length.  	       */
/*  from the disk.             						       */
/*  This routine allocates the output array and passes back the size.          */ 
/*                                                                             */
/*******************************************************************************/
int Scz_Decompress_Buffer2Buffer( char *inbuffer, int N, char **outbuffer, int *M )
{
 int k, success, sz1=0, sz2=0, buflen, continuation, totalin=0, totalout=0;
 unsigned char ch, chksum, chksum0;
 struct scz_item *buffer0_hd, *buffer0_tl, *bufpt, *bufprv, *sumlst_hd=0, *sumlst_tl=0;

 do 
  { /*Segment*/

   /* Convert buffer segment into linked list for expansion. */
   /* Get the 6-byte header to find the number of chars to read in this segment. */
   /*  (magic number (101), magic number (98), iter-count, seg-size (MBS), seg-size, seg-size (LSB). */ 
   buffer0_hd = 0;  buffer0_tl = 0;
   totalin = totalin + N;

   ch = inbuffer[sz1++];		/* Byte 1, expect magic numeral 101. */
   if ((sz1>8) || (ch!=101)) {printf("Error1: This does not look like a compressed buffer.\n"); return 0;}
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = inbuffer[sz1++];		/* Byte 2, expect magic numeral 98. */
   if (ch!=98) {printf("Error2: This does not look like a compressed buffer. (%d)\n", ch); return 0;}
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = inbuffer[sz1++];		/* Byte 3, iteration-count. */
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = inbuffer[sz1++];			/* Byte 4, MSB of segment buffer length. */
   buflen = ch << 16;
   ch = inbuffer[sz1++];			/* Byte 5, middle byte of segment buffer length. */
   buflen = (ch << 8) | buflen;
   ch = inbuffer[sz1++];			/* Byte 6, LSB of segment buffer length. */
   buflen = ch | buflen;
 
   k = 0;
   ch = inbuffer[sz1++];
   if (N<buflen+sz1+1) {printf("Error: Unexpectedly short buffer.\n");  buflen = N - sz1 - 1;}
   while (k<buflen)
    {
     scz_add_item( &buffer0_hd, &buffer0_tl, ch );
     ch = inbuffer[sz1++];     k++;
    }

   chksum0 = ch;
   ch = inbuffer[sz1++];
   // printf("End ch = '%c'\n", ch);

   /* Decode the 'end-marker'. */
   if (ch==']') continuation = 0;
   else
   if (ch=='[') continuation = 1;
   else {printf("Error4: Reading compressed buffer. (%d)\n", ch);  return 0; }

   success = Scz_Decompress_Seg( &buffer0_hd );	/* Decompress the buffer !!! */
   if (!success) return 0;

   /* Check checksum and sum length. */
   sz2 = 0;       /* Get buffer size. */
   bufpt = buffer0_hd;
   chksum = 0;
   bufprv = 0;
   while (bufpt!=0)
    {
     chksum += bufpt->ch;
     sz2++;  bufprv = bufpt;
     bufpt = bufpt->nxt;
    }
   if (chksum != chksum0) {printf("Error: Checksum mismatch (%dvs%d)\n", chksum, chksum0);}

   /* Attach to tail of main list. */
   totalout = totalout + sz2;
   if (sumlst_hd==0) sumlst_hd = buffer0_hd;
   else sumlst_tl->nxt = buffer0_hd;
   sumlst_tl = bufprv;

  } /*Segment*/
 while (continuation);

 /* Convert list into buffer. */
 *outbuffer = (char *)malloc((totalout)*sizeof(char));
 sz2 = 0;
 while (sumlst_hd!=0)
  {
   bufpt = sumlst_hd;
   (*outbuffer)[sz2++] = bufpt->ch;
   sumlst_hd = bufpt->nxt;
   sczfree(bufpt);
  }
 if (sz2 > totalout) printf("Unexpected overrun error\n");

 *M = totalout;

 printf("Decompression ratio = %g\n", (float)totalout / (float)totalin );
 return 1;
}

#endif /* SCZ_DECOMPRESSLIB_DEFD */
