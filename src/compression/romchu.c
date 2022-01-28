// by hcs http://hcs64.com/files/romchu06.zip
// modified by Luigi Auriemma
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* romchu 0.6 */
/* a decompressor for type 2 romc */
/* reversed by hcs from the Wii VC wad for Super Smash Bros EU. */
/* this code is public domain, have at it */

#define VERSION "0.6"



/* bitstream reader */
struct bitstream
{
    const unsigned char *pool;
    long bits_left;
    uint8_t first_byte;
    int first_byte_bits;
};

static struct bitstream *init_bitstream(const unsigned char *pool, unsigned long pool_size)
{
    struct bitstream *bs = malloc(sizeof(struct bitstream));
    if (!bs)
    {
        perror("bitstream malloc");
        exit(EXIT_FAILURE);
    }

    bs->pool = pool;
    bs->bits_left = pool_size;
    bs->first_byte_bits = 0;

    /* check that padding bits are 0 (to ensure we aren't ignoring anything) */
    if (pool_size%8)
    {
        if (pool[pool_size/8] & ~((1<<(pool_size%8))-1))
        {
            fprintf(stderr, "nonzero padding at end of bitstream\n");
            exit(EXIT_FAILURE);
        }
    }

    return bs;
}

static uint32_t get_bits(struct bitstream *bs, int bits)
{
    uint32_t accum = 0;

    if (bits > 32)
    {
        fprintf(stderr, "get_bits() supports max 32\n");
        exit(EXIT_FAILURE);
    }
    if (bits > bs->bits_left + bs->first_byte_bits)
    {
        fprintf(stderr, "get_bits() underflow\n");
        exit(EXIT_FAILURE);
    }

    int i;
    for (i = 0; i < bits; i++)
    {
        if (bs->first_byte_bits == 0)
        {
            bs->first_byte = *bs->pool;
            bs->pool ++;
            if (bs->bits_left >= 8)
            {
                bs->first_byte_bits = 8;
                bs->bits_left -= 8;
            }
            else
            {
                bs->first_byte_bits = bs->bits_left;
                bs->bits_left = 0;
            }
        }

        accum >>= 1;
        accum |= (bs->first_byte & 1)<<31;
        bs->first_byte >>= 1;
        bs->first_byte_bits --;
    }

    return accum>>(32-bits);
}

static int bitstream_eof(struct bitstream *bs)
{
    return (bs->bits_left + bs->first_byte_bits == 0);
}

static void free_bitstream(struct bitstream *bs)
{
    free(bs);
}

/* Huffman code handling */
struct hufnode {
    int is_leaf;
    union {
        struct {
            int left, right;
        } inner;
        struct {
            int symbol;
        } leaf;
    } u;
};
struct huftable {
    int symbols;
    struct hufnode *t;
};

static struct huftable *load_table(struct bitstream *bs, int symbols)
{
    int len_count[32] = {0};
    uint32_t codes[32];
    int length_of[symbols];
    struct huftable *ht;
    int next_free_node;

    int i, j, accum;
    for (i = 0; i < symbols; )
    {
        if (get_bits(bs, 1))
        {
            /* run of equal lengths */
            int count = get_bits(bs, 7) + 2;
            int length = get_bits(bs, 5);

            len_count[length] += count;
            for (j = 0; j < count; j++, i++)
            {
                length_of[i] = length;
            }
        }
        else
        {
            /* set of inequal lengths */
            int count = get_bits(bs, 7) + 1;

            for (j = 0; j < count; j++, i++)
            {
                int length = get_bits(bs, 5);
                length_of[i] = length;
                len_count[length] ++;
            }
        }
    }

    if (!bitstream_eof(bs))
    {
        fprintf(stderr, "did not exhaust bitstream reading table\n");
        exit(EXIT_FAILURE);
    }

    /* compute the first canonical Huffman code for each length */
    len_count[0] = 0; // not strictly necessary
    for (i = 1, accum = 0; i < 32; i++)
    {
        accum = codes[i] = (accum + len_count[i-1]) << 1;
    }

    /* allocate space for the tree */
    ht = malloc(sizeof(struct huftable));
    if (!ht)
    {
        perror("malloc of huftable");
        exit(EXIT_FAILURE);
    }
    ht->symbols = symbols;
    ht->t = malloc(sizeof(struct hufnode) * symbols * 2);
    if (!ht->t)
    {
        perror("malloc of hufnodes");
        exit(EXIT_FAILURE);
    }

