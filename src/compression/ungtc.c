// modified by Luigi Auriemma
/*******************************************************************\
*                                                   *
*   GT lossless compression system, multi platform C code.         *
*                                                   *
\*******************************************************************/

#define UINT8   unsigned char
#define UINT16  unsigned short
#define UINT32  unsigned int


#ifdef _WINDOWS
#define READINFOBIT(v,p,c) {v=*((UINT32 *)p)++;c=32;}
#define GETSRCWORD(v,p) v=*((UINT16 *)p)++
#else
#define READINFOBIT(v,p,c) {v=((*(p+3))<<24)+((*(p+2))<<16)+((*(p+1))<<8)+*p;p+=4;c=32;}
#define GETSRCWORD(v,p) v=*p++;v+=((*p++)<<8)
#endif

#define NEXTINFOBIT(v,p,c) v>>=1; if (!(--c)) READINFOBIT(v,p,c)



/*******************************************************************\
*                                                   *
*   GT32 decompressor written in C for porting speed and also for   *
*   decompression speed. Should still be faster than loading off   *
*   CD for most data.                                    *
*                                                   *
\*******************************************************************/

int ungtc(UINT8 *GtPtr,UINT8 *DestPtr)
{
   register UINT32 InfoBits;
   register UINT16 InfoCnt;
   register UINT8 *CopySrc;
   register UINT16 CopyCnt;


    UINT8   *ret = DestPtr;

   /*********************\
   * Advance Past Header *
   \*********************/

   //GtPtr+=((PGTHEADER)GtPtr)->GTSkip;
   //GtPtr+=sizeof(GTHEADER);


   /*******************\
   * Pre Read InfoBits *
   \*******************/

   READINFOBIT(InfoBits,GtPtr,InfoCnt);


   /*******************\
   * Process Forever ! *
   \*******************/

   while (1)
      {
      if (!(InfoBits & 1))
         {
         /*********\
         * Literal *
         \*********/

         *DestPtr++=*GtPtr++;
         }
      else
         {
         /****************\
         * Encoded String *
         \****************/

         NEXTINFOBIT(InfoBits,GtPtr,InfoCnt);

         if (InfoBits & 1)
            {
            /***************\
            * Medium String *
            \***************/

            GETSRCWORD(CopyCnt,GtPtr);
            CopySrc=DestPtr+((CopyCnt>>3) | 0xffffe000);

            if ((CopyCnt = CopyCnt & 7))
               {
               CopyCnt+=2;
               }
            else
               {
               /***************\
               * Longer String *
               \***************/

               CopyCnt=*GtPtr++;

               if (CopyCnt & 128) CopySrc-=0x2000;

               CopyCnt &= 127;


               /*************************\
               * Check for Special Codes *
               \*************************/

               if(CopyCnt == 1) return(DestPtr - ret);
               if(!CopyCnt)
                  {
                  GETSRCWORD(CopyCnt,GtPtr);
                  }
               else
                  {
                  CopyCnt+=2;
                  }
               }
            while (CopyCnt--) *DestPtr++=*CopySrc++;
            }
         else
            {
            /**********************************************\
            * Short String: 2-5 bytes long, up to 256 back *
            \**********************************************/

            CopySrc=DestPtr + (0xffffff00 | *GtPtr++);

            *DestPtr++=*CopySrc++;
            *DestPtr++=*CopySrc++;

            NEXTINFOBIT(InfoBits,GtPtr,InfoCnt);


            /***********************************\
            * InfoCnt MUST Be 1 Or Greater Here *
            \***********************************/

            if(InfoBits & 1)
               {
               *DestPtr++=*CopySrc++;
               *DestPtr++=*CopySrc++;
               }

            NEXTINFOBIT(InfoBits,GtPtr,InfoCnt);

            if(InfoBits & 1)
               {
               *DestPtr++=*CopySrc++;
               }
            }
         }
      NEXTINFOBIT(InfoBits,GtPtr,InfoCnt);
      }
    return(DestPtr - ret);
}
