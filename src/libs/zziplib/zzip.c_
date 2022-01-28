/*---------------------------------------------*/
/* Zzip/Zzlib compressor                zzip.c */
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

#include <sys/stat.h>
#include <sys/timeb.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#ifdef WIN32
# include <io.h>
#else  /* WIN32 */
# include <sys/types.h>
# include <unistd.h>
# include <dirent.h>
#endif /* WIN32 */

#include "zzip.h"

/*---------------------------------------------*/

#ifdef WIN32
# define  SEP_PATH '\\'
# define  STAR_STR "\\*.*"
#else  /* WIN32 */
# define  SEP_PATH '/'
# define  STAR_STR "/*"
#endif /* WIN32 */

bool verbose = true;

static char    **file_list = NULL, *input_filename = NULL, *output_filename = NULL;
static bool    with_path = false;
static uint32  nb_input_file = 0;
static actions action = NONE;

#ifdef SFX
 static char    *exe_filename = NULL;
#else  /* SFX */
 static uint32  block_size = DEFAULT_BLOCK_SIZE;
 static bool    recurse = false;
 static uint    compression_mode = 0;
 static bool    multimedia_test = false;
#endif /* SFX */

#define IO_ERROR() { last_error = errno; return; }

/*---------------------------------------------*/

static
void PrintVersion()
{
#ifdef SFX
	printf(
		"*** ZZIP " VERSION_STRING ", SelF-eXtracting archive\n"
		"*** Copyright (c)2001 Damien Debin, all rights reserved.\n"
		);
#else  /* SFX */
	printf(
		"*** ZZIP " VERSION_STRING ", yet another block-sorting compressor.\n"
		"*** Copyright (c)2001 Damien Debin, all rights reserved.\n"
		);
#endif /* SFX */
}

/*---------------------------------------------*/

INLINE static
void MyFree(void *p)
{
	free(p);
}

INLINE static
void *MyMalloc(uint32 size)
{
	void* m = malloc(size);
	if (m == NULL) last_error = NOT_ENOUGH_MEMORY;
	return m;
}

/*---------------------------------------------*/

#ifdef GET_STAT

static
uint GetCPUIDflags()
{
	uint return_value = 0;

	asm (
	"pushfl;"						/* get extended flags				*/
	"popl	%%eax;"				/* in eax							*/
	"movl	%%eax,%%ecx;"		/* save origional extended flags	*/
	"xorl	$0x00200000,%%eax;"	/* flip id bit 21					*/
	"pushl	%%eax;"				/* save modified flags				*/
	"popfl;"						/* put modified flags in eflag reg	*/
	"pushfl;"						/* get flags again					*/
	"popl	%%eax;"				/* modified flags back to eax		*/
	"pushl	%%ecx;"
	"popfl;"
	"xorl	%%eax,%%ecx;"		/* if 0, bit 21 was set				*/
	"xorl	%%eax,%%eax;"
	"andl	$0x00200000,%%ecx;"
	"jz		0;"
	"movl	$0x00000001,%%eax;"	/* CPUID function 1					*/
	"cpuid;"						/* get signature/std feature flgs	*/
	
	"movl	$0x00000001,%%eax;"

	/* Check for time stamp counter support */

	"movl	$0x00000010,%%ecx;"	/* bit 4 indicates TSC support		*/
	"andl	%%edx,%%ecx;"		/* supports TSC ? CPUID_STD_TSC:0	*/
	"negl	%%ecx;"				/* supports TSC ? CY : NC			*/
	"sbb		%%ecx,%%ecx;"		/* supports TSC ? 0xffffffff:0		*/
	"andl	$0x00000010,%%ecx;"	/* supports TSC ? FEATURE_TSC:0		*/
	"orl		%%ecx,%%eax;"		/* merge into feature flags			*/

	/* Check for MMX support */

	"movl	$0x00800000,%%ecx;"	/* bit 23 indicates MMX support		*/
	"andl	%%edx,%%ecx;"		/* supports MMX ? CPUID_STD_MMX:0	*/
	"negl	%%ecx;"				/* supports MMX ? CY : NC			*/
	"sbb		%%ecx,%%ecx;"		/* supports MMX ? 0xffffffff:0		*/
	"andl	$0x00000020,%%ecx;"	/* supports MMX ? FEATURE_MMX:0		*/
	"orl		%%ecx,%%eax;"		/* merge into feature flags			*/

	/* Check for CMOV support */

	"movl	$0x00008000,%%ecx;"	/* bit 15 indicates CMOV support	*/
	"andl	%%edx,%%ecx;"		/* supports CMOV?CPUID_STD_CMOV:0	*/
	"negl	%%ecx;"				/* supports CMOV ? CY : NC			*/
	"sbb		%%ecx,%%ecx;"		/* supports CMOV ? 0xffffffff:0		*/
	"andl	$0x00000040,%%ecx;"	/* supports CMOV ? FEATURE_CMOV:0	*/
	"orl		%%ecx,%%eax;"		/* merge into feature flags			*/

"0:	" : "=a" (return_value) : : "ebx", "ecx", "edx");

	return return_value;
}

