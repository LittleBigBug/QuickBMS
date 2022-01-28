/* ctw is the main program for encoding and decoding files */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>

#include "ctw-settings.h"
#include "ctwmath.h"
#include "ctwlarc.h"
#include "ctwstep.h"
#include "ctwtree.h"
#include "ctwencdec.h"
#include "ctw-header.h"

/* variables */
unsigned int databytes, codebits;
pathname     in_filename = "";  /* filename of input file */
pathname     out_filename = ""; /* filename of output file */
pathname     logfn = "";        /* filename of log file */
boolean      force = false;	/* do not overwrite existing files */
clock_t      delta_t;
char	     exec_mode;    /* contains 'e' for encode, 'd' for decode, and 'i' for info mode */


/* functions */
void usage(void)
{ 
  printf("\nUsage: ctw e/d/i [-options] <input_filename> [<output_filename>]\n"\
    "Where e = encode, d = decode and i = show file information\n"\
    "For encoding, the default output_filename is input_filename.ctw, and for\n"\
    "decoding, the default output_filename is input_filename.d (without .ctw).\n"\
    "Available options:\n"\
    " -dX : Set maximum tree depth to X\n"\
    " -tX : Set maximum number of tries in tree array\n"\
    " -nX : Set maximun number of nodes (size of the tree array)\n"\
    "       values can be specified using 'M' for millions of nodes and 'K' for \n"\
    "       thousands of nodes (eg. -n4M means 4194304 nodes)\n"\
    "       Tree array requires 8*X bytes of memory\n"\
    " -fX : Set maximum file buffer size; values can be specified using\n"\
    "       'M' for megabytes and 'K' for kilobytes\n"\
    " -bX : Set maximum value of log beta\n"\
    " -s  : Disable strict tree pruning (enabled by default)\n"\
    " -r  : Enable weighting at root nodes (disabled by default)\n"\
    " -k  : Use Krichevski-Trofimov estimator instead of Zero-Redundancy estimator\n"\
    " -y  : Force overwriting of existing files\n"\
    " -lX : Enables logging to file X\n");
  exit(-1);
}


void fatal_error(char *mess, char *opt)
{
  fprintf(stderr, mess, opt);
  exit(1);
}

/* converts string n to a number, with the possibility to use M (for mega) and K (for kilo) */
unsigned int get_number(char *n)
{
  unsigned int j = strlen(n)-1;
  
  if (n[j] == 'M' || n[j] == 'm')
    return atoi(n) << 20;
  else if (n[j] == 'K' || n[j] == 'k')
    return atoi(n) << 10;
  else
    return atoi(n);
}


boolean process_options(int *i, int argc, char *argv[])
{
  /* processes options in argv[*i] if it starts with '-', Processing stops when *i
     reaches argc or when an option not starting with '-' is found.
     Returns true if end of argument list is reached */
  unsigned int j;
  char opt, *number;
  
  while (*i < argc && argv[*i][0] == '-')
  {
    opt = tolower(argv[*i][1]);
    /* check numeric options */
    if (strchr("dtnfb", opt) != NULL)
    {
      if (strlen(argv[*i])>2)
       	number = argv[*i] + 2;
      else if (++(*i) < argc)
       	number = argv[*i];
      else {
        fprintf(stderr, "Warning: missing numeric argument for option -%c, option ignored\n", argv[(*i)-1][1]);
        return true; }
      switch (opt)
      {
      	case 'd': settings.treedepth = atoi(number); break;
        case 't': settings.maxnrtries = atoi(number); break;
        case 'n': settings.maxnrnodes = get_number(number); break;
        case 'f': settings.maxfilebufsize = settings.filebufsize = get_number(number); break;
        case 'b': settings.maxlogbeta = get_number(number); break;
      }
    }
    /* other options */
    else if (opt == 'l') /* logging to file enabled */
    {
      if (strlen(argv[*i])>2)
       	strcpy (logfn, argv[*i] + 2);
      else if (++(*i) < argc)
      	strcpy (logfn, argv[*i]);
      else {
        fprintf(stderr, "Warning: missing filename argument for option -L\n"); return true; }
      printf("Logging to \"%s\" enabled\n", logfn);
    }
    /* check for switches */
    else for (j=1; argv[*i][j]; j++) 
    {
      switch (tolower(argv[*i][j]))
      {
      	/* switches (booleans) */
      	case 's':
          settings.strictpruning = false; break; /* do not use strict pruning in ctwtree.c */
      	case 'r':
      	  settings.rootweighting = true; break;  /* use root weighting in ctwmath.c */
      	case 'k':
      	  settings.use_zeroredundancy = false; break; /* use KT-estimator instead of ZR */
	case 'y':
	  force = true; break; /* force overwriting of existing files */
        default:
          fprintf(stderr, "Warning: invalid switch or option '%c', ignored\n", argv[*i][j]);
      }
    }
    (*i)++;
  }
  return (*i >= argc);
}


