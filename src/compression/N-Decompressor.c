// modified by Luigi Auriemma
/*-----------------------------------------------------------------------------*\
|																				|
|	Nemesis.dll: Compression / Decompression of data in Nemesis format			|
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

// Art format decompressor  - This program is capable of decompressing tiles stored 
//							- in the art compression format used in many of the Sonic
//							- roms for the Sega Genesis.
// Programmed by Roger Sanders (AKA Nemesis) and William Sanders (AKA Milamber)
// Created 4/11/02		Last modified 25/11/02

//#include "iostream.h"
//#include <malloc.h>
//#include <fcntl.h>
//#include <sys/stat.h>
//#include <io.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define true    1
#define false   0
typedef int bool;
int stage_1 (long *pointer, char *rompointer, char *bufferpointer);
void stage_2 (long *pointer, char *rompointer, char *bufferpointer, unsigned long *tiles, short rtiles, bool alt_out, int *out_loc);

//long __cdecl NDecomp(char *SrcFile, char *DstFile, long Pointer)
int KENS_Nemesis(unsigned char *in, int insz, unsigned char *out, int outsz)
{
// In the following code, I removed the declaration of the Pointer variable as it is now passed
// as parameters. I also removed all console I/O operation as the function does not need to ask
// for filenames and for a pointer, as they are passed as parameters.

//set input and output modes to hex
//	cin.setf(ios::hex);

//misc variables and arrays
	int /*openrom = 0,*/ result = 0, out_loc = 0, loopcount = 0/*, file = 0, romsize = 0*/;
	long pointer = 0; //Pointer; //place in the rom to load from */
	unsigned short rtiles; //remaining tiles to decompress
	bool alt_out = false; //flag to change between the two different output modes
	unsigned long tiles[0x8000]; //output array
	//char romfilename[260]; //name of rom */
	//char outfilename[260]; //name of output file */

// We set up romfilename and outfilename according to the passed parameters
	//strcpy(romfilename, SrcFile);
	//strcpy(outfilename, DstFile);

//prompts for name of rom and reads it into memory
/*	cout << "Enter name of rom:\t\t\t"; */
/*	cin >> romfilename; */
	//openrom = _open( romfilename, _S_IWRITE, _O_BINARY );
	//if (openrom < 0)
	//{
/*		cout << "\n\nInvalid rom filename!"; */
		//return -1;
	//}
	//result = _setmode(openrom, _O_BINARY);
	//romsize = insz; //_filelength(openrom);
	char* rompointer = in; //(char *)calloc( romsize, 0x01 );
	//result = _read(openrom, rompointer, romsize);

//prompts for name of output file and pointer in rom
/*	cout << "Enter name of output file:\t\t"; */
/*	cin >> outfilename; */
/*	cout << "Enter location of archive in rom:\t"; */
/*	cin >> pointer; */

//allocates block of memory for decompression buffer
	char* bufferpointer = (char *)calloc( 0x200, 0x01 );

// There starts the original code by Nemesis
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//entry point into algorithm
	rtiles = (*(pointer++ + rompointer) * 0x100); //reads in the number of tiles in this block
	rtiles += (*(pointer++ + rompointer) & 0xFF);
	alt_out = (rtiles & 0x8000) / 0x8000; //ses the output mode based on the value of the first bit
	rtiles = 0x7FFF & rtiles; //truncates the first bit as it is no longer significant
	result = stage_1(&pointer, rompointer, bufferpointer); //calls the header decompression routine
	if(result == -1)
	{
		return -1;
	}
	stage_2(&pointer, rompointer, bufferpointer, &tiles[0], rtiles, alt_out, &out_loc); //calls the main decompression routine


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Additional filehanding to read decompressed tiles from memory into file
	//file = _creat(outfilename, _S_IWRITE);
	//result = _setmode(file, _O_BINARY);
    unsigned char *p = out;
	for(loopcount = 0; loopcount < out_loc; loopcount++)
	{
		tiles[loopcount] = ((tiles[loopcount] & 0xFF000000) / 0x1000000) + 
						   ((tiles[loopcount] & 0xFF0000) / 0x100) + 
						   ((tiles[loopcount] & 0xFF00) *0x100) + 
						   ((tiles[loopcount] & 0xFF) * 0x1000000); //byteswaps the output before writing to the output file
		//result = _write(file, (&tiles[loopcount]) , 4);
        if((p + 4) > (out + outsz)) return(-1);
        memcpy(p, (&tiles[loopcount]) , 4);
        p += 4;
	}
	//result = _close(file);
	//_close(openrom);
	//return 0;
    free(bufferpointer);
    return(p - out);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//First stage decompression algorithm