static
void GetCpuSpeed()
{
	struct _timeb timeStart, timeStop;
	uint64 StartTicks, EndTicks, TotalTicks;

	do _ftime(&timeStart);
	while (timeStart.millitm > 700);

	GET_TSC(StartTicks);

	do _ftime(&timeStop);
	while ((timeStop.millitm - timeStart.millitm) < 200);

	GET_TSC(EndTicks);
	
	TotalTicks = EndTicks - StartTicks;
	
	time_stat.cpuspeed = /*300;*/ TotalTicks / 200000; /* CPU speed (Mhz) */
	time_stat.cpuspeed *= 1000000;            /* cycles for 1 second */
}

#endif /* GET_STAT */

/*---------------------------------------------*/

static
void Help()
{
	printf("\n");

	PrintVersion();

#ifndef SFX
	printf(
		"\n usage: zzip [<command>] [<sw1><sw2>...] [<archive>] file1 file2 ...\n\n"
		" <commands>\n"
		"   a  Add files to archive\n"
		"   e  Extract files from archive\n"
		"   x  eXtract files with full path\n"
		"   d  Delete files from archive\n"
		"   t  Test files in archive\n"
		"   l  List contents of archive\n\n"
		" <switches>\n"
		"  -...  Block size (e.g. -3m,-3072k)\n"
		"  -a    Adaptive block size reduction\n"
		"  -mx   MaXimum compression\n"
		"  -mm   MultiMedia compression\n"
		"  -q    Quiet output to screen\n"
		"  -r    Recurse subdirectories\n"
		"  -s    Statistics\n\n"
		" http://debin.org/zzip/\n"
		" damien@debin.net\n"
		);
#else  /* !SFX */
	printf(
		"\n usage: %s [<sw1><sw2>...] [files_to_extract]\n\n"
		" <switches>\n"
		"  -x  eXtract files without path\n"
		"  -t  Test files in archive\n"
		"  -l  List contents of archive\n"
		"  -q  Quiet output to screen\n"
		, exe_filename);
#endif /* !SFX */
	exit(1);
}

/*---------------------------------------------*/

