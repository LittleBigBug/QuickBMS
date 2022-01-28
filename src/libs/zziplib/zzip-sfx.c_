/*---------------------------------------------*/
/* Zzip/Zzlib compressor            zzip-sfx.c */
/*---------------------------------------------*/

/*
  This file is a part of zzip and/or zzlib, a program and
  library for lossless, block-sorting data compression.
  Copyright (C) 1999-2001 Damien Debin. All Rights Reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the 
  Free Software Foundation, Inc., 
  59 Temple Place, Suite 330, 
  Boston, MA 02111-1307 USA

  Damien Debin
  <damien@debin.net>

  This program is based on (at least) the work of: Mike Burrows, 
  David Wheeler, Peter Fenwick, Alistair Moffat, Ian H. Witten, 
  Robert Sedgewick, Jon Bentley, Brenton Chapin, Stephen R. Tate, 
  Szymon Grabowski, Bernhard Balkenhol, Stefan Kurtz
*/

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "sfx_code.h"
#include "global.h"

#define BUFFER_SIZE (512*1024)

static int last_error = OK;

static const uint32 crc32_table[] ALIGN = {
0UL, 16777216UL, 33554432UL, 50331648UL, 67108864UL, 83886080UL, 100663296UL, 
117440512UL, 134217728UL, 150994944UL, 167772160UL, 184549376UL, 201326592UL, 218103808UL, 
234881024UL, 251658240UL, 268435456UL, 285212672UL, 301989888UL, 318767104UL, 335544320UL, 
352321536UL, 369098752UL, 385875968UL, 402653184UL, 419430400UL, 436207616UL, 452984832UL, 
469762048UL, 486539264UL, 503316480UL, 520093696UL, 536870912UL, 553648128UL, 570425344UL, 
587202560UL, 603979776UL, 620756992UL, 637534208UL, 654311424UL, 671088640UL, 687865856UL, 
704643072UL, 721420288UL, 738197504UL, 754974720UL, 771751936UL, 788529152UL, 805306368UL, 
822083584UL, 838860800UL, 855638016UL, 872415232UL, 889192448UL, 905969664UL, 922746880UL, 
939524096UL, 956301312UL, 973078528UL, 989855744UL, 1006632960UL, 1023410176UL, 1040187392UL, 
1056964608UL, 1073741824UL, 1090519040UL, 1107296256UL, 1124073472UL, 1140850688UL, 1157627904UL, 
1174405120UL, 1191182336UL, 1207959552UL, 1224736768UL, 1241513984UL, 1258291200UL, 1275068416UL, 
1291845632UL, 1308622848UL, 1325400064UL, 1342177280UL, 1358954496UL, 1375731712UL, 1392508928UL, 
1409286144UL, 1426063360UL, 1442840576UL, 1459617792UL, 1476395008UL, 1493172224UL, 1509949440UL, 
1526726656UL, 1543503872UL, 1560281088UL, 1577058304UL, 1593835520UL, 1610612736UL, 1627389952UL, 
1644167168UL, 1660944384UL, 1677721600UL, 1694498816UL, 1711276032UL, 1728053248UL, 1744830464UL, 
1761607680UL, 1778384896UL, 1795162112UL, 1811939328UL, 1828716544UL, 1845493760UL, 1862270976UL, 
1879048192UL, 1895825408UL, 1912602624UL, 1929379840UL, 1946157056UL, 1962934272UL, 1979711488UL, 
1996488704UL, 2013265920UL, 2030043136UL, 2046820352UL, 2063597568UL, 2080374784UL, 2097152000UL, 
2113929216UL, 2130706432UL, 2147483648UL, 2164260864UL, 2181038080UL, 2197815296UL, 2214592512UL, 
2231369728UL, 2248146944UL, 2264924160UL, 2281701376UL, 2298478592UL, 2315255808UL, 2332033024UL, 
2348810240UL, 2365587456UL, 2382364672UL, 2399141888UL, 2415919104UL, 2432696320UL, 2449473536UL, 
2466250752UL, 2483027968UL, 2499805184UL, 2516582400UL, 2533359616UL, 2550136832UL, 2566914048UL, 
2583691264UL, 2600468480UL, 2617245696UL, 2634022912UL, 2650800128UL, 2667577344UL, 2684354560UL, 
2701131776UL, 2717908992UL, 2734686208UL, 2751463424UL, 2768240640UL, 2785017856UL, 2801795072UL, 
2818572288UL, 2835349504UL, 2852126720UL, 2868903936UL, 2885681152UL, 2902458368UL, 2919235584UL, 
2936012800UL, 2952790016UL, 2969567232UL, 2986344448UL, 3003121664UL, 3019898880UL, 3036676096UL, 
3053453312UL, 3070230528UL, 3087007744UL, 3103784960UL, 3120562176UL, 3137339392UL, 3154116608UL, 
3170893824UL, 3187671040UL, 3204448256UL, 3221225472UL, 3238002688UL, 3254779904UL, 3271557120UL, 
3288334336UL, 3305111552UL, 3321888768UL, 3338665984UL, 3355443200UL, 3372220416UL, 3388997632UL, 
3405774848UL, 3422552064UL, 3439329280UL, 3456106496UL, 3472883712UL, 3489660928UL, 3506438144UL, 
3523215360UL, 3539992576UL, 3556769792UL, 3573547008UL, 3590324224UL, 3607101440UL, 3623878656UL, 
3640655872UL, 3657433088UL, 3674210304UL, 3690987520UL, 3707764736UL, 3724541952UL, 3741319168UL, 
3758096384UL, 3774873600UL, 3791650816UL, 3808428032UL, 3825205248UL, 3841982464UL, 3858759680UL, 
3875536896UL, 3892314112UL, 3909091328UL, 3925868544UL, 3942645760UL, 3959422976UL, 3976200192UL, 
3992977408UL, 4009754624UL, 4026531840UL, 4043309056UL, 4060086272UL, 4076863488UL, 4093640704UL, 
4110417920UL, 4127195136UL, 4143972352UL, 4160749568UL, 4177526784UL, 4194304000UL, 4211081216UL, 
4227858432UL, 4244635648UL, 4261412864UL, 4278190080UL };

