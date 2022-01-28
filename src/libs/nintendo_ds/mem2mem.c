#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CMD_DECODE

    #define Memory  calloc
    #define Load    nintendo_ds_Load
    #define Save    nintendo_ds_Save
    unsigned char *nintendo_ds_Load(char *filename, int *length, int min, int max);
    void  nintendo_ds_Save(char *filename, unsigned char *buffer, int length);

#else

    unsigned char    *nintento_ds_in     = NULL;
    int              nintento_ds_insz    = 0;
    unsigned char    *nintento_ds_out    = NULL;
    int              nintento_ds_outsz   = 0;

    unsigned char *nintendo_ds_Load(char *filename, int *length, int min, int max) {
        *length = nintento_ds_insz;
        return nintento_ds_in;
    }

    void  nintendo_ds_Save(char *filename, unsigned char *buffer, int length) {
        if((length >= 0) && (length <= nintento_ds_outsz)) {
            nintento_ds_outsz = length;
            memcpy(nintento_ds_out, buffer, nintento_ds_outsz);
        } else {
            nintento_ds_outsz = -1;
        }
    }

    int nintendo_ds_set_inout(unsigned char *in, int insz, unsigned char *out, int outsz) {
        int     ret = nintento_ds_outsz;
        //nintento_ds_in      = in;
        nintento_ds_in      = malloc(insz); // because the code frees the input memory
        memcpy(nintento_ds_in, in, insz);
        nintento_ds_insz    = insz;
        nintento_ds_out     = out;
        nintento_ds_outsz   = outsz;
        return ret;
    }

#endif