static
void Options(char *s)
{
	while (*(++s) != '\0')
		switch (*s)
		{
#ifndef SFX
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			{
				char *ss = s;
				while ((*s >= '0') && (*s <= '9')) s++;
				if (*s == 'm')
				{ *s = 0; block_size = atoi(ss) * 1024 * 1024; }
				else if (*s == 'k')
				{ *s = 0; block_size = atoi(ss) * 1024; }
				else Help();
			} 
			break;
		case 'b': /* for compatibility with older versions */
			s++;
			{
				char *ss = s, c;
				while ((*s >= '0') && (*s <= '9')) s++;
				c = *s;
				*s = 0; 
				block_size = atoi(ss) * 256 * 1024;
				*s = c;
				s--;
			} 
			break;
		case 'a': 
			compression_mode |= 2;
			break;
		case 'm': 
			s++; 
			switch (*s)
			{
			case 'm': multimedia_test = true; break;
			case 'x': compression_mode |= 1; break;
			default : Help();
			} 
			break;
		case 'r': 
			recurse = true;
			break;
#ifdef GET_STAT
		case 's': 
			time_stat.t_stamp = ((GetCPUIDflags() & FEATURE_TSC) == FEATURE_TSC);
			break;
#endif /* GET_STAT */
#else  /* !SFX */
		case 't': 
			action = TEST;
			break;
		case 'l': 
			action = LIST;
			break;
		case 'x': 
			with_path = false;
			break;
#endif /* !SFX */
		case 'q': 
			verbose = false; 
			break;
		case '-': 
			Options(s);
			break;
		default :
			Help();
			break;
		}
}

/*---------------------------------------------*/

static
void CmdLineParameters(int  argc, 
					   char **argv)
{
	file_list = (char**)MyMalloc(sizeof(char*) * MAX_FILES);

	verbose = true;
	nb_input_file = 0;

#ifdef SFX

	{
		sint l;
		action = EXTRACT;
		with_path = true;

		exe_filename = argv[0];
		l = strlen(exe_filename) - 4;
		if (l < 0 || strcmp(exe_filename + l, ".exe") != 0)
		{
			exe_filename = (char*)MyMalloc(sizeof(char) * FILENAME_LENGTH_MAX);
			strcpy(exe_filename, argv[0]);
			strcat(exe_filename, ".exe");
		}

		file_list[nb_input_file++] = exe_filename;
	}

#else  /* SFX */

	with_path = false;
	compression_mode = 0;
	recurse = false;
	block_size = DEFAULT_BLOCK_SIZE;
	action = NONE;

	if (argc > 1)
	{
		if (strlen(argv[1]) == 1)
		{
			switch (*argv[1])
			{
			case 'A':
			case 'a': action = CREATE; argc--; argv++;  break;
			case 'D':
			case 'd': action = DELETE; argc--; argv++;  break;
			case 'X':
			case 'x': with_path = true;
			case 'E':
			case 'e': action = EXTRACT; argc--; argv++; break;
			case 'L':
			case 'l': action = LIST; argc--; argv++;    break;
			case 'T':
			case 't': action = TEST; argc--; argv++;    break;
			default :;
			}
		}
	}
	else Help();

#endif /* SFX */

	while ((argc > 1) && (*argv[1] == '-'))
	{
		Options(argv[1]);
		argc--;
		argv++;
	}

	while (argc > 1)
	{
		file_list[nb_input_file++] = argv[1];
		argc--;
		argv++;
	}

#ifndef SFX
	if (nb_input_file == 0) Help();
#endif /* !SFX */
}

/*---------------------------------------------*/

#ifndef SFX
/* use 'file.zz' if 'file.zz' exists and if 'file' doesn't exist */
static
void CheckFilename(char *result, 
				   char *filename)
{
    struct stat buf_stat;

	strcpy(result, filename);
	strcat(result, ".zz");
	if (stat(filename, &buf_stat) != -1 || stat(result, &buf_stat) == -1)
		strcpy(result, filename);
}
#endif /* !SFX */

/*---------------------------------------------*/