/* cu_strcmp returns false if s1 and s2 are the same, true otherwise */
boolean cu_strcmp(char *s1, char *s2)
{
  while (*s1 && *s2)
    if (toupper(*(s1++)) != toupper(*(s2++))) return true;
  if (*s1 || *s2) return true;
  return false;
}


void check_filenames()
{
  FILE  *tmp_file;

  if (!strcmp(in_filename, out_filename))
  {
    fprintf(stderr, "Error: input and output file are the same\n");
    exit(1);
  }

  if ((tmp_file = fopen(out_filename, "r")) != NULL)
  {
    fclose(tmp_file);
    if (!force)
    {
      fprintf(stderr, "\"%s\" already exists and will be replaced.\nAre you sure (y/n)? ", out_filename);
      if (getchar() != 'y')
	exit(1);
    }
  }
}


void show_progress(unsigned int p, unsigned int t)
{
  printf("\b\b\b\b%3u%%", (int)((float)p/t * 100 + 0.5));
  fflush(stdout);
}


void encode_file()
{
  FILE          *unc_file, *cmp_file;
  struct stat	st_datain;
  unsigned char let;
  unsigned int  phase, i, pinterv, pcount;
  clock_t	start_t;

  /* initialize encoding: open files, output settings, write header */
  if (!out_filename[0]) /* output filename not explicitely specified */
  {
    strcpy(out_filename, in_filename);
    strcat(out_filename, ".ctw");
  }

  check_filenames();

  if ((unc_file = fopen(in_filename, "rb")) == NULL)
    fatal_error("Error: can't open \"%s\"\n", in_filename);
  if (stat(in_filename, &st_datain))
    fatal_error("Error: can't find info on \"%s\"\n", in_filename);
  databytes = st_datain.st_size;
 
  if (!check_CTWsettings()) exit(1);
  if (settings.maxfilebufsize > databytes)
    settings.filebufsize = databytes;

  if ((cmp_file = fopen(out_filename, "wb")) == NULL)
    fatal_error("Error: can't open \"%s\"\n", out_filename);

  printf("\nCurrent CTW settings:\n");
  print_CTWsettings(stdout);
  printf("\nEncode %s (%u bytes) to %s:\nInitializing...", in_filename, databytes, out_filename);
  fflush(stdout);
  start_t = clock();

  write_header(cmp_file, databytes);
  arencinit(cmp_file);
  if (!init_filebuffer()) exit(1);

  /* Get the first settings.treedepth + 1 characters and encode each bit with probability
     1/2. These first settings.treedepth + 1 characters are needed as an initial context
     for building the tree structure.	*/
  /* STEPHALF points to probability 0.5 */
  for(nrsymbols = 0; nrsymbols <= settings.treedepth && nrsymbols < databytes; nrsymbols++) {
    let = filebuffer[nrsymbols] = getc(unc_file);
    for(phase = 0; phase < 8; phase++)
      arenc(STEPHALF, ByteBit(let,phase));
  }

  if (nrsymbols < databytes) /* if file is smaller than treedepth, we are already finished */
  {
    if (!init_tree(let)) exit(1);
    init_ctxstring();

    /* start encoding */
    pcount = pinterv = databytes / 100;
    if (pinterv == 0) pcount = pinterv = 1;
    printf("\rEncoding...   0%%"); fflush(stdout);
    for(i = settings.treedepth + 1; i < databytes; i++) 
    {
      let = getc(unc_file);
      Encode(let);
      if ((--pcount)==0) /* after pinterv bytes encoded, the progress indicator will be updated */
      {
        show_progress(i,databytes);
        pcount = pinterv;
      }
    }
  }
  arencexit(&codebits);
  free_memory();
  fclose(cmp_file);
  fclose(unc_file);
  delta_t = clock() - start_t; /* stop CPU time measurement */
  printf("\rFinished.       \n");
}


void decode_file()
{
  FILE          *unc_file, *cmp_file;
  unsigned char let;
  unsigned int  phase, i, pinterv, pcount;
  clock_t	start_t;

  /* initialize decoding: open files, read header, output settings */
  if (!out_filename[0]) /* output filename not explicitely specified */
  {
    strcpy(out_filename, in_filename);
    if ((i = strlen(out_filename)) >= 4)
      if (!cu_strcmp(out_filename + i - 4, ".ctw"))
        out_filename[i - 4] = 0;
    strcat(out_filename, ".d");
  }

  check_filenames();

  if ((cmp_file = fopen(in_filename, "rb")) == NULL)
    fatal_error("Error: can't open \"%s\"\n", in_filename);
  if (!read_header(cmp_file, &databytes)) exit(1);
  if (!check_CTWsettings()) exit(1);

  if ((unc_file = fopen(out_filename, "wb")) == NULL)
    fatal_error("Error: can't open \"%s\"\n", out_filename);
  
  printf("\nCurrent CTW settings:\n");
  print_CTWsettings(stdout);
  printf("\nDecode %s to %s (%u bytes):\nInitializing...", in_filename, out_filename, databytes);
  fflush(stdout);

  start_t = clock();   /* start CPU time measurement */
  ardecinit(cmp_file);
  if (!init_filebuffer()) exit(1);

  /* Decode the bits for the first settings.treedepth + 1 characters with probability
     1/2. These first settings.treedepth + 1 characters are needed as an initial context
     for building the tree structure.	*/
  /* STEPHALF points to probability 0.5 */
  for(nrsymbols = 0; nrsymbols <= settings.treedepth && nrsymbols < databytes; nrsymbols++)
  {
    let = 0;
    for(phase = 0; phase < 8; phase++)
      let = (let << 1) | ardec(STEPHALF);
    filebuffer[nrsymbols] = let;
    putc(let,unc_file);
  }

  if (nrsymbols < databytes) /* if file is smaller than treedepth, we are already finished */
  {
    if (!init_tree(let)) exit(1);
    init_ctxstring();

    /* start decoding */
    pcount = pinterv = databytes / 100;
    if (pinterv == 0) pcount = pinterv = 1;
    printf("\rDecoding...   0%%"); fflush(stdout);
    for(i = settings.treedepth + 1; i < databytes; i++)
    {
      let = Decode();
      putc(let, unc_file);
      if ((--pcount)==0) /* after pinterv bytes decoded, the progress indicator will be updated */
      {
        show_progress(i,databytes);
        pcount = pinterv;
      }
    }
  }
  fclose(unc_file);
  fclose(cmp_file);
  ardecexit(&codebits);
  free_memory();
  delta_t = clock() - start_t; /* stop CPU time measurement */
  printf("\rFinished.       \n");
}


void show_fileinfo()
{
  FILE *cmp_file;
  struct stat st_datain;

  if ((cmp_file = fopen(in_filename, "rb")) == NULL)
    fatal_error("Error: can't open \"%s\"\n", in_filename);
  if (stat(in_filename, &st_datain))
    fatal_error("Error: can't find info on \"%s\"\n", in_filename);
  if (!read_header(cmp_file, &databytes)) exit(1);
  check_CTWsettings();
  printf("\nFile: %s\nCompressed filesize: %u bytes\nUncompressed filesize: %u bytes\n\n"\
  	 "CTW settings used for encoding:\n", in_filename, st_datain.st_size, databytes);
  print_CTWsettings(stdout);
  fclose(cmp_file);
} 


void print_stats(FILE *out)
{ 
  fprintf(out, "\nStatistics:\n#codebits        = %u\n", codebits);
  fprintf(out, "#treenodes       = %u\n", nrnodes);
  fprintf(out, "#failed          = %u\n", nrfailed);
  fprintf(out, "processing time  = %.1f seconds\n", (double)delta_t / CLOCKS_PER_SEC);
  if (exec_mode == 'e')
    fprintf(out, "compression-rate = %5.5f bits/byte\n", (double)codebits / databytes);
  else
    fprintf(out, "decompression-rate = %5.5f bytes/bit\n", (double)databytes / codebits);
}


void write_logfile()
{
  FILE *log;
  if ((log = fopen(logfn, "a")) == NULL)
  {
    fprintf(stderr, "Error: can't open logfile \"%s\"\n", logfn);
    return;
  }
  fprintf(log, "\n%s file: %s (%u bytes)\n\nSettings:\n", exec_mode == 'e'? "Encode" : "Decode",
          in_filename, databytes);
  print_CTWsettings(log);
  print_stats(log);
  fprintf(log, "\n------------------------------------------------------------\n");
  fclose(log);
}


int main(int argc, char *argv[])
{
  int  i;
 
  printf("CTW (Context Tree Weighting) compression/decompression utility version 0.1\n");
 
  /* initialize settings to default values (can be adjusted by command line options) */
  init_CTWsettings();
  
  /* process command line options */
  if (argc<3) usage();
  i=1;
  if (process_options(&i, argc, argv) || (argv[i][0] != 'e' && argv[i][0] != 'd' && argv[i][0] != 'i'))
    usage();
  exec_mode = argv[i++][0];
  if (process_options(&i, argc, argv)) usage();
  strcpy(in_filename, argv[i++]);
  if (!process_options(&i, argc, argv))
  {
    /* a second filename argument is specified, which is the output file */
    strcpy(out_filename, argv[i++]);
    while (!process_options(&i, argc, argv))
      fprintf(stderr, "Warning: invalid argument \"%s\", ignored.\n", argv[i++]);
  }

  
  if (exec_mode == 'i')
    /* only show information on file */
    show_fileinfo();
  else
  {
    /* start encoding / decoding */
    if (exec_mode == 'e')
      encode_file();
    else
      decode_file();

    /* write statistics */
    print_stats(stdout);
  
    if (logfn[0])
      write_logfile();
  }

  return 0;
}