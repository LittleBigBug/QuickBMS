/*----------------------------------------------*/
/* GRZipII/libGRZip. Version 0.2.4, 12-Feb-2004 */
/* CopyRight (c) 2002-2004 by Grebnov Ilya.     */
/*----------------------------------------------*/

GRZipII - is a high-performance file compressor based
on Burrows-Wheeler Transform, Schindler Transform,
Move-To-Front and Weighted Frequency Counting.
It uses The Block-Sorting Lossless Data Compression
Algorithm, which has received considerable attention over
recent years for both its simplicity and effectiveness.
This implementation has compression rate of 2.234 bps
on the Calgary Corpus(14 files) without preprocessing filters.

NOTE: GRZipII can't compress/uncompress pkzip/winzip files (*.zip).

libGRZip is a library based on GRZipII, it uses the
same algorithms as GRZipII and enables you to compress
memory block. You are free to use this library in your
program and you are free to distribute this libray
with your program, but you have to mention somewhere
in your program that it uses libGRZip.

This software is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU Lesser General Public
License for more details.

This program is based on (at least) the work of:
Juergen Abel, Jon L. Bentley, Edgar Binder,
Charles Bloom, Mike Burrows, Andrey Cadach,
Damien Debin, Sebastian Deorowicz, Peter Fenwick,
George Plechanov, Michael Schindler, Robert Sedgewick,
Julian Seward, David Wheeler, Vadim Yoockin.

/*---------------------------------------------------*/
/* Grebnov Ilya, Ivanovo, Russian Federation.        */
/* Ilya.Grebnov@magicssoft.ru, http://magicssoft.ru/ */
/*---------------------------------------------------*/



/*---------------------------------------------------*/
/* New versions >= 0.2.5 by Jean-Pierre Demailly     */
/* <demailly@fourier.ujf-grenoble.fr>                */
/*---------------------------------------------------*/

These versions add more support for Linux/Unix systems,
especially through a new command line program 'grzip' which supports
essentially the same options as gzip/bzip2.

A major segfault bug causing frequent crashes (at least on Linux)
has been fixed in 0.2.5. Starting with version 0.2.7, grzip seem to be 
64-bit safe, and produces stable and consistent results on the various
smallendian environments in which it was tested. Grzip compiles on
bigendian processors but does not work there (yet).

The script 'grztar' can be used to produce tarball archives :
   grztar -cvf archive.tar.grz <directory>
or to extract grzipped archives :
   grztar -xvf archive.tar.grz
   
Alternatively, the file build/tar.c.patch is a patch to GNU-tar 
to make it directly work with grzip (by using command line otions 
'tar -cvfy' and 'tar -xvfy' ). 
[Patch tar-x.yz/src/tar.c  recompile, and install...]

Consult CHANGES for more information.