static
void DoFiles()
{
#ifndef SFX
	bool		single_archive;
    struct stat	file_stat;
	uint32		nb_archive;
#endif  /* !SFX */
	uint32		i, j;
	sint		l;
	info_s		*info;

	output_filename		= (char*)MyMalloc(sizeof(char) * FILENAME_LENGTH_MAX);
	*output_filename	= '\0';
	input_filename		= (char*)MyMalloc(sizeof(char) * FILENAME_LENGTH_MAX);
	info				= (info_s*)MyMalloc(sizeof(info_s));

#ifndef SFX
	if (action == NONE)
	{
		l = strlen(file_list[0]) - 3;
		if (l >= 0 && strcmp(file_list[0] + l, ".zz") == 0) action = EXTRACT;
	}

	if ((nb_input_file <= 1) && ((action == DELETE) || (action == CREATE))) Help();
#endif /* !SFX */

	VERBOSE PrintVersion();

	switch (action)
	{
#ifndef SFX
	case UPDATE:
	case CREATE:
		/* first filename is archive filename */
		strcpy(output_filename, file_list[0]);
		file_list++;
		nb_input_file--;
	case NONE:
		/* we process readable files only and recurse directories */
		j = 0;
		for (i = 0; i < nb_input_file; ++i)
		{
			bool go_through = false;
			char *path;
			
			path = (char*)MyMalloc(sizeof(char) * (strlen(file_list[i]) + FILENAME_LENGTH_MAX));
			strcpy(path, file_list[i]);

			if (strchr(file_list[i], '*') != NULL) go_through = true;
			else
			{
				if (stat(file_list[i], &file_stat) == -1) IO_ERROR();
				
				if (recurse == true && (file_stat.st_mode & (S_IFDIR | S_IREAD)) == (S_IFDIR | S_IREAD))
				{
					strcat(path, STAR_STR);
					go_through = true;
				}
			}

			if (go_through == true)
			{
				char *p = strrchr(path, SEP_PATH);
				if (p == NULL) p = path;
				else p++;
#ifdef WIN32
				{
					struct _finddata_t file_info;
					sint handle;

					if ((handle = _findfirst(path, &file_info)) == -1) IO_ERROR();
					do
					{
						if (strcmp(file_info.name, ".") != 0 && strcmp(file_info.name, "..") != 0)
						{
							*p = '\0';
							strcat(path, file_info.name);
							file_list[nb_input_file++] = strdup(path);
						}
					}
					while (_findnext(handle, &file_info) == 0);
					_findclose(handle);
				}
#else  /* WIN32 */
				{
					DIR				*dirp;
					struct dirent	*entry;

					if ((dirp = opendir(path)) == NULL) IO_ERROR();
					while ((entry = readdir(dirp)) != NULL)
					{
						if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
						{
							*p = '\0';
							strcat(path, entry->d_name);
							file_list[nb_input_file++] = strdup(path);
						}
					}
					if (closedir(dirp) != 0) IO_ERROR();
				}
#endif /* WIN32 */
			}
			else if ((file_stat.st_mode & (S_IFDIR | S_IREAD)) == S_IREAD)
			{
				file_list[j++] = file_list[i];
			}

			MyFree(path);
		}
		nb_input_file = j;

		/* if no output file, each input file is compressed into a 'single-archive' */
		if (*output_filename == '\0') 
		{
			nb_archive = nb_input_file;
			nb_input_file = 1;
			single_archive = true;
		}
		/* otherwise we build only one archive with every input file */
		else 
		{
			nb_archive = 1;
			/* we append extension '.zz' to the filename if it doesn't exist */
			l = strlen(output_filename) - 3;
			if (l < 0 || strcmp(output_filename + l, ".zz") != 0) 
				strcat(output_filename, ".zz");
			single_archive = false;
		}

		for (j = 0; j < nb_archive; ++j)
		{
			if (single_archive == true)
			{
				strcpy(output_filename, file_list[j]);
				strcat(output_filename, ".zz");
			}
			
			if (access(output_filename, 0) == -1)
			{ 
				/* we create a new archive */
				OpenArchive(CREATE, output_filename, info);
				if (last_error != OK) return;
				VERBOSE printf("\ncreating archive: %s", output_filename);
			}
			else
			{ /* we add to an existing archive */
				OpenArchive(UPDATE, output_filename, info);
				if (last_error != OK) return;
				VERBOSE printf("\nupdating archive: %s", output_filename);
			}

			/* output to screen formating */
			{
				char temp[32];
				sprintf(temp, "%d", nb_input_file);
				l = strlen(temp);
			}

			for (i = 0; i < nb_input_file; ++i)
			{
				input_filename = file_list[i + j];

				VERBOSE printf("\n  %0*u/%u %s ", l, i + 1, nb_input_file, input_filename);

				AddFile(input_filename, compression_mode, multimedia_test, block_size, info);
				if (last_error != OK) return;

				VERBOSE
				{
					if (info->input_size != 0) printf("%+#05.1f%%", MIN(((float)(info->output_size) * 100) / info->input_size - 100, 0));
					else printf("N/A");
				}
			}

			CloseArchive(info);
			if (last_error != OK) return;

			VERBOSE
			{
				printf("\n\n%d file(s) added,", nb_input_file);
				if (info->input_size != 0) printf(" %+#05.1f%%", MIN(((float)(info->output_size) * 100) / info->input_size - 100, 0));
				else printf(" N/A");
				printf(" (%u->%u)\n", info->input_size, info->output_size);
			}
		}
		break;
#endif /* !SFX */
	case EXTRACT:
		{
			uint   nbfile, nbfile_extracted = 0;
			info_s **all_files_info = NULL;
			
#ifndef SFX
			CheckFilename(input_filename, file_list[0]);
			VERBOSE printf("\nextracting archive: %s", input_filename);
#else  /* !SFX */
			input_filename = file_list[0];
#endif /* !SFX */
			file_list++;
			
			OpenArchive(EXTRACT, input_filename, info);
			if (last_error != OK) return;
			
			nbfile = (nb_input_file > 1) ? nb_input_file - 1 : info->nb_of_file;

			all_files_info = (info_s**)MyMalloc(sizeof(info_s*) * info->nb_of_file);
			if (last_error != OK) return;

			for (j = 0; j < info->nb_of_file; ++j)
				all_files_info[j] = (info_s*)MyMalloc(sizeof(info_s));
			if (last_error != OK) return;

			ListAllFile(all_files_info);
			if (last_error != OK) return;

			SetArchivePointer(NULL, NULL);
			if (last_error != OK) return;

			/* output to screen formating */
			{
				char temp[32];
				sprintf(temp, "%d", nbfile);
				l = strlen(temp);
			}
			
			for (i = 0; i < nbfile; ++i)
			{
				char *name;

				if (nb_input_file > 1) 
				{
					name = file_list[i];
					SetArchivePointer(file_list[i], NULL);
					if (last_error == FILE_NOT_FOUND) 
					{ 
						VERBOSE printf("\n  %0*u/%u %s not found", l, i+1, nbfile, name);
						last_error = OK;
						continue;
					}
					if (last_error != OK) return;
				}
				else
				{
					name = all_files_info[i]->filename;
				}

				VERBOSE printf("\n  %0*u/%u %s", l, i+1, nbfile, name);

				ExtractNextFile(NULL, with_path, info);
				if (last_error != OK) return;

				nbfile_extracted++;
				
				VERBOSE printf(" done");
			}
			
			CloseArchive(info);
			if (last_error != OK) return;
			
			VERBOSE
			{
				printf("\n\n%d file(s) extracted,", nbfile_extracted);
				if (info->input_size != 0) 
					printf(" %+#05.1f%%", ((float)(info->output_size) * 100) / info->input_size - 100);
				else printf(" N/A");
				printf(" (%u->%u)\n", info->input_size, info->output_size);
			}
		}
		break;
	case LIST :
#ifndef SFX
		for (i = 0; i < nb_input_file; ++i)
#endif /* !SFX */
		{
			uint   nbfile;
			sint   namemax; 
			char   timestr[256], filename[FILENAME_LENGTH_MAX];
			info_s **all_files_info = NULL;

#ifndef SFX
			CheckFilename(input_filename, file_list[i]);
			VERBOSE printf("\nlisting archive: %s\n", input_filename);
#else  /* !SFX */
			input_filename = file_list[0];
#endif /* !SFX */

			OpenArchive(LIST, input_filename, info);
			if (last_error != OK) return;

			nbfile = info->nb_of_file;

			all_files_info = (info_s**)MyMalloc(sizeof(info_s*) * nbfile);
			if (last_error != OK) return;

			for (j = 0; j < nbfile; ++j)
				all_files_info[j] = (info_s*)MyMalloc(sizeof(info_s));
			if (last_error != OK) return;

			ListAllFile(all_files_info);
			if (last_error != OK) return;

			namemax = 0;
			for (j = 0; j < nbfile; ++j)
				if (namemax < (sint)strlen(all_files_info[j]->filename)) namemax = strlen(all_files_info[j]->filename);

			memset(filename, '-', FILENAME_LENGTH_MAX);
			filename[FILENAME_LENGTH_MAX-1] = '\0';
			namemax = MIN(namemax,32);
			namemax = MAX(namemax,8);
			printf(
				"\n%-*s       date & time   original compressed  ratio"
				"\n%.*s ----------------- ---------- ---------- ------",
				namemax, "filename",
				namemax, filename
				);
			
			for (j = 0; j < nbfile; ++j)
			{
				info = all_files_info[j];

				if ((sint)strlen(info->filename) > namemax) 
				{ 
					info->filename += (sint)strlen(info->filename) - namemax;
					info->filename[0] = '.'; 
					info->filename[1] = '.'; 
					info->filename[2] = '.'; 
				}

				strftime(timestr, 255, "%d-%b-%Y %H:%M", gmtime(&info->filetime));

				VERBOSE printf("\n%-*s %s %10d %10d %+#05.1f%%", namemax, info->filename, timestr, info->input_size, info->output_size,
					MIN(((float)(info->output_size) * 100) / info->input_size - 100,0));
			}

			CloseArchive(info);
			if (last_error != OK) return;

			printf("\n\n%d file(s) listed,", nbfile);
			if (info->input_size != 0) printf(" %+#05.1f%%", MIN(((float)(info->output_size) * 100) / info->input_size - 100,0));
			else printf(" N/A");
			printf(" (%u->%u)\n", info->input_size, info->output_size);
		}
		break;
	case TEST :
#ifndef SFX
		for (i = 0; i < nb_input_file; ++i)
#endif /* !SFX */
		{
			info_s **all_files_info = NULL;

#ifndef SFX
			CheckFilename(input_filename, file_list[i]);
			VERBOSE printf("\ntesting archive: %s", input_filename);
#else  /* !SFX */
			input_filename = file_list[0];
#endif /* !SFX */

			OpenArchive(TEST, input_filename, info);
			if (last_error != OK) return;

			all_files_info = (info_s**)MyMalloc(sizeof(info_s*) * info->nb_of_file);
			if (last_error != OK) return;

			for (j = 0; j < info->nb_of_file ; ++j)
				all_files_info[j] = (info_s*)MyMalloc(sizeof(info_s));
			if (last_error != OK) return;

			ListAllFile(all_files_info);
			if (last_error != OK) return;

			SetArchivePointer(NULL, NULL);
			if (last_error != OK) return;

			/* output to screen formating */
			{
				char temp[32];
				sprintf(temp, "%d", info->nb_of_file);
				l = strlen(temp);
			}

			for (j = 0; j < info->nb_of_file; ++j)
			{
				VERBOSE printf("\n  %0*u/%u %s", l, j + 1, info->nb_of_file, all_files_info[j]->filename);

				TestNextFile(info);
				if (last_error != OK) return;

				VERBOSE printf(" CRC OK");
			}
			
			CloseArchive(info);
			if (last_error != OK) return;

			VERBOSE printf("\n\n%d file(s) tested\n", info->nb_of_file);
		}
		break;
#ifndef SFX
	case DELETE: 
		{
			uint nbfile, nb_deleted = 0;
			
			CheckFilename(input_filename, file_list[0]);
			file_list++;
			
			VERBOSE printf("\nprocessing archive: %s", input_filename);
			
			OpenArchive(DELETE, input_filename, info);
			if (last_error != OK) return;
			
			nbfile = (nb_input_file > 1) ? nb_input_file - 1 : info->nb_of_file;

			/* output to screen formating */
			{
				char temp[32];
				sprintf(temp, "%d", nbfile);
				l = strlen(temp);
			}
			
			for (i = 0; i < nbfile; ++i)
			{
				SetArchivePointer(file_list[i], NULL);

				if (last_error == FILE_NOT_FOUND)
				{
					last_error = OK;
					VERBOSE printf("\n  %0*u/%u %s not found", l, i+1, nbfile, file_list[i]);
				}
				else
				{
					if (last_error != OK) return;
					
					VERBOSE printf("\n  %0*u/%u deleting %s", l, i+1, nbfile, file_list[i]);

					DeleteNextFile();
					if (last_error != OK) return;

					VERBOSE printf(" done");
					
					nb_deleted++;
				}
			}
			
			CloseArchive(info);
			if (last_error != OK) return;

			VERBOSE printf("\n\n%d file(s) deleted\n", nb_deleted);
		}
		break; 
#endif /* !SFX */
	default:;
	}
}

