// converted by Luigi Auriemma
// probably they are duplicates of other algorithms already available in quickbms (unz.c, scummvm and others)
// ALL THIS CODE IS UNTESTED!

// All the original code comes from http://www.shikadi.net/moddingwiki/Category:Compression_algorithms
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char byte;
typedef signed char s8;
typedef signed short s16;
typedef unsigned char u8;
typedef unsigned short u16;
typedef int bool;
#define true 1
#define false 0



int jazz_jackrabbit_rle(unsigned char *compressed, int compressed_length, unsigned char *out) {
    unsigned char   *o = out;
    int index = 0;
    int i;
		while (index < compressed_length) {
			if ((compressed[index] & 0xff) > 128) {
				// Repeat the next byte X times
				for (i = 0; i < (compressed[index] & 0xff) - 128; i++)
					*o++ = compressed[index + 1];
				index++;
			}
			else if ((compressed[index]) == 0) {
				// Write the last remaining byte to the buffer
				*o++ = compressed[++index];
				break;
			}
			else {
				// Read the next few bytes
				for (i = 0; i < compressed[index]; i++)
					*o++ = compressed[index + i + 1];
				index += i;
			}
			index++;
		}
    return o - out;
}



int keen13_rle(unsigned char *in, int insz, unsigned char *out, int outsz) {
    unsigned char   *o = out, *outl = out+ outsz, *inl = in + insz;
    while((in < inl) && (o < outl)) {
        int value = *in++;
        if(value >= 128) {
            for(value -= 127; value; value--) *o++ = *in++;
        } else {
            int c = *in++;
            for(value += 3; value; value--) *o++ = c;
        }
    }
    return o - out;
}



int sango_fighter_rlc(unsigned char *in, int insz, unsigned char *out, int outsz) {
    unsigned char   *o = out, *outl = out+ outsz, *inl = in + insz;
    while((in < inl) && (o < outl)) {
        int value = *in++;
        if(value < 128) {
            *o++ = value;
        } else {
            int c = *in++;
            for(value &= 0x7f; value; value--) *o++ = c;
        }
    }
    return o - out;
}



int westwood1(unsigned char *c_data, int c_data_Length, unsigned char *data, int data_Length) {
    /*
    Header h = GetHeader(c_data);
    byte[] data = new byte[h.originalSize];
    int a = 10 + h.paletteSize, b = 0;
    bool low = false;
    byte[] tribbles = new byte[(c_data_Length - (a + 1)) * 2 / 3 * 2];
    if (c_data[c_data_Length - 1] != 0x00) return null;
    */
    int a = 0, b = 0;
    int tribbles_Length = (c_data_Length - (a + 1)) * 2 / 3 * 2;
    unsigned char *tribbles = calloc(tribbles_Length, 1);
    bool low = false;
    int n;
    while (true)
    {
        if (a >= c_data_Length - 1) return -1;
        byte nibbles[3];
        for ( n = 0; n != 3; n++)
        {
            if (low == false)
            {
                nibbles[n] = (byte)(c_data[a] >> 4);
                low = true;
            }
            else
            {
                nibbles[n] = (byte)(c_data[a] & 0x0f);
                low = false;
                a++;
            }
        }
        if (b + 1 >= tribbles_Length) return -1;
        tribbles[b] = nibbles[0];
        tribbles[b + 1] = (byte)((nibbles[1] << 4) | nibbles[2]);
        if (tribbles[b] == 0x0f && tribbles[b + 1] == 0xff) break;
        b += 2;
    }
    //if (a + 1 != c_data_Length && a + 2 != c_data_Length && b + 2 != tribbles_Length) return -1;
    for (a = 0, b = 0; a != tribbles_Length - 2; a += 2)
    {
        int offset = a;
        int count = 0;
        while (tribbles[offset] != 0x00)
        {
            offset = (((tribbles[offset] - 1) << 8) | tribbles[offset + 1]) * 2;
            if (offset >= a) return -1;
            count++;
        }
        if (b == data_Length) return -1;
        data[b] = tribbles[offset + 1];
        b++;
        if (count == 0) continue;
        while (count != 0)
        {
            offset += 2;
            int o = offset;
            while (tribbles[offset] != 0x00)
            {
                offset = (((tribbles[offset] - 1) << 8) | tribbles[offset + 1]) * 2;
                if (offset >= o) return -1;
            }
            if (b == data_Length) return -1;
            data[b] = tribbles[offset + 1];
            b++;
            count--;
            offset = a;
            int counter = 0;
            while (counter != count)
            {
                offset = (((tribbles[offset] - 1) << 8) | tribbles[offset + 1]) * 2;
                if (offset >= a) return -1;
                counter++;
            }
        }
    }
    free(tribbles);
    return b;
}



int westwood3(unsigned char *src, unsigned char *dest, int len, int swapWord)  // Source data, Dest Data, and source size.
{
 
  s8 com;   // s8 is a signed byte
  s16 count;// signed word or short.
  u8 col/*, debug*/;
  int DP, SP, i;
  DP=0;
  SP=0;
 
 
  while( SP< len)
  {
    com = src[SP];
    SP++;
    if (com > 0)  // Copy command
    {
        for(i=0;i<com; i++)
        {
            dest[DP] = src[SP];
            DP++; SP++;
        }
 
    }else
    {
        if(com < 0 ) //Command 2 < fill 2
        {
            count = -com;  // invert the count to make it a positive number.
 
        }else // Other wise, Command 1 Fill 1
        {
            // This fixes beholde1.cps for the amiga EOB1
            if( swapWord == true)
               count = (src[SP+1] * 256 )+ src[SP];
            else
               count = (src[SP] * 256 )+ src[SP+1];
 
            SP +=2;
 
        }
 
        col = src[SP];  // Grab the colour to fill with.
        SP++;
 
        for(i=0;i<count; i++)
        {
            dest[DP] = col;
            DP++;
        }
    }
 
  }
  return DP;  // This should be 64000.
}


int westwood40(unsigned char *Source, unsigned char *Dest) {
    int Count, i, b;
  int SP=0;
  int DP=0;
  for(;;) {
    int Com=Source[SP];
    SP++;
 
    if ((Com & 0x80)!=0)  //{if bit 7 set}
    {
      if (Com!=0x80)   //{small skip command (1)}
      {
        Count=Com & 0x7F;
        DP+=Count;
      }
      else  //{Big commands}
      {
        Count=*(short *)(Source+SP);
        if (Count==0)  break;
        SP+=2;
 
        int Tc=(Count & 0xC000) >> 14;  //{Tc=two topmost bits of count}
 
        if((Tc==0)||(Tc==1)) {  //{Big skip (2)}
                  DP+=Count;
        } else if(Tc==2) { //{big xor (3)}
                Count=Count & 0x3FFF;
                for( i=1; i <= Count; i++)
                {
                  Dest[DP]=Dest[DP] ^ Source[SP];
                  DP++;
                  SP++;
                }
        } else if(Tc==3) {  //{big repeated xor (4)}
                Count=Count & 0x3FFF;
                b=Source[SP];
                SP++;
                for( i=1; i <= Count; i++)
                {
                  Dest[DP]=Dest[DP] ^ b;
                  DP++;
                }
        }
      }
    } else  //{xor command}
    {
      Count=Com;
      if (Count==0 )
      { //{repeated xor (6)}
        Count=Source[SP];
        SP++;
        b=Source[SP];
        SP++;
        for( i=1; i <= Count; i++)
        {
          Dest[DP]=Dest[DP] ^ b;
          DP++;
        }
      } else  //{copy xor (5)}
        for( i=1; i <= Count; i++)
        {
          Dest[DP]=Dest[DP] ^ Source[SP];
          DP++;
          SP++;
        }
    }
  }
    return DP;
}



int westwood80(unsigned char *Source, unsigned char *Dest) {
    int Posit, Count, i, b, b6, b7;
  int SP=0;
  int DP=0;
  for(;;) {
    int Com=Source[SP];
    SP++;
    b7=Com >> 7;  //{b7 is bit 7 of Com}
    if(b7== 0) {  //{copy command (2)}
            //{Count is bits 4-6 + 3}
            Count=((Com & 0x7F) >> 4) + 3;
            //{Position is bits 0-3, with bits 0-7 of next byte}
            Posit=((Com & 0x0F) << 8)+Source[SP];
            SP++;
            //{Starting pos=Cur pos. - calculated value}
            Posit=DP-Posit;
            for( i=Posit; i <= Posit+Count-1; i++)
            {
              Dest[DP]=Dest[i];
              DP++;
            }
    } else if(b7== 1) {
            //{Check bit 6 of Com}
            b6=(Com & 0x40) >> 6;
            if(b6 == 0) {  //{Copy as is command (1)}
                    Count=Com & 0x3F;  //{mask 2 topmost bits}
                    if(Count==0) break; //{EOF marker}
                    for (i=1; i <= Count; i++)
                    {
                      Dest[DP]=Source[SP];
                      DP++;
                      SP++;
                    }
            } else if(b6 == 1) {  //{large copy, very large copy and fill commands}
                    //{Count = (bits 0-5 of Com) +3}
                    //{if Com=FEh then fill, if Com=FFh then very large copy}
                    Count=Com & 0x3F;
                    if(Count<0x3E) //{large copy (3)}
                    {
                      Count+=3;
                      //{Next word = pos. from start of image}
                      Posit=*(short*)(&Source[SP]);
                      SP += 2;
                      for( i=Posit; i <= Posit+Count-1; i++)
                      {
                        Dest[DP]=Dest[i];
                        DP++;
                      }
                    }
                    else if(Count==0x3F)   //{very large copy (5)}
                    {
                      //{next 2 words are Count and Pos}
                      Count=*(short*)(&Source[SP]);
                      Posit=*(short*)(&Source[SP+2]);
                      SP+=4;
                      for( i=Posit; i <= Posit+Count-1; i++)
                      {
                        Dest[DP]=Dest[i];
                        DP++;
                      }
                    } else
                    {   //{Count=$3E, fill (4)}
                      //{Next word is count, the byte after is color}
                      Count=*(short*)(&Source[SP]);
                      SP += 2;
                      b=Source[SP];
                      SP++;
                      for (i=0; i <= Count-1; i++)
                      {
                        Dest[DP]=b;
                        DP++;
                      }
                    }
            }
          }
    }
    return DP;
}