    /* determine codes and build a tree */
    for (i = 0; i < symbols*2; i++)
    {
        ht->t[i].is_leaf = 0;
        ht->t[i].u.inner.left = ht->t[i].u.inner.right = 0;
    }
    next_free_node = 1;
    for (i = 0; i < symbols; i++)
    {
        int cur = 0;
        if (0 == length_of[i])
        {
            // 0 length indicates absent symbol
            continue;
        }

        for (j = length_of[i]-1; j >= 0; j --)
        {
            int next;
            if (ht->t[cur].is_leaf)
            {
                fprintf(stderr, "oops, walked onto a leaf\n");
                exit(EXIT_FAILURE);
            }

            if (codes[length_of[i]]&(1<<j))
            {
                // 1 == right
                next = ht->t[cur].u.inner.right;
                if (0 == next)
                {
                    next = ht->t[cur].u.inner.right = next_free_node ++;
                }
            }
            else
            {
                // 0 == left
                next = ht->t[cur].u.inner.left ;
                if (0 == next)
                {
                    next = ht->t[cur].u.inner.left = next_free_node ++;
                }
            }

            cur = next;
        }

        ht->t[cur].is_leaf = 1;
        ht->t[cur].u.leaf.symbol = i;

        codes[length_of[i]] ++;
    }

    return ht;
}

static int huf_lookup(struct bitstream *bs, struct huftable *ht)
{
    int cur = 0;
    while (!ht->t[cur].is_leaf)
    {
        if (get_bits(bs, 1))
        {
            // 1 == right
            cur = ht->t[cur].u.inner.right;
        }
        else
        {
            // 0 == left
            cur = ht->t[cur].u.inner.left;
        }
    }

    return ht->t[cur].u.leaf.symbol;
}

static void free_table(struct huftable *ht)
{
    if (ht)
    {
        free(ht->t);
    }
    free(ht);
}