/*---------------------------------------------*/

#ifdef GET_STAT

#define FORMAT1(a)   (float)(a)/(time_stat.cpuspeed>>16), (float)(a)*100/time_stat.time_tot
#define FORMAT2(a,b) (float)(a)*100/(b)
#define FORMAT3(a)   (float)(a)/(time_stat.cpuspeed>>16)
#define FORMAT4(a,b) (float)(a)*((float)(time_stat.cpuspeed>>16)/(1024))/(b)

static
void Print_Stat()
{
	uint32 bwt = time_stat.time_bwt1 + time_stat.time_bwt2 + time_stat.time_bwt3 + time_stat.time_bwt4;
	uint32 st  = time_stat.time_st0  + time_stat.time_st1;
	uint32 misc= time_stat.time_tot - bwt - st - time_stat.time_mtf - time_stat.time_rle - time_stat.time_txt - time_stat.time_mm - time_stat.time_ana;

	GetCpuSpeed();
	printf("\n(CPU speed : %d Mhz)\n", time_stat.cpuspeed / 1000000);

	if (time_stat.time_ana != 0)
	  printf("\nAnalyzing         = %#7.3fs (%#4.1f%%) %#8.1f kb/s", FORMAT1(time_stat.time_ana), FORMAT4(time_stat.kb_ana,time_stat.time_ana));
	if (time_stat.time_rle != 0)
	  printf("\nRLE (de)coding    = %#7.3fs (%#4.1f%%) %#8.1f kb/s", FORMAT1(time_stat.time_rle), FORMAT4(time_stat.kb_rle,time_stat.time_rle));
	if (time_stat.time_mm != 0)
	  printf("\nMmedia (de)coding = %#7.3fs (%#4.1f%%) %#8.1f kb/s", FORMAT1(time_stat.time_mm), FORMAT4(time_stat.kb_mm,time_stat.time_mm));
	if (time_stat.time_txt != 0)
	  printf("\nTxt/Bin filtering = %#7.3fs (%#4.1f%%) %#8.1f kb/s", FORMAT1(time_stat.time_txt), FORMAT4(time_stat.kb_txt,time_stat.time_txt));
	if (bwt != 0)
	{
		printf("\n(un)BWT           = %#7.3fs (%#4.1f%%) %#8.1f kb/s", FORMAT1(bwt), FORMAT4(time_stat.kb_bwt,bwt));
		VERBOSE if (time_stat.time_bwt4 != 0) printf(" (%.1f%%, %.1f%%, %.1f%%, %.1f%%)", 
			FORMAT2(time_stat.time_bwt1,bwt),FORMAT2(time_stat.time_bwt2,bwt),
			FORMAT2(time_stat.time_bwt3,bwt),FORMAT2(time_stat.time_bwt4,bwt));
	}
	if (time_stat.time_mtf != 0)
	  printf("\nMTF (de)coding    = %#7.3fs (%#4.1f%%) %#8.1f kb/s", FORMAT1(time_stat.time_mtf), FORMAT4(time_stat.kb_mtf,time_stat.time_mtf));
	if (st != 0)
	{
		printf("\nArith. (de)coding = %#7.3fs (%#4.1f%%) %#8.1f kb/s", FORMAT1(st), FORMAT4(time_stat.kb_st0+time_stat.kb_st1,st));
		VERBOSE printf(" (%.1f%%, %.1f%%)", 
			FORMAT2(time_stat.time_st0,st), FORMAT2(time_stat.time_st1,st));
	}
	if (misc != 0)
	  printf("\nMisc (i/o,crc...) = %#7.3fs (%#4.1f%%) %#8.1f kb/s", FORMAT1(misc), FORMAT4(time_stat.kb_tot,misc));
	printf("\n\nTotal             = %#7.3fs         %#8.1f kb/s\n", FORMAT3(time_stat.time_tot), FORMAT4(time_stat.kb_tot,time_stat.time_tot));
}