int stage_1 (long *pointer, char *rompointer, char *bufferpointer)
{
	unsigned short in_val; //storage for value coming from rom
	unsigned short out_val = 0; //storage for output value to decompression buffer
	unsigned short num_times; //number of times the calculated output value is written to the buffer
	unsigned short offset; //stores buffer offset to write to
	//main loop. Header is terminated by the value of 0xFF in the rom
	for (in_val = (*((*pointer)++ + rompointer) & 0xFF); in_val != 0xFF; in_val = (*((*pointer)++ + rompointer) & 0xFF))
	{
		if(in_val > 0x7F) //if most significant bit is set, store the last 4 bits and discard the rest
		{
			out_val = in_val; //last four bits set here are not changed until it enters here again
			in_val = (*((*pointer)++ + rompointer) & 0xFF); //read another value from the rom
		}
		num_times = 0x08 - (in_val & 0x0F); //the last 4 bits here determine the number of times the value is written to the buffer. Never greater than 8
		in_val = ((in_val & 0xF0) / 0x10) + ((in_val & 0x0F) * 0x10); //nibble swap the value from the rom
		out_val = (out_val & 0x0F) + (in_val * 0x10); //multiply the input value by 0x10 and place it in front of the last 4 bits in the output value
		offset = (*((*pointer)++ + rompointer) & 0xFF) * pow(2, (num_times + 1)); //read another value from the rom and use it to determine the offset
		if(offset >= 0x200)
		{
			return -1;
		}

		num_times = pow(2, num_times); //finish setting the number of times the value is written to the buffer
		for(; num_times != 0; num_times--,offset += 2) //loop for writing the values to the buffer
		{
			*(bufferpointer + offset + 1) = (out_val & 0x00FF);
			*(bufferpointer + offset) = ((out_val & 0xFF00) / 0x100); //wriiting the values to the buffer
		}
	}
	return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Second stage decompression algorithm
void stage_2 (long *pointer, char *rompointer, char *bufferpointer, unsigned long *tiles, short rtiles, bool alt_out, int *out_loc)
{
	unsigned short rnibbles; //remaining nibbles in this line
	unsigned long out_val; //output value
	unsigned short rom_mod = 0x10; //This one is complex. Used in maths as a power of 2, but also controls when 
									//new values are read from the rom. Just read the algorithm.
	unsigned long alt_out_val = 0; //If the alternate output method is used by this archive, each line is XOR'd with
								   //the line before it prior to writing to the output.
	unsigned short rlines; //remaining lines to decompress in this tile
	unsigned short mode_val; //There are two different things it can do in the main loop. This holds the value to switch between them
	unsigned short bufferoffset; //the buffer offset to read from next
	unsigned short next_out = 0; //the next nibble to be added to the output.
	short num_times = 0; //the number of times the value is repeated in the output
	unsigned short rom_val = (*((*pointer)++ + rompointer) * 0x100); //value from the rom
	rom_val += (*((*pointer)++ + rompointer) & 0xFF);
	for(;rtiles > 0;rtiles--) //main loop for tiles
	{
		for (rlines = 8; rlines != 0; rlines--) //loop for lines in the tile
		{
			for (out_val = 0, rnibbles = 8; rnibbles != 0; rnibbles--) //loop for nibbles in the line
			{
				num_times--; //decrements number of times to write current value to output
				if (num_times == -1) //if -1, calculate new value
				{
					mode_val = rom_val / pow(2, (rom_mod - 8)); //determines which set of commands to perform
					if ((mode_val & 0x00FF) >= 0xFC) //determining value is 0xFC or greater
					{
						if (rom_mod < 0x0F) //determines whether or not to read another value from the rom
						{
							rom_mod += 8; //and adds 8 to rom_mod if it does
							rom_val = (rom_val * 0x100) + (*((*pointer)++ + rompointer) & 0xFF);
						}
						rom_mod -= 0x0D; //subtracts 0x0D from rom_mod
						num_times = ((short)(rom_val / pow(2, rom_mod)) & 0x70) / 0x10; //calculates number of times to write value to output
						next_out = (short)(rom_val / pow(2, rom_mod)) & 0x0F; //calculates next output value
					}
					else
					{
						bufferoffset = (mode_val & 0xFF) * 2; //sets buffer offset
						rom_mod -= *(bufferpointer + bufferoffset); //reads a buffer value, then subtracts it from rom_mod. Never greater than 0x7
						num_times = (*(bufferpointer + bufferoffset + 1) & 0xF0) / 0x10; //set number of times based on a buffer value. Never greater than 0x7
						next_out = *(bufferpointer + bufferoffset + 1) & 0x0F; //gets next output value from buffer
					}
					if (rom_mod < 9) //again, potentially load a value from the rom
					{
						rom_mod += 8; //and add 8 to rom_mod
						rom_val = (rom_val * 0x100) + (*((*pointer)++ + rompointer) & 0xFF);
					}
				}
				out_val = out_val * 0x10 + next_out; //adds next value to output
			}
			if (alt_out == false)
			{
				*(tiles + (*out_loc)++) = out_val; //writes next line to output array
			}
			else
			{
				alt_out_val = alt_out_val ^ out_val; //Alternate output method. Only bits set are those different to the last line
				*(tiles + (*out_loc)++) = alt_out_val;
			}
		}
	}
}
