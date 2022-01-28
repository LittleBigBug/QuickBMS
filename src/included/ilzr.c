/*
This xpk library named ILZR (Incremental Lempel-Ziv-Renau) is a fast
Hash version of LZSS and works with two search tables in oposite
directions.
This library packs & unpacks obtaining good compression ratio's with
fast speed and low memory requirements.

                        --------------
Author:

Jose Renau Ardevol is one of Ramon LLull University system adminis-
trators. This Spanish Programmer actually works on Multi-Processor
Linux Development. He studies Informatic Systems Engineering.

Adress: Juan Torras 26 8-1
				Barcelona -Spain-

Phone:	(93) 345 80 59

e-mail: renau@els.url.es

                        --------------
Uploader:

Jordi Caimel Compte also known as StarWave, studies Electronics &
Comunications Engineering in the same university.
He is an Spanish Amiga musician & programmer.

Adress: Santa Llucia 15
				Tordera (Barcelona) -Spain-

Phone:	(93) 765 04 67

e-mail: se03375@els.url.es

                        --------------
University:

EUETT:Escola Universitaria d'Enginyeria Tecnica de Telecomunicació
ETSEEI:Escola Tecnica Superior d'Enginyeria Electronica i Informatica
 				
          Ramon Llull
       Passeig Bonanova 8
        Barcelona (Spain)

                        --------------


Thanks to:

All Aminet people for his work and dedication,
All xpk developers for his code examples,
Linux,GNU,etc...
and to all people who makes possible Amiga computers are the best.
*/
// modified by Luigi Auriemma to avoid the reading of the 16bit uncompressed size at the beginning


#define ILZR_INIT_BIT_BUMP         8
#define ILZR_BITS_LOOKAHEAD        4
#define ILZR_RAW_LOOKAHEAD         ( 1 << BITS_LOOKAHEAD ) 
#define ILZR_MIN_MATCH             3     /* No lo toques o no funciona */
#define ILZR_MAX_MATCH             (RAW_LOOKAHEAD + MIN_MATCH -1 )

int ilzr_expand(unsigned char *xpar_InBuf, int xpar_InBufLen, unsigned char *xpar_OutBuf, int xpar_OutBufLen) {
    /*  variables para input - output   */
  
          unsigned char *inpb       = xpar_InBuf;
          unsigned char *inpl       = xpar_InBuf + xpar_InBufLen;
          unsigned char *outb       = xpar_OutBuf;
          unsigned char *outl       = xpar_OutBuf + xpar_OutBufLen;
          unsigned char *bumpcode;
          unsigned char *outbstart  = xpar_OutBuf;
          signed char   shift       = 0;
          unsigned char bitcount;
          signed long   outbsize    = xpar_OutBufLen;
          unsigned      matchpos;
          unsigned      matchlen;      

  //outbsize = *((unsigned short *)inpb);
  //inpb    += 2;     /* short leido */

  bitcount = ILZR_INIT_BIT_BUMP;
  bumpcode = &outb[ 1<<ILZR_INIT_BIT_BUMP ]; 
  
  //if( outbsize > xpar_OutBufLen )
    //return( XPKERR_SMALLBUF );
  
  //xpar_OutBufLen = outbsize; 
    
  while( outbsize > 0 )
    {
    if(inpb >= inpl) break;
    if( *inpb & (0x80>>shift) )
      {
      if( (++shift) > 7 )
        {
        shift=0;
        inpb++;
        }

      if(inpb >= inpl) break;
      if(outb >= outl) return(-1);
      *outb = ( ((unsigned *)inpb)[0] )>>(24-shift);
      outb++;
      inpb++;
      outbsize--;
      }
    else
      {
      if( ++shift > 7 )
        {
        shift=0;
        inpb++;
        }
      
      if( bumpcode <  outb )
        {
        bitcount++;
        bumpcode = &outbstart[ 1<<bitcount ];
        }

      if(inpb >= inpl) break;
      matchpos = (( ((unsigned *)inpb)[0] ) >> ( 32 - bitcount - shift ) ) & ((1<<bitcount)-1);
      
      shift   = shift + bitcount - 8;    /* ALWAYS bitccount >= 8 */
      inpb++;
      if( shift > 7 )
        {
        shift-=8;
        inpb++;
        }

      if(inpb >= inpl) break;
      matchlen = (( ((unsigned *)inpb)[0] ) >> ( 32 - ILZR_BITS_LOOKAHEAD - shift ) ) & ((1<<ILZR_BITS_LOOKAHEAD)-1);
      
      shift   += ILZR_BITS_LOOKAHEAD;
      if( shift > 7 )
        {
        shift-=8;
        inpb++;
        }
        
      matchlen += ILZR_MIN_MATCH;

      if((outb + matchlen) > outl) return(-1);
      memcpy( outb , &outbstart[ matchpos ] , (size_t)matchlen );
      outb     += matchlen;

      outbsize -= matchlen;
      }
    }
  return( outb - xpar_OutBuf );
}