#endif /* GET_STAT */

/*---------------------------------------------*/

int main(int  argc, 
		 char **argv)
{
#ifdef GET_STAT
	uint64 p1, p2;
#endif /* GET_STAT */

	CmdLineParameters(argc, argv);

	GET_TSC(p1);
	DoFiles();
	GET_TSC(p2);
	STAT_ADD_TIME(time_tot, p2, p1);

#ifdef GET_STAT
	if (time_stat.t_stamp == true) Print_Stat();
#endif /* GET_STAT */

	if (last_error != OK)
	{
		fprintf(stderr, "\nZzip error : ");
		switch (last_error)
		{
#ifndef SFX
		case NOT_A_ZZIP_FILE:
			fprintf(stderr, "this file is not a zzip file"); break;
		case UNSUPPORTED_VERSION :
			fprintf(stderr, "unsupported compression method"); break;
		case FILE_NOT_FOUND:
			fprintf(stderr, "file not found"); break;
		case UNEXPECTED_EOF:
			fprintf(stderr, "unexpected end of file"); break;
#endif /* !SFX */
		case NOT_ENOUGH_MEMORY :
			fprintf(stderr, "not enough memory"); break;
		case CRC_ERROR:
			fprintf(stderr, "CRC check failed"); break;
		case CANNOT_CLOSE_INPUT_FILE:
			fprintf(stderr, "cannot close input file"); break;
		case CANNOT_CLOSE_OUTPUT_FILE:
			fprintf(stderr, "cannot close output file"); break;
		case CANNOT_OPEN_INPUT_FILE:
			fprintf(stderr, "cannot open input file"); break;
		case CANNOT_OPEN_OUTPUT_FILE:
			fprintf(stderr, "cannot open output file"); break;
		case FSEEK_INPUT_FILE:
			fprintf(stderr, "fseek failed on input file"); break;
		case FSEEK_OUTPUT_FILE:
			fprintf(stderr, "fseek failed on output file"); break;
		default : 
			fprintf(stderr, strerror(errno)); break;
		}
		fprintf(stderr, "\n");
		CloseArchive(NULL);
#ifndef SFX
		CleanMemory();
#endif /* !SFX */
		return 1;
	}

	return 0;
}

/*---------------------------------------------*/
/* end                                  zzip.c */
/*---------------------------------------------*/
