/* ctw-settings.c : contains functions for keeping settings. */

#include <stdio.h>
#include "ctw-settings.h"

struct CTWsettings settings;

void init_CTWsettings()
{
  /* init standard ctwtree settings */
  settings.treedepth = 6;
  settings.maxnrnodes = 4194304; /* tree array size in bytes is 8 * maxnrnodes */
  settings.maxnrtries = 32;
  settings.filebufsize = settings.maxfilebufsize = 4194304;
  settings.strictpruning = true;
 
  /* init standard ctwmath settings */
  /* maxlogbeta = 1024 is chosen as default value. According to the formula,
     maxlogbeta = ACCURACY*(M+1)*2 is the bound on the absolute value of log
     beta, so MAXLOGBETA should be 2304. But it appeared that 1024
     is a slightly better choice [EIDMA par 7.1.2] */
  settings.maxlogbeta = 1024;  
  settings.rootweighting = false;
  settings.use_zeroredundancy = true;
}


/* local function to check min and max value of n. if checkpow2 is true n is checked if it is
   a power of 2. returns 1 if an error occurs, otherwise 0 */
byte check_setting(unsigned int n, unsigned int min, unsigned int max, boolean checkpow2, char *name)
{
  if (n < min || n > max)
  {
    fprintf(stderr, "Error: invalid value of %s (min. value %u, max. value %u%s)\n",
            name, min, max, checkpow2? ",\n       must be a power of 2": "");
    return 1;
  }
  if (checkpow2)
  {
    unsigned int cnt;
    for (cnt=0; n; n >>= 1)
      if (n & 1) cnt++;
    if (cnt != 1)
    {
      fprintf(stderr, "Error: value of %s must be a power of 2\n", name);
      return 1;
    }
  }
  return 0;
}
	

boolean check_CTWsettings()
{
  byte nr_errors =
   check_setting(settings.treedepth, MIN_TREEDEPTH, MAX_TREEDEPTH, false, "tree depth") +
   check_setting(settings.maxnrtries, MIN_MAXNRTRIES, MAX_MAXNRTRIES, false, "number of tries") +
   check_setting(settings.maxnrnodes, MIN_MAXNRNODES, MAX_MAXNRNODES, true, "number of nodes") +
   check_setting(settings.maxfilebufsize, MIN_FILEBUFSIZE, MAX_FILEBUFSIZE, true, "max. file buffer size") +
   check_setting(settings.maxlogbeta, MIN_MAXLOGBETA, MAX_MAXLOGBETA, true, "max. log beta");
  return nr_errors == 0;
}


void print_CTWsettings(FILE *out)
{
  fprintf(out, "Tree depth: %u\n", settings.treedepth);
  fprintf(out, "Size of tree array: %u nodes (%u bytes)\n", settings.maxnrnodes, settings.maxnrnodes*8);
  fprintf(out, "Max. number of tries in tree array: %u\n", settings.maxnrtries);
  fprintf(out, "File buffer size: %u bytes (max. %u)\n", settings.filebufsize, settings.maxfilebufsize);
  fprintf(out, "Strict pruning: %s\n", settings.strictpruning? "enabled" : "disabled");
  fprintf(out, "Root weighting: %s\n", settings.rootweighting? "enabled" : "disabled");
  fprintf(out, "Max. log beta: %u\n", settings.maxlogbeta);
  fprintf(out, "Estimator: %s\n", settings.use_zeroredundancy? "Zero-Redundancy" : "Krichevski-Trofimov");
}