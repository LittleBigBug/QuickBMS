// by hcs http://hcs64.com/files/lzh8_cmpdec08.zip
// modified by Luigi Auriemma
/* 
   LZH8 decompressor

   An implementation of LZSS, with symbols stored via two Huffman codes:
   - one for backreference lengths and literal bytes (8 bits each)
   - one for backreference displacement lengths (bits - 1)

   Layout of the compression:

   0x00:        0x40 (LZH8 identifier)
   0x01-0x03:   uncompressed size (little endian)
   0x04-0x07:   optional 32-bit size if 0x01-0x03 is 0
   followed by:

   9-bit prefix coding tree table (for literal bytes and backreference lengths)
   0x00-0x01:   Tree table size in 32-bit words, -1
   0x02-:       Bit packed 9-bit inner nodes and leaves, stored as in Huff8
   Total size:  2 ^ (leaf count + 1)

   5-bit prefix coding tree table (for backreference displacement lengths)
   0x00:        Tree table size in 32-bit words, -1
   0x01-:       Bit packed 5-bit inner nodes and leaves, stored as in Huff8
   Total size:  2 ^ (leaf count + 1)

   Followed by compressed data bitstream:
   1) Get a symbol from the 9-bit tree, if < 0x100 is a literal byte, repeat 1.
   2) If 1 wasn't a literal byte, symbol - 0x100 + 3 is the backreference length
   3) Get a symbol from the 5-bit tree, this is the length of the backreference
      displacement.
   3a) If displacement length is zero, displacement is zero
   3b) If displacement length is one, displacement is one
   3c) If displacement length is > 1, displacement is next displen-1 bits,
       with an extra 1 on the front (normalized).

   Reverse engineered by hcs.
   This software is released to the public domain as of November 2, 2009.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define CHECK_ERRNO(X,Y)    if(X) return -1;
#define CHECK_ERROR(X,Y)    if(X) return -1;

#define VERSION "0.8 " __DATE__

/* debug output options */
#define SHOW_SYMBOLS        0
#define SHOW_FREQUENCIES    0
#define SHOW_TREE           0
#define SHOW_TABLE          0

/* constants */
enum {LENBITS = 9};
enum {DISPBITS = 5};
enum {LENCNT = (1 << LENBITS)};
enum {DISPCNT = (1 << DISPBITS)};

static int get_byte_seek(int offset, unsigned char *infile) {
    return infile[offset];
}
static int get_16_le_seek(int offset, unsigned char *infile) {
    return infile[offset] | (infile[offset+1]<<8);
}

/* read MSB->LSB order */
static inline uint16_t get_next_bits(
        unsigned char *infile,
        long * const offset_p,
        uint8_t * const bit_pool_p,
        int * const bits_left_p,
        const int bit_count)
{
    uint16_t out_bits = 0;
    int num_bits_produced = 0;
    while (num_bits_produced < bit_count)
    {
        if (0 == *bits_left_p)
        {
            *bit_pool_p = get_byte_seek(*offset_p, infile);
            *bits_left_p = 8;
            ++*offset_p;
        }

        int bits_this_round;
        if (*bits_left_p > (bit_count - num_bits_produced))
            bits_this_round = bit_count - num_bits_produced;
        else
            bits_this_round = *bits_left_p;

        out_bits <<= bits_this_round;
        out_bits |=
            (*bit_pool_p >> (*bits_left_p - bits_this_round)) &
            ((1 << bits_this_round) - 1);

        *bits_left_p -= bits_this_round;
        num_bits_produced += bits_this_round;
    }

    return out_bits;
}

#define GET_NEXT_BITS(bit_count) \
    get_next_bits(infile, &input_offset, &bit_pool, &bits_left, bit_count)

int analyze_LZH8(unsigned char *infile, unsigned char *outbuf /*, long file_length*/, int uncompressed_length)
{
    //unsigned long uncompressed_length;

    long input_offset = 0;
    uint8_t bit_pool = 0;
    int bits_left = 0;

    /* read header */
    /*
    {
        uint32_t header;
        header = get_32_le_seek(input_offset, infile);
        input_offset += 4;
        CHECK_ERROR ((header & 0xFF) != 0x40, "not LZH8");
        uncompressed_length = header >> 8;
        if (0 == uncompressed_length)
        {
            uncompressed_length = get_32_le_seek(input_offset, infile);
            input_offset += 4;
        }
    }
    */

    /* allocate backreference length decode table */
    const uint32_t length_table_bytes =
        (get_16_le_seek(input_offset, infile) + 1) * 4;
    input_offset += 2;
    const long length_decode_table_size = LENCNT * 2;
    uint16_t * const length_decode_table =
        malloc(length_decode_table_size*sizeof(uint16_t));
    CHECK_ERRNO(NULL == length_decode_table, "malloc");

    /* read backreference length decode table */
#if SHOW_TABLE
    printf("backreference length table\n");
#endif
    {
        long start_input_offset = input_offset-2;
        long i = 1;
        bits_left = 0;
        while (input_offset - start_input_offset < length_table_bytes)
        {
            if (i >= length_decode_table_size)
            {
                break;
            }
            length_decode_table[i++] = GET_NEXT_BITS(LENBITS);
#if SHOW_TABLE
            printf("%ld: %d\n", i-1, (int)length_decode_table[i-1]);
#endif
        }
        input_offset = start_input_offset + length_table_bytes;
        bits_left = 0;
    }
#if SHOW_TABLE
    printf("done at 0x%lx\n", (unsigned long)input_offset);
    fflush(stdout);
#endif

    /* allocate backreference displacement length decode table */
    const uint32_t displen_table_bytes =
        (get_byte_seek(input_offset, infile) + 1) * 4;
    input_offset ++;
    const long displen_decode_table_size = DISPCNT * 2;
    uint8_t * const displen_decode_table =
        malloc(displen_decode_table_size*sizeof(uint8_t));
    CHECK_ERRNO(NULL == displen_decode_table, "malloc");

    /* read backreference displacement length decode table */
#if SHOW_TABLE
    printf("backreference displacement length table\n");
#endif
    {
        long start_input_offset = input_offset-1;
        long i = 1;
        bits_left = 0;
        while (input_offset - start_input_offset < displen_table_bytes)
        {
            if (i >= length_decode_table_size)
            {
                break;
            }
            displen_decode_table[i++] = GET_NEXT_BITS(DISPBITS);
#if SHOW_TABLE
            printf("%ld: %d\n", i-1, (int)displen_decode_table[i-1]);
#endif
        }
        input_offset = start_input_offset + displen_table_bytes;
        bits_left = 0;
    }
#if SHOW_TABLE
    printf("done at 0x%lx\n", (unsigned long)input_offset);
    fflush(stdout);
#endif

    unsigned long bytes_decoded = 0;

#if SHOW_FREQUENCIES
    long back_length_count[LENCNT] = {0};
    long back_displen_count[DISPCNT] = {0};
#endif

#if SHOW_SYMBOLS
    int symbol_count = 0;
#endif

    int i;

    /* main decode loop */
    while ( bytes_decoded < uncompressed_length )
    {
        unsigned int length_table_offset = 1;
#if SHOW_TREE
        uint16_t length_key_bits = 0;
        uint8_t length_key_len = 0;
#endif

        /* get next backreference length or literal byte */
        for (;;)
        {
            unsigned int next_length_child = GET_NEXT_BITS(1);
#if SHOW_TREE
            length_key_bits = (length_key_bits << 1) | next_length_child;
            length_key_len ++;
#endif
            unsigned int length_node_payload =
                length_decode_table[length_table_offset] & 0x7F;
            unsigned int next_length_table_offset = 
                (length_table_offset / 2 * 2) +
                (length_node_payload + 1) * 2 +
                (next_length_child ? 1 : 0);
            unsigned int next_length_child_isleaf =
                length_decode_table[length_table_offset] &
                (0x100 >> next_length_child);

            if (next_length_child_isleaf)
            {
#if SHOW_SYMBOLS
                printf("%08lx symbol %d: ",
                        (unsigned long)bytes_decoded, symbol_count);
#endif
                uint16_t length = length_decode_table[next_length_table_offset];
#if SHOW_FREQUENCIES
                back_length_count[length] ++;
#endif
#if SHOW_TREE
                printf("%d: ", length);
                for (i=length_key_len-1; i>=0; i--)
                {
                    printf("%c", (length_key_bits&(1<<i)) ? '1' : '0');
                }
                printf("\n");
                fflush(stdout);
#endif

                if ( 0x100 > length )
                {
#if SHOW_SYMBOLS
                    printf("literal %02"PRIX8"\n", length);
#endif
                    /* literal byte */
                    outbuf[bytes_decoded] = length;
                    bytes_decoded++;
                }
                else
                {
                    /* backreference */
                    length = (length & 0xFF) + 3;

                    unsigned int displen_table_offset = 1;
#if SHOW_TREE
                    uint16_t displen_key_bits = 0;
                    uint8_t displen_key_len = 0;
#endif
                    /* get backreference displacement length */
                    for (;;)
                    {
                        unsigned int next_displen_child = GET_NEXT_BITS(1);
#if SHOW_TREE
                        displen_key_bits =
                            (displen_key_bits << 1) | next_displen_child;
                        displen_key_len ++;
#endif
                        unsigned int displen_node_payload =
                            displen_decode_table[displen_table_offset] & 0x7;
                        unsigned int next_displen_table_offset =
                            (displen_table_offset / 2 * 2) +
                            (displen_node_payload + 1) * 2 +
                            (next_displen_child ? 1 : 0);
                        unsigned int next_displen_child_isleaf =
                            displen_decode_table[displen_table_offset] &
                            (0x10 >> next_displen_child);

                        if (next_displen_child_isleaf)
                        {
                            uint16_t displen =
                                displen_decode_table[next_displen_table_offset];
                            uint16_t displacement = 0;

#if SHOW_FREQUENCIES
                            back_displen_count[displen] ++;
#endif
#if SHOW_TREE
                            printf("displen: %d: ", displen);
                            for (i=displen_key_len-1; i>=0; i--)
                            {
                                printf("%c",
                                        (displen_key_bits&(1<<i)) ? '1' : '0');
                            }
                            printf("\n");
                            fflush(stdout);
#endif


#if SHOW_TREE
                            printf("displacement: ");
#endif
                            if ( displen != 0 )
                            {
                                displacement = 1;   /* normalized */

                                /* collect the bits */
                                for (i = displen-1; i > 0; i--)
                                {
                                    displacement *= 2;
                                    int next_bit = GET_NEXT_BITS(1);
#if SHOW_TREE
                                    printf("%c", next_bit ? '1':'0');
#endif
                                    displacement |= next_bit;
                                }
#if SHOW_TREE
                                printf("\n");
                                fflush(stdout);
#endif
                            }
#if SHOW_SYMBOLS
                            printf("%d bytes, offset %d\n",
                                    length,displacement+1);
                            fflush(stdout);
#endif

                            /* apply backreference */
                            for (i = 0;
                                    i < length &&
                                    bytes_decoded < uncompressed_length;
                                    bytes_decoded ++, i ++)
                            {
                                outbuf[bytes_decoded] = 
                                    outbuf[bytes_decoded - displacement - 1];
                            }

                            break; /* break out of displen tree traversal loop*/
                        }
                        else
                        {

                            CHECK_ERROR (
                              next_displen_table_offset == displen_table_offset,
                              "stuck in a loop somehow");
                            displen_table_offset = next_displen_table_offset;
                        }
                    }   /* end of displen tree traversal loop */
                } /* end of if backreference !(0x100 > length)*/
                break; /* break out of length tree traversal loop */
            }
            else
            {

                CHECK_ERROR (next_length_table_offset == length_table_offset,
                        "stuck in a loop somehow");
                length_table_offset = next_length_table_offset;
            }
        }   /* end of length tree traversal */

#if SHOW_SYMBOLS
        symbol_count ++;
#endif
    }   /* end of main decode loop */

#if SHOW_FREQUENCIES
    for (i = 0; i < LENCNT; i++)
    {
        printf("%d: %ld\n", i, back_length_count[i]);
    }
    for (i = 0; i < DISPCNT; i++)
    {
        printf("%d: %ld\n", i, back_displen_count[i]);
    }
#endif

    //put_bytes(outfile, outbuf, uncompressed_length);

    free(length_decode_table);
    free(displen_decode_table);
    return bytes_decoded;
}
