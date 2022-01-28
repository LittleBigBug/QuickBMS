// modified by Luigi Auriemma
/*-----------------------------------------------------------------------------*\
|																				|
|	Kosinski.dll: Compression / Decompression of data in Kosinski format		|
|	Copyright © 2002-2004 The KENS Project Development Team						|
|																				|
|	This library is free software; you can redistribute it and/or				|
|	modify it under the terms of the GNU Lesser General Public					|
|	License as published by the Free Software Foundation; either				|
|	version 2.1 of the License, or (at your option) any later version.			|
|																				|
|	This library is distributed in the hope that it will be useful,				|
|	but WITHOUT ANY WARRANTY; without even the implied warranty of				|
|	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU			|
|	Lesser General Public License for more details.								|
|																				|
|	You should have received a copy of the GNU Lesser General Public			|
|	License along with this library; if not, write to the Free Software			|
|	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA	|
|																				|
\*-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define true    1
#define false   0
typedef int bool;
//extern unsigned long GetFileSize(FILE *File);

//#define SUCCESS										0x00
//#define ERROR_UNKNOWN								0x01
//#define ERROR_SOURCE_FILE_DOES_NOT_EXIST			0x02

//-----------------------------------------------------------------------------------------------
// Name: KDecomp(char *SrcFile, char *DstFile, long Location, bool Moduled)
// Desc: Decompresses the data using the Kosinski compression format
//-----------------------------------------------------------------------------------------------
//long KDecomp(char *SrcFile, char *DstFile, long Location, bool Moduled)
int KENS_Kosinski(unsigned char *in, int insz, unsigned char *out, int outsz, int Moduled)
{
// Files
	//FILE *Src;
	//FILE *Dst;

// Bitfield Infos
	unsigned short BITFIELD;
	unsigned char BFP;
	unsigned char Bit;

// R/W infos
	unsigned char Byte;
	unsigned char Low, High;

// Localization Infos
	int Pointer;
	
// Count and Offest
	long Count = 0;
	long Offset = 0;

// Moduled infos
	long FullSize = 0;
	long DecBytes = 0;

//------------------------------------------------------------------------------------------------

	//Src=fopen(SrcFile,"rb");	
	//if (Src==NULL) return ERROR_SOURCE_FILE_DOES_NOT_EXIST;
	//Dst=fopen(DstFile,"w+b");

    //int Location = 0;
	//fseek(Src, Location, SEEK_SET);
    unsigned char *p = out;

	if (Moduled)
	{
		High = *in++;
		Low = *in++;
		FullSize = ((long)High << 8) + (long)Low;
	}

start:
	BITFIELD = *(unsigned short *)in; in += 2;
	BFP=0;

//------------------------------------------------------------------------------------------------
	while(1)
	{
		if(BITFIELD & (1<<BFP++)) Bit=1; else Bit=0;
		if (BFP>=16) { BITFIELD = *(unsigned short *)in; in += 2; BFP=0; }
//-- Direct Copy ---------------------------------------------------------------------------------
		if (Bit)
		{
			Byte = *in++;
            if(p >= (out + outsz)) return(-1);
			*p++ = Byte;
			DecBytes+=1;
		}
		else
		{
			if(BITFIELD & (1<<BFP++)) Bit=1; else Bit=0;
			if (BFP>=16) { BITFIELD = *(unsigned short *)in; in += 2; BFP=0; }
//-- Embedded / Separate -------------------------------------------------------------------------
			if (Bit)
			{
				Low = *in++;
				High = *in++;

				Count=(long)(High & 0x07);

				if (Count==0)
				{
					Count = *in++;
					if (Count==0) goto end;
					if (Count==1) continue;
				}
				else
				{
					Count+=1;
			    }

				Offset = 0xFFFFE000 | ((long)(0xF8 & High) << 5) | (long)Low;
			}
//-- Inline --------------------------------------------------------------------------------------
			else
			{
				if(BITFIELD & (1<<BFP++)) Low=1; else Low=0;
				if (BFP>=16) { BITFIELD = *(unsigned short *)in; in += 2; BFP=0; }
				if(BITFIELD & (1<<BFP++)) High=1; else High=0;
				if (BFP>=16) { BITFIELD = *(unsigned short *)in; in += 2; BFP=0; }

				Count = ((long)Low)*2 + ((long)High) + 1;

				Offset = *in++;
				Offset|=0xFFFFFF00;
			}
//-- Write to file for indirect copy -------------------------------------------------------------
            int i;
			for (i=0; i<=Count; i++)
			{
				Pointer=(p - out);
				p += Offset; //fseek(Dst, Offset, SEEK_CUR);
				Byte = *in++;
				p = out + Pointer; //fseek(Dst, Pointer, SEEK_SET);
                if(p >= (out + outsz)) return(-1);
				*p++ = Byte;
			}
			DecBytes+=Count+1;
//------------------------------------------------------------------------------------------------
		}
	}
//------------------------------------------------------------------------------------------------

end:
	if (Moduled)
	{
		if (DecBytes < FullSize)
		{
			do { Byte = *in++; } while (Byte==0);
			in--;   //fseek(Src, -1, SEEK_CUR);
			goto start;
		}
	}
	//fclose(Dst);
	//fclose(Src);
	//return SUCCESS;
    return(p - out);
}