static
uint32 Crc32(uint8  *buffer, 
			 uint8  *buffer_end, 
			 uint32 crc)
{
	while (buffer < buffer_end)
		crc = (crc >> 8) ^ crc32_table[(crc ^ *buffer++) & 0xFF];

	return crc;
}

static
void MakeSFX(char *fname_in, 
			 char *fname_out)
{
	FILE   *fin, *fout;
	uint8  *buffer;
	uint32 nb, crc, first_time = 0;

	buffer = (uint8*)malloc(BUFFER_SIZE * sizeof(uint8));
	if (buffer == NULL)
	{
		last_error = NOT_ENOUGH_MEMORY;
		return;
	}

	if ((fin = fopen(fname_in, "rb")) == NULL)
	{ 
		last_error = CANNOT_OPEN_INPUT_FILE; 
		return;
	}

	if ((fout = fopen(fname_out, "wb")) == NULL)
	{ 
		last_error = CANNOT_OPEN_OUTPUT_FILE; 
		return;
	}

	fwrite(sfx_code, 1, SFX_CODE_SIZE, fout);
	if (ferror(fout) != 0)
	{
		last_error = errno;
		return;
	}

	crc = Crc32(sfx_code, sfx_code + SFX_CODE_SIZE, 0xFFFFFFFFUL);

	while (feof(fin) == 0)
	{
		nb = fread(buffer, 1, BUFFER_SIZE, fin);
		if (ferror(fin) != 0)
		{
			last_error = errno;
			return;
		}

		if (first_time == 0)
		{
			first_time = 1;
			if (buffer[0] != 'Z' || buffer[1] != 'Z')
			{ 
				last_error = NOT_A_ZZIP_FILE; 
				return;
			}
			if (buffer[2] != VERSION_NUMBER) 
			{ 
				last_error = UNSUPPORTED_VERSION; 
				return; 
			}
		}

		fwrite(buffer, 1, nb, fout);
		if (ferror(fout) != 0)
		{
			last_error = errno;
			return;
		}

		crc = Crc32(buffer, buffer + nb, crc);
	} 

	fwrite(&crc, 1, sizeof(crc), fout);
	if (ferror(fout) != 0)
	{
		last_error = errno;
		return;
	}

	if (fclose(fin ) != 0) 
	{ 
		last_error = CANNOT_CLOSE_INPUT_FILE;  
		return; 
	}

	if (fclose(fout) != 0) 
	{ 
		last_error = CANNOT_CLOSE_OUTPUT_FILE; 
		return; 
	}

	free(buffer);
}

static
void PrintVersion()
{
	printf(
		"*** ZZIP-SFX " VERSION_STRING "\n"
		"*** Copyright (c)2001 Damien Debin, all rights reserved.\n"
		);
}

static
void Help()
{
	printf("\n");
	PrintVersion();
	printf(
		"\n usage: zzip-sfx file\n\n"
		" http://www.zzip.f2s.com/\n"
		" <damien.debin@via.ecp.fr>\n"
		);
	exit(1);
}

int main(int  argc, 
		 char **argv)
{
	char *file_in, *file_out, *p;

	if (argc == 1) Help();
	PrintVersion();

	file_in = argv[1];
	file_out = (char*)malloc(strlen(file_in) + 5);
	if (file_out == NULL) exit(1);
	strcpy(file_out, file_in);
	p = strrchr(file_out, '.');
	if (p != NULL) *p = '\0';
	strcat(file_out, ".exe");

	printf("\n building a SelF-eXtracting archive '%s' from '%s'...", file_out, file_in);

	MakeSFX(file_in, file_out);

	if (last_error != OK)
	{
		fprintf(stderr, "\nZzip-sfx error: ");
		switch (last_error)
		{
		case NOT_A_ZZIP_FILE:
			fprintf(stderr, "this file is not a zzip file"); break;
		case UNSUPPORTED_VERSION:
			fprintf(stderr, "unsupported compression method"); break;
		case NOT_ENOUGH_MEMORY:
			fprintf(stderr, "not enough memory"); break;
		case CANNOT_CLOSE_INPUT_FILE:
			fprintf(stderr, "cannot close input file"); break;
		case CANNOT_CLOSE_OUTPUT_FILE:
			fprintf(stderr, "cannot close output file"); break;
		case CANNOT_OPEN_INPUT_FILE:
			fprintf(stderr, "cannot open input file"); break;
		case CANNOT_OPEN_OUTPUT_FILE:
			fprintf(stderr, "cannot open output file"); break;
		default: 
			fprintf(stderr, strerror(errno)); break;
		}
		fprintf(stderr, "\n");
		return 1;
	}
	printf(" done\n");
	return 0;
}
