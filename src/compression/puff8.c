// by hcs http://hcs64.com/files/puff8_00.zip
// modified by Luigi Auriemma

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define CHECK_ERRNO(X,Y)    if(X) return -1;
#define CHECK_ERROR(X,Y)    if(X) return -1;

int analyze_Huf8(unsigned char *infile, unsigned char *outfile /*, int file_length*/, int decoded_length)
{
    unsigned char   *o = outfile;

    unsigned char *decode_table = NULL;
    int decode_table_size;
    //long decoded_length;
    int symbol_count;

    /* read header */
    /*
    {
        unsigned char buf[5];
        get_bytes_seek(0, infile, buf, 5);
        CHECK_ERROR (buf[0] != 0x28, "not 8-bit Huffman");
        decoded_length = read_24_le(&buf[1]);
        symbol_count = buf[4] + 1;
    }
    */
    symbol_count = (*infile++) + 1;

    /* allocate decode table */
    decode_table_size = symbol_count * 2 - 1;
    decode_table = malloc(decode_table_size);
    CHECK_ERRNO(decode_table == NULL, "malloc");

    /* read decode table */
    //get_bytes(infile, decode_table, decode_table_size);
    memcpy(decode_table, infile, decode_table_size);
    infile += decode_table_size;

#if 0
    printf("encoded size = %ld bytes (%d header + %ld body)\n",
            file_length, 5 + decode_table_size,
            file_length - (5 + decode_table_size));
    printf("decoded size = %ld bytes\n", decoded_length);
#endif

    /* decode */
    //{
        uint32_t bits;
        int bits_left = 0;
        int table_offset = 0;
        long bytes_decoded = 0;

        while ( bytes_decoded < decoded_length )
        {
            if (bits_left == 0)
            {
                bits = infile[0]|(infile[1]<<8)|(infile[2]<<16)|(infile[3]<<24); infile += 4;   //get_32_le(infile);
                bits_left = 32;
            }

            int current_bit = ((bits & 0x80000000) != 0);
            int next_offset = ((table_offset + 1) / 2 * 2) + 1 +
                (decode_table[table_offset] & 0x3f) * 2 +
                (current_bit ? 1 : 0);

#if 0
            printf("%d %02x %lx => %lx\n", current_bit,
                    decode_table[table_offset],
                    (unsigned long)table_offset,
                    (unsigned long)next_offset);
#endif

            CHECK_ERROR (next_offset >= decode_table_size,
                    "reading past end of decode table");

            if ((!current_bit && (decode_table[table_offset] & 0x80)) ||
                ( current_bit && (decode_table[table_offset] & 0x40)))
            {
                *o++ = decode_table[next_offset];
                //CHECK_FILE(
                //    fwrite(&decode_table[next_offset], 1, 1, outfile) != 1,
                //    outfile, "fwrite");
                bytes_decoded++;
#if 0
                printf("%02x\n", decode_table[next_offset]);
                return;
#endif
                next_offset = 0;
            }

            CHECK_ERROR (next_offset == table_offset,
                    "stuck in a loop somehow");
            table_offset = next_offset;
            bits_left--;
            bits <<= 1;
        }
    //}

#if 0
    printf("done\n");
#endif
    free(decode_table);
    return bytes_decoded;
}
