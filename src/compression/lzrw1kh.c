// modified by Luigi Auriemma
/*   ###################################################################
     ##                                                               ##
     ##      ##    ##### #####  ##   ##  ##      ## ##  ## ##  ##     ##
     ##      ##      ### ##  ## ## # ## ###     ##  ## ##  ##  ##     ##
     ##      ##     ###  #####  #######  ##    ##   ####   ######     ##
     ##      ##    ###   ##  ## ### ###  ##   ##    ## ##  ##  ##     ##
     ##      ##### ##### ##  ## ##   ## #### ##     ##  ## ##  ##     ##
     ##                                                               ##
     ##   EXTREMELY FAST AND EASY TO UNDERSTAND COMPRESSION ALGORITM  ##
     ##                                                               ##
     ###################################################################
     ##                                                               ##
     ##   This header file implements the  LZRW1/KH algoritm which    ##
     ##   also implements  some RLE coding  which is usefull  when    ##
     ##   compressing files containing a lot of consecutive  bytes    ##
     ##   having the same value.   The algoritm is not as good  as    ##
     ##   LZH, but can compete with Lempel-Ziff.   It's the fasted    ##
     ##   one I've encountered upto now.                              ##
     ##                                                               ##
     ##                                                Kurt HAENEN    ##
     ##                                                               ##
     ###################################################################   */

#define   FLAG_Copied         0x80
#define   FLAG_Compress       0x40
#define   TRUE                1
#define   FALSE               0
typedef   signed char         byte;
typedef   signed int          word;     // 32bit
typedef   unsigned char       ubyte;
typedef   unsigned int        uword;    // 32bit
typedef   unsigned long       ulong;
typedef   unsigned char       bool;

static bool      lzrw1kh_GetMatch(Source,X,SourceSize,Hash,Size,Pos)
ubyte     *Source;
uword     X;
uword     SourceSize;
word      *Hash;
uword     *Size;
word      *Pos;
{    uword     HashValue      = (40543L*((((Source[X] << 4) ^
                                   Source[X+1]) << 4) ^ Source[X+2]) >> 4) & 0xfff;
     *Pos = Hash[HashValue];
     Hash[HashValue] = X;
     if ((*Pos != -1) && ((X-*Pos) < 4096))
          {    for (*Size = 0; ((*Size < 18)
                         && (Source[X+*Size] == Source[*Pos+*Size])
                         && ((X+*Size) < SourceSize)); (*Size)++);
               return(*Size >= 3);
          }
     return(FALSE);
}

uword     lzrw1kh_Compression(Source,Dest,SourceSize)
ubyte     *Source,*Dest;
uword     SourceSize;
{    word      Hash[4096];
     uword     Key,Size,Pos;
     ubyte     Bit                      = 0;
     uword     Command                  = 0;
     uword     X                        = 0;
     uword     Y                        = 3;
     uword     Z                        = 1;
     for (Key = 0; Key < 4096; Key++)
          Hash[Key] = -1;
     Dest[0] = FLAG_Compress;
     for (; (X < SourceSize) && (Y <= SourceSize);)
         {     //printf("X = %i Y = %i Z = %i",X,Y,Z);
               if (Bit > 15)
                    {    Dest[Z++] = (Command >> 8) & 0x00ff;
                         Dest[Z] = Command & 0x00ff;
                         Z = Y;
                         Bit = 0;
                         Y += 2;
                    }
               for (Size = 1; (Source[X] == Source[X+Size]) && (Size < 0x0fff)
                                   && (X+Size < SourceSize); Size++);
               if (Size >= 16)
                    {    Dest[Y++] = 0;
                         Dest[Y++] = ((Size - 16) >> 8) & 0x00ff;
                         Dest[Y++] = (Size - 16) & 0x00ff;
                         Dest[Y++] = Source[X];
                         X += Size;
                         Command = (Command << 1) + 1;
                    }
               else if (lzrw1kh_GetMatch(Source,X,SourceSize,Hash,&Size,&Pos))
                         {    Key = ((X-Pos) << 4) + (Size - 3);
                              Dest[Y++] = (Key >> 8) & 0x00ff;
                              Dest[Y++] = Key & 0x00ff;
                              X += Size;
                              Command = (Command << 1) + 1;
                         }
                    else {    Dest[Y++] = Source[X++];
                              Command = (Command << 1);
                         }
               Bit++;
          }
     Command <<= (16-Bit);
     Dest[Z++] = (Command >> 8) & 0x00ff;
     Dest[Z] = Command & 0x00ff;
     if (Y > SourceSize)
          {    for(Y = 0; Y < SourceSize; Dest[Y+1] = Source[Y++]);
               Dest[0] = FLAG_Copied;
               return(SourceSize+1);
          }
     return(Y);
}

uword     lzrw1kh_Decompression(Source,Dest,SourceSize)
ubyte     *Source,*Dest;
uword     SourceSize;
{    uword     X                        = 3;
     uword     Y                        = 0;
     uword     Pos,Size,K;
     uword     Command                  = (Source[1] << 8) + Source[2];
     ubyte     Bit                      = 16;
     if (Source[0] == FLAG_Copied)
          {    for (Y = 1; Y < SourceSize; Dest[Y-1] = Source[Y++]);
               return(SourceSize-1);
          }
     for (; X < SourceSize;)
          {    if (Bit == 0)
                    {    Command = (Source[X++] << 8);
                         Command += Source[X++];
                         Bit = 16;
                    }
               if (Command & 0x8000)
                    {    Pos = (Source[X++] << 4);
                         Pos += (Source[X] >> 4);
                         if (Pos)
                              {    Size = (Source[X++] & 0x0f)+3;
                                   for (K = 0; K < Size; K++)
                                        Dest[Y+K] = Dest[Y-Pos+K];
                                   Y += Size;
                              }
                         else {    Size = (Source[X++] << 8);
                                   Size += Source[X++] + 16;
                                   for (K = 0; K < Size; Dest[Y+K++] = Source[X]);
                                   X++;
                                   Y += Size;
                              }
                    }
               else Dest[Y++] = Source[X++];
               Command <<= 1;
               Bit--;
          }
     return(Y);
}
