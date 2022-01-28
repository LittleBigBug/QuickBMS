/*-------------------------------------------------*/
/* GRZipII/libGRZip compressor           GRZipII.c */
/* GRZipII file to file compressor                 */
/*-------------------------------------------------*/

/*--
  This file is a part of GRZipII and/or libGRZip, a program
  and library for lossless, block-sorting data compression.

  Copyright (C) 2002-2004 Grebnov Ilya. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  Grebnov Ilya, Ivanovo, Russian Federation.
  Ilya.Grebnov@magicssoft.ru, http://magicssoft.ru/

  This program is based on (at least) the work of:
  Juergen Abel, Jon L. Bentley, Edgar Binder,
  Charles Bloom, Mike Burrows, Andrey Cadach,
  Damien Debin, Sebastian Deorowicz, Peter Fenwick,
  George Plechanov, Michael Schindler, Robert Sedgewick,
  Julian Seward, David Wheeler, Vadim Yoockin.

  Normal compression mode:
    Compression     memory use : [9-11]*BlockLen + 1Mb
    Decompression   memory use : 7*BlockLen      + 1Mb
  Fast compression mode:
    Compression     memory use : 7*BlockLen      + 1Mb
    Decompression   memory use : 7.125*BlockLen  + 1Mb

  For more information on these sources, see the manual.
--*/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "grzip.h"
#include "intl.h"

#include "main.c"

/*-----------------------------------------------*/
/* End                                   grzip.c */
/*-----------------------------------------------*/
