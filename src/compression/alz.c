// modified by Luigi Auriemma

/*
Nisto
https://github.com/Nisto/bio-tools/tree/master/bio0/alz-tool
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint8_t *buf;
    uint32_t offset;
    uint32_t size;
    uint8_t bitnum;
} alz_bitstream;

#define ALZ_LSB_MASK(nbits) ((1UL << (nbits)) - 1)

uint8_t alz_get_bit(alz_bitstream *bs)
{
    uint8_t bit;

    if (bs->offset >= bs->size)
    {
        printf("ERROR: read_bit() buffer overread\n");
        exit(EXIT_FAILURE);
    }

    bit = (bs->buf[bs->offset] >> bs->bitnum) & 1;

    if(bs->bitnum < 7)
        ++bs->bitnum;
    else
        ++bs->offset, bs->bitnum=0;

    return bit;
}

uint32_t alz_get_bits(alz_bitstream *bs, uint8_t bits_todo)
{
    uint8_t bits_read = 0, octet_bits_left;
    uint32_t bits = 0;

    if (bits_todo > 32)
    {
        printf("ERROR: read_bits() supports reading at most 32 bits at a time (%d bits requested)\n", bits_todo);
        exit(EXIT_FAILURE);
    }

    if (bs->offset >= bs->size || bits_todo > ((bs->size - bs->offset) * 8) - bs->bitnum)
    {
        printf("ERROR: read_bits() buffer overread at 0x%08X@%u (%d bits requested)\n", bs->offset, bs->bitnum, bits_todo);
        exit(EXIT_FAILURE);
    }

    while (bits_todo)
    {
        octet_bits_left = 8 - bs->bitnum;
        if (bits_todo >= octet_bits_left)
        {
            bits |= (bs->buf[bs->offset++] >> bs->bitnum) << bits_read;
            bs->bitnum = 0;
            bits_read += octet_bits_left;
            bits_todo -= octet_bits_left;
        }
        else
        {
            bits |= ((bs->buf[bs->offset] >> bs->bitnum) & ALZ_LSB_MASK(bits_todo)) << bits_read;
            bs->bitnum += bits_todo;
            bits_read += bits_todo;
            bits_todo = 0;
        }
    }

    return bits;
}


/*
 * ALZ stuff
 */
/*
#define ALZ_OFF_TYPE 0x00
#define ALZ_OFF_DCSZ 0x01
#define ALZ_OFF_DATA 0x05
*/
#define ALZ_FLAG_WORD 0
#define ALZ_FLAG_BYTE 1

const uint8_t alz_bitcounts[4] = { 2, 4, 6, 10 };

uint8_t alz_alz_get_bitcount(alz_bitstream *bs)
{
    uint8_t i = 0, start_bitnum = bs->bitnum;

    uint32_t start_offset = bs->offset;

    while (alz_get_bit(bs) == 0)
    {
        i++;

        if (i >= sizeof(alz_bitcounts))
        {
            printf("ERROR: 0x%08X@%u: invalid bitcount index code.\n", start_offset, start_bitnum);
            exit(EXIT_FAILURE);
        }
    }

    return alz_bitcounts[i];
}

int alz_decompress(unsigned char *in, int insz, unsigned char *out, int outsz, int type)
{
    alz_bitstream
        alz;

    uint8_t
        *dest,
        alz_type;

    uint32_t
        dest_offset,
        dest_size,
        word_offset,
        word_size;

    alz.offset = 0; //ALZ_OFF_DATA;

    alz.bitnum = 0;

    alz.size = insz; //filesize(infile);

    alz.buf = in; //calloc(alz.size, 1);

    if(!alz.buf)
    {
        printf("ERROR: Could not allocate memory for compressed data.\n");
        return -1; //exit(EXIT_FAILURE);
    }

    /*
    if(alz.size != fread(alz.buf, 1, alz.size, infile))
    {
        printf("ERROR: Could not read input file.\n");
        return -1; //exit(EXIT_FAILURE);
    }
    */

    alz_type = type; //alz.buf[ALZ_OFF_TYPE];

    dest_offset = 0;

    dest_size = outsz; //get_u32_le(&alz.buf[ALZ_OFF_DCSZ]);

    dest = out; //calloc(dest_size, 1);

    if(!dest)
    {
        printf("ERROR: Could not allocate memory for decompressed data.\n");
        return -1; //exit(EXIT_FAILURE);
    }

    if(alz_type != 0)
    {
        while(dest_offset < dest_size)
        {
            if(alz_get_bit(&alz) == ALZ_FLAG_WORD)
            {
                word_offset = alz_get_bits(&alz, alz_type == 1 ? 10 : alz_alz_get_bitcount(&alz));

                word_size = alz_get_bits(&alz, alz_alz_get_bitcount(&alz));

                if((int)dest_offset - (int)word_offset - 1 < 0)
                {
                    printf("ERROR: 0x%08X@%u: Word offset out of range: %d", alz.offset, alz.bitnum, word_offset);
                    return -1; //exit(EXIT_FAILURE);
                }

                if(((int)dest_offset - (int)word_offset - 1) + (int)word_size > (int)dest_offset)
                {
                    printf("ERROR: 0x%08X@%u: Word size too large: %d", alz.offset, alz.bitnum, word_size);
                    return -1; //exit(EXIT_FAILURE);
                }

                memcpy(&dest[dest_offset], &dest[dest_offset - word_offset - 1], word_size);

                dest_offset += word_size;
            }
            else
            {
                dest[dest_offset] = (uint8_t)alz_get_bits(&alz, 8);

                dest_offset += 1;
            }
        }
    }
    else
    {
        dest = alz.buf + alz.offset;
    }

    /*
    if(dest_size != fwrite(dest, 1, dest_size, outfile))
    {
        printf("ERROR: Could not write decompressed data.\n");
        return -1; //exit(EXIT_FAILURE);
    }
    */

    //free(alz.buf);

    //free(dest);

    //printf("Successfully wrote decompressed data.\n");
    return dest_offset;
}