int romchiu(unsigned char *infile, int infile_size, unsigned char *out_buf, int nominal_size)
{

    struct {
        unsigned int bits;
        unsigned int base;
    } backref_len[0x1D], backref_disp[0x1E];

    unsigned char head_buf[4];
    unsigned char payload_buf[0x10000];
    int block_count = 0;
    long out_offset = 0;

    //uint64_t nominal_size;
    //int romc_type;

    int i, j, k, scale;

    unsigned char   *limit = infile + infile_size;

    // read header
    /*
    {
        if (1 != fread(head_buf, 4, 1, infile))
        {
            int save_errno = errno;
            if (infile >= limit)
            {
                fprintf(stderr, "unexpected EOF reading header\n");
            }
            else
            {
                errno = save_errno;
                perror("fread header");
            }

            return 1;
        }

        nominal_size = head_buf[0];
        nominal_size *= 0x100;
        nominal_size |= head_buf[1];
        nominal_size *= 0x100;
        nominal_size |= head_buf[2];
        nominal_size *= 0x40;
        nominal_size |= head_buf[3]>>2;
        romc_type = head_buf[3]&0x3;

        if (romc_type != 2)
        {
            fprintf(stderr,"Expected type 2 romc, got %d\n", romc_type);
            return 1;
        }
    }
    */

    // initialize backreference lookup tables
    {
        for (i = 0; i < 8; i++)
        {
            backref_len[i].bits = 0;
            backref_len[i].base = i;
        }

        for (i = 8, scale = 1; scale < 6; scale++)
        {
            for (             k = (1<<(scale+2));
                              k < (1<<(scale+3));
                              k += (1<<scale), i++)
            {
                backref_len[i].bits = scale;
                backref_len[i].base = k;
            }
        }

        backref_len[28].bits = 0;
        backref_len[28].base = 255;

        for (i = 0; i < 4; i++)
        {
            backref_disp[i].bits = 0;
            backref_disp[i].base = i;
        }

        for (i = 4, scale = 1, k = 4; scale < 14; scale ++)
        {
            for (j = 0; j < 2; j ++, k += (1 << scale), i++)
            {
                backref_disp[i].bits = scale;
                backref_disp[i].base = k;
            }
        }
    }

    out_offset = 0;

    // decode each block
    //while (1 == fread(head_buf, 4, 1, infile))
    while((infile + 4) <= limit)
    {
        memcpy(head_buf, infile, 4); infile += 4;
        int compression_flag;
        uint32_t payload_bytes;
        int payload_bits;
        uint32_t read_size;

        struct bitstream *head_bs;

#if 0
        printf("%08lx=%08lx\n",
            (unsigned long)(ftell(infile)-4),
            (unsigned long)block_count*block_mult);
#endif

        head_bs = init_bitstream(head_buf, 4*8);

        compression_flag = get_bits(head_bs, 1);
        if (compression_flag)
        {
            /* compressed */

            uint32_t block_size;

            /* bits, including this header */
            block_size = get_bits(head_bs, 31) - 32;

            payload_bytes = block_size/8;
            payload_bits = block_size%8;
        }
        else
        {
            /* uncompressed */

            uint32_t block_size;

            /* bytes */
            block_size = get_bits(head_bs, 31);

            payload_bytes = block_size;
            payload_bits = 0;
        }

        free_bitstream(head_bs);
        head_bs = NULL;

        /* read payload */
        read_size = payload_bytes;
        if (payload_bits > 0)
        {
            read_size ++;
        }

        if (read_size > sizeof(payload_buf))
        {
            fprintf(stderr, "payload too large\n");
            return 1;
        }
        //if (1 != fread(payload_buf, read_size, 1, infile))
        memcpy(payload_buf, infile, read_size); infile += read_size;
        {
            int save_errno = errno;
            if (infile >= limit)
            {
                fprintf(stderr, "fread of payload: unexpected EOF\n");
            }
            else
            {
                errno = save_errno;
                perror("fread of payload");
            }
            return 1;
        }

        /* attempt to parse... */

        if (compression_flag)
        {
            uint16_t tab1_size, tab2_size;
            uint32_t body_size;
            unsigned long tab1_offset, tab2_offset, body_offset;
            struct bitstream *bs;
            struct huftable *table1, *table2;
            
            /* read table 1 size */
            tab1_offset = 0;
            bs = init_bitstream(payload_buf + tab1_offset, payload_bytes*8+payload_bits);
            tab1_size = get_bits(bs, 16);
            free_bitstream(bs);

            /* load table 1 */
            bs = init_bitstream(payload_buf + tab1_offset + 2, tab1_size);
            table1 = load_table(bs, 0x11D);
            free_bitstream(bs);

            /* read table 2 size */
            tab2_offset = tab1_offset + 2 + (tab1_size+7) / 8;
            bs = init_bitstream(payload_buf + tab2_offset, 2*8);
            tab2_size = get_bits(bs, 16);
            free_bitstream(bs);

            /* load table 2 */
            bs = init_bitstream(payload_buf + tab2_offset + 2, tab2_size);
            table2 = load_table(bs, 0x1E);
            free_bitstream(bs);

            /* decode body */
            body_offset = tab2_offset + 2 + (tab2_size+7) / 8;
            body_size = payload_bytes*8 + payload_bits - body_offset*8;
            bs = init_bitstream(payload_buf + body_offset, body_size);

            while (!bitstream_eof(bs))
            {
                int symbol = huf_lookup(bs, table1);

                if (symbol < 0x100)
                {
                    /* byte literal */
                    unsigned char b = symbol;
                    if (out_offset >= nominal_size)
                    {
                        fprintf(stderr, "generated too many bytes\n");
                        return 1;
                    }
                    out_buf[out_offset++] = b;
                }
                else
                {
                    /* backreference */
                    unsigned int len_bits = backref_len[symbol-0x100].bits;
                    unsigned int len = backref_len[symbol-0x100].base;
                    if (len_bits > 0)
                    {
                        len += get_bits(bs, len_bits);
                    }
                    len += 3;

                    int symbol2 = huf_lookup(bs, table2);

                    unsigned int disp_bits = backref_disp[symbol2].bits;
                    unsigned int disp = backref_disp[symbol2].base;
                    if (disp_bits > 0)
                    {
                        disp += get_bits(bs, disp_bits);
                    }
                    disp ++;

                    if (disp > out_offset)
                    {
                        fprintf(stderr, "backreference too far\n");
                        return 1;
                    }
                    if (out_offset+len > nominal_size)
                    {
                        fprintf(stderr, "generated too many bytes\n");
                        return 1;
                    }
                    for (i = 0; i < len; i++, out_offset++)
                    {
                        out_buf[out_offset] = out_buf[out_offset-disp];
                    }
                }
            }

            free_table(table1);
            free_table(table2);
            free_bitstream(bs);
        }
        else
        {
            if (out_offset + payload_bytes > nominal_size)
            {
                fprintf(stderr, "generated too many bytes\n");
                return 1;
            }
            memcpy(out_buf+out_offset, payload_buf, payload_bytes);
            out_offset += payload_bytes;
        }

        block_count ++;
    }

    if (out_offset != nominal_size)
    {
        fprintf(stderr, "size mismatch\n");
        return 1;
    }

    return out_offset;
}
