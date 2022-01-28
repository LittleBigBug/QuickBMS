/*---------------------------------------------*/
/* Zzip/Zzlib compressor              make_h.c */
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

#include <stdio.h>

int main(int  argc, 
		 char **argv)
{
	FILE *f;
	unsigned char tab[65536];
	int nb = 0, i;

	if (argc == 1) return -1;

	f = fopen(argv[1], "rb");
	if (f != NULL)
	{
		nb = fread(tab, 1, 65536, f);
		fclose(f);
	}	

	printf("#ifndef SFX_CODE\n");
	printf("#define SFX_CODE\n");
	printf("#define SFX_CODE_SIZE %dUL\n\n", nb);
	printf("#ifndef SFX\n");
	printf("static unsigned char sfx_code[] = {");

	for(i = 0; i < nb-1; ++i)
	{
		if ((i & 31) == 0) printf("\n");
		printf("%d,", tab[i]);
	}

	printf("%d\n};\n", tab[i]);
	printf("#endif /* !SFX */\n");
	printf("#endif /* SFX_CODE */\n\n");

	return 0;
}
