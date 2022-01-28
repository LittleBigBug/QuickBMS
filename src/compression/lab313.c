// modified and/or rewritten by Luigi Auriemma
/*
original code by Lab 313
https://github.com/lab313ru?tab=repositories
http://lab313.ru
*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef uint8_t     byte;
typedef uint16_t    ushort;
typedef uint32_t    uint;



/*
SMD Factor5 LZ Tool
https://github.com/lab313ru/fact5lz
*/



#define minlen ((ver == '1') ? 3 : 4)

static byte read_byte(byte *input, int *readoff)
{
	return (input[(*readoff)++]);
}

static void write_byte(byte *output, int *writeoff, byte b)
{
	output[(*writeoff)++] = b;
}

static ushort read_word(byte *input, int *readoff)
{
	ushort retn = read_byte(input, readoff) << 8;
	retn |= read_byte(input, readoff);
	return retn;
}

static ushort read_word_inv(byte *input, int *readoff)
{
	ushort retn = read_byte(input, readoff);
	retn |= (read_byte(input, readoff) << 8);
	return retn;
}

void write_word(byte *output, int *writeoff, ushort w)
{
	write_byte(output, writeoff, w >> 8);
	write_byte(output, writeoff, w & 0xFF);
}

int fact5lz_decompress(byte *input, byte *output)
{
	int readoff = 0, writeoff = 0, from = 0;
	byte reps = 0;
	byte *copy_from = 0;

	byte b = 0, ver = 0;
	ushort out_size = 0;

	ver = read_word(input, &readoff) & 0xFF;
	out_size = read_word_inv(input, &readoff);

    ushort i;

	if (ver != '1' && ver != '2')
	{
		for (i = 0; i < out_size; ++i)
		{
			b = read_byte(input, &readoff);
			write_byte(output, &writeoff, b);
		}

		return out_size;
	}

	while (writeoff < out_size)
	{
		b = read_byte(input, &readoff);

		if (b & 0x80)
		{
			switch (ver)
			{
			case '1':
			{
				// 0rrr rfff ffff ffff
				reps = ((b >> 3) & 0xF) + minlen;
				from = ((b & 7) << 8);
				from |= read_byte(input, &readoff);

				from = (short)(writeoff - from - 1);
			} break;
			case '2':
			{
				// 0rrr rrrr ffff ffff
				// ffff ffff
				reps = (b & 0x7F) + minlen;
				from = (read_byte(input, &readoff) << 8);
				from |= read_byte(input, &readoff);

				from = (int)(writeoff - from - 1);
			} break;
			}
			copy_from = output;
		}
		else
		{
			// 0rrr rrrr
			reps = (b & 0x7F) + 1;
			from = readoff;
			copy_from = input;
			readoff += reps;
		}

		for (i = 0; i < reps; ++i)
		{
			b = read_byte(copy_from, &from);
			write_byte(output, &writeoff, b);
		}
	}

	return writeoff;
}




/*
[SMD] Tecmo Cup Football Game LZ Compressor
https://github.com/lab313ru/LZCapTsu
*/



#define maxfrom_bits_0 (16 - reps_bits_cnt)

static byte read_cmd_bit(byte *input, int *readoff, byte *bitscnt, byte *cmd)
{
	(*bitscnt)--;

	if (!*bitscnt) {
		*cmd = read_byte(input, readoff);
		*bitscnt = 8;
	}

	byte retn = *cmd & 1;
	*cmd >>= 1;
	return retn;
}

static ushort do_decompress_0(byte *input, byte *output, ushort out_size)
{
    int readoff = 0, writeoff = 0;
	ushort i = 0;
	byte cmdbits = 0, cmd = 0, bit = 0;
	ushort reps_mask = 0, from_mask = 0;
	ushort reps = 0; int from = 0;
	byte reps_bits_cnt = 0;
	byte b = 0;
	ushort w = 0;
	int repeat = 0;
	
	cmd = 0;
	cmdbits = 1;

	reps_bits_cnt = read_byte(input, &readoff);

	while (writeoff < out_size)
	{
		bit = read_cmd_bit(input, &readoff, &cmdbits, &cmd);

		if (bit)
		{
			b = read_byte(input, &readoff);
			write_byte(output, &writeoff, b);
		}
		else
		{
			// reps_bits_count = MSB bits in word of reps
			// from = rest of bits

			w = read_word(input, &readoff);
			reps_mask = ((1 << reps_bits_cnt) - 1) << maxfrom_bits_0;
			from_mask = (~reps_mask);
			reps = ((w & reps_mask) >> (16 - reps_bits_cnt)) + 2;
			from = (w & from_mask);

			from = writeoff - from - 1;
			repeat = (short)from < 0;

			for (i = 0; i < reps; ++i)
			{
				b = read_byte(output, &from);
				b = (repeat ? 0 : b);
				write_byte(output, &writeoff, b);
			}
		}
	}

	return writeoff;
}

static ushort do_decompress_1_byte(byte *input, byte *output, ushort in_size)
{
    int readoff = 0, writeoff = 0;
	ushort i = 0, reps = 0;
	byte b = 0;

	while (readoff < in_size)
	{
		b = read_byte(input, &readoff);

		if (b == input[readoff])
		{
			readoff++;
			reps = read_byte(input, &readoff) + 2;
		}
		else
		{
			reps = 1;
		}

		for (i = 0; i < reps; ++i)
		{
			write_byte(output, &writeoff, b);
		}
	}

	return writeoff;
}

static ushort do_decompress_1_word(byte *input, byte *output, ushort in_size)
{
    int readoff = 0, writeoff = 0;
	ushort w = 0, reps = 0, i = 0;

	while (readoff < in_size)
	{
		w = read_word(input, &readoff);

		if (
			(w >> 0x8) == input[readoff + 0] &&
			(w & 0xFF) == input[readoff + 1]
			)
		{
			readoff += 2;
			reps = read_byte(input, &readoff) + 2;
		}
		else
		{
			reps = 1;
		}

		for (i = 0; i < reps; ++i)
		{
			write_word(output, &writeoff, w);
		}
	}

	return writeoff;
}

static ushort do_decompress_1_copy(byte *input, byte *output)
{
	int readoff = 0;
	short size = (short)read_word(input, &readoff);
	size = (size < 0) ? 3 : size;

	memcpy(output, &input[readoff], size);

	return size;
}

ushort LZCapTsu_decompress(byte *input, byte *output)
{
    int readoff = 0;
	ushort bit = 0, out_size = 0;
	byte method = 0;

	readoff = 0;
	out_size = read_word(input, &readoff);

	bit = (out_size & 0x8000);
	out_size &= ~0x8000;

	if (bit)
	{
		method = read_byte(input, &readoff) & 3;

		switch (method)
		{
		case 0: return do_decompress_1_byte(&input[readoff], output, out_size);
		case 1: return do_decompress_1_word(&input[readoff], output, out_size);
		case 2:
		case 3: return do_decompress_1_copy(&input[readoff], output);
		}
	}

	return do_decompress_0(&input[readoff], output, out_size);
}




/*
Thunder Force 3 Compression Routines
https://github.com/lab313ru/tf3compr
*/


// lzh instead is just lzss so it's not implemented
int tf3_rle_decompress(byte *data, byte *output, int out_size) {
    const byte NIBBLES_IN_LONG = 8;
    //const byte NIBBLES_IN_BYTE = 2;
    //const byte BITS_IN_NIBBLE = 4;

    /// <summary>
    /// Decompresses RLE packed data
    /// </summary>
    /// <param name="data">Compressed byte array</param>
    /// <param name="offset">Initial offset for compressed data</param>
    /// <returns></returns>
    //public static byte[] DecompressData(byte[] data, int offset = 0)
    //{
        int read_off = 0;
        int write_off = 0;

        //int out_size = (int)data.ReadLong(read_off);
        //byte[] output = new byte[out_size];
        byte *o = output;

        int longs_to_write = out_size / 4;
        //read_off += 4;
        read_off = 0;

        uint long_to_write = 0;
        byte nibbles_in_long = NIBBLES_IN_LONG;

        while (longs_to_write > 0)
        {
            byte b = data[read_off++];

            byte nib_count = (byte)(b & 0xF);
            byte nibble = (byte)(b >> 4);

            int nibbles_to_write = 0;

            if (nib_count < 0xD)
            {
                nibbles_to_write = nib_count;
            }
            else if (nib_count == 0xD)
            {
                nibbles_to_write = data[read_off++];
            }
            else
            {
                nibbles_to_write = data[read_off] | (data[read_off+1] << 8); //data.ReadWord(read_off);
                read_off += 2;

                if (nib_count != 0xE)
                {
                    nibbles_to_write += 0x10000;
                }
            }

            int i;
            for (i = 0; i < nibbles_to_write + 1; ++i)
            {
                long_to_write = (long_to_write << 4) | nibble;

                if (--nibbles_in_long == 0)
                {
                    //output.WriteLong(write_off, long_to_write);
                    *o++ = long_to_write >> 0;
                    *o++ = long_to_write >> 8;
                    *o++ = long_to_write >> 16;
                    *o++ = long_to_write >> 24;

                    write_off += 4;
                    longs_to_write--;

                    nibbles_in_long = NIBBLES_IN_LONG;

                    if (longs_to_write == 0)
                    {
                        break;
                    }
                }
            }
        }

        return o - output;
        //return output;
    }



/*
https://github.com/lab313ru/WinImploder
*/


static unsigned char token;
static int cmpr_pos /*a3*/;
static unsigned char *cmpr_data;

static void copy_bytes(unsigned char *dst, unsigned char *src, int count){
    int i;
	for (i = 0; i < count; i++){
		dst[i] = src[i];
	}
}

static unsigned short _read_word(unsigned char *buf){
	return ((buf[0] << 8) | buf[1]);
}

static unsigned int _read_dword(unsigned char *buf){
	return ((_read_word(&buf[0]) << 16) | _read_word(&buf[2]));
}

static void _write_word(unsigned char *dst, unsigned short value){
	dst[0] = (value >> 8) & 0xFF;
	dst[1] = (value >> 0) & 0xFF;
}

static void _write_dword(unsigned char *dst, unsigned int value){
	_write_word(&dst[0], (value >> 16) & 0xFFFF);
	_write_word(&dst[2], (value >> 0) & 0xFFFF);
}

static unsigned int read_bits(unsigned char count){
	unsigned int retn = 0;

    int i;
	for (i = 0; i < count; i++){
		char bit = (token >> 7);
		token <<= 1;

		if (!token){
			token = (cmpr_data[cmpr_pos-1] << 1) | bit;
			bit = (cmpr_data[--cmpr_pos] >> 7);
		}

		retn <<= 1;
		retn |= bit;
	}
	return retn;
}

static int check_imp(unsigned char *input){
	unsigned int id, end_off, out_len;

	if (!input){
		return 0;
	}

	id = _read_dword(&input[0]);
	out_len = _read_dword(&input[4]);
	end_off = _read_dword(&input[8]);

	/* check for magic ID 'IMP!', or one of the other IDs used by Imploder
	clones; ATN!, BDPI, CHFI, Dupa, EDAM, FLT!, M.H., PARA and RDC9 */
	if (id != 0x494d5021 && id != 0x41544e21 && id != 0x42445049 && id != 0x43484649 && id != 0x44757061 &&
			id != 0x4544414d && id != 0x464c5421 && id != 0x4d2e482e && id != 0x50415241 &&	id != 0x52444339){
		return 0;
	}

	/* sanity checks */
	return !((end_off & 1) || (end_off < 14) || ((end_off + 0x26) > out_len));
}

int winimplode_explode(unsigned char *input){
static unsigned char explode_token_base[4] = { 6, 10, 10, 18 };
static unsigned char static_token_extra_bits[12] = { 1,  1,  1,  1,  2,  3,  3,  4,  4,  5,  7, 14 };

unsigned char *src;
unsigned int /*write_pos,*/ src_size, src_end /*a4*/, token_run_len /*d2*/;

unsigned short run_base_off_tbl[8];
unsigned char run_extra_bits_tbl[12];


	if (!check_imp(input)){
		return 0;
	}

	/*write_pos = 0,*/ src_size = 0, src_end = 0 /*a4*/, token_run_len = 0 /*d2*/;
	cmpr_pos = 0 /*a3*/;
	token = 0;

	src = input;
	src_end = src_size = _read_dword(&src[0x04]);
	cmpr_data = &src[_read_dword(&src[0x08])];
	cmpr_pos = 0;

	_write_dword(&src[0x08], _read_dword(&cmpr_data[0x00]));
	_write_dword(&src[0x04], _read_dword(&cmpr_data[0x04]));
	_write_dword(&src[0x00], _read_dword(&cmpr_data[0x08]));

	token_run_len = _read_dword(&cmpr_data[0x0C]);

	if (!(cmpr_data[0x10] & 0x80)){
		cmpr_pos--;
	}

	token /*d3*/ = cmpr_data[0x11];

    int i;
	for (i = 0; i < 8; i++){
		/*a1*/run_base_off_tbl[i] = _read_word(&cmpr_data[0x12+i*2]);
	}

	copy_bytes(&run_extra_bits_tbl[0], &cmpr_data[0x22], 12);
//int cnt = 0;
	while (1){
		for (i = 0; (i < token_run_len) && (src_end > 0); i++){
			src[--src_end] = cmpr_data[--cmpr_pos];
		}

		if (src_end == 0){
			break;
		}

		unsigned int match_len, selector;

		if (read_bits(1)){
			if (read_bits(1)){
				if (read_bits(1)){
					if (read_bits(1)){
						if (read_bits(1)){
							match_len = cmpr_data[--cmpr_pos];
							selector = 3;
						}
						else{
							match_len = read_bits(3) + 6;
							selector = 3;
						}
					}
					else{
						match_len = 5;
						selector = 3;
					}
				}
				else{
					match_len = 4;
					selector = 2;
				}
			}
			else{
				match_len = 3;
				selector = 1;
			}
		}
		else{
			match_len = 2;
			selector = 0;
		}

		if (read_bits(1)){
			if (read_bits(1)){
				token_run_len = read_bits(static_token_extra_bits[selector+8]) + explode_token_base[selector];
			}
			else{
				token_run_len = read_bits(static_token_extra_bits[selector+4]) + 2;
			}
		}
		else{
			token_run_len = read_bits(static_token_extra_bits[selector]);
		}

		unsigned char *match;

		if (read_bits(1)){
			if (read_bits(1)){
				match = &src[src_end + read_bits(run_extra_bits_tbl[8+selector]) + run_base_off_tbl[4+selector] + 1];
			}
			else{
				match = &src[src_end + read_bits(run_extra_bits_tbl[4+selector]) + run_base_off_tbl[selector] + 1];
			}
		}
		else{
			match = &src[src_end + read_bits(run_extra_bits_tbl[selector]) + 1];
		}

		//printf("%06X-%06X\n", match_len, src_end); cnt++;

		for (i = 0; (i < match_len) && (src_end > 0); i++){
			src[--src_end] = *--match;
		}
	}

	return src_size;
}



/*
# lzkn
LZ Konami Compression Tools

**LZKN1**:
- Animaniacs
- Contra - Hard Corps
- Lethal Enforcers II - Gun Fighters
- Sparkster

**LZKN2**:
- Animaniacs
- Castlevania - Bloodlines
- Contra - Hard Corps
- Rocket Knight Adventures
- Sunset Riders
- Teenage Mutant Ninja Turtles - The Hyperstone Heist
- Tiny Toon Adventures - Acme All-Stars
- Tiny Toon Adventures - Buster’s Hidden Treasure

**LZKN3**:
- Castlevania - Bloodlines
- Hyper Dunk - The Playoff Edition
- Lethal Enforcers
- Teenage Mutant Ninja Turtles - Tournament Fighters
- Tiny Toon Adventures - Acme All-Stars
*/

#define wndsize (1 << 10)
#define wndmask (wndsize - 1)
#define maxreps 0x40
#define maxreps0 0x03
#define maxreps1 0x22
#define maxreps2 0x45



static void init_wnd(byte **window, byte **reserve, int *wndoff, int chr, int offset)
{
	*window = (byte *)malloc(wndsize);
	*reserve = (byte *)malloc(wndsize);
	memset(*window, chr, wndsize);
	*wndoff = offset;
}

static byte read_wnd_byte(byte *window, int *wndoff)
{
	byte b = window[*wndoff];
	*wndoff = (*wndoff + 1) & wndmask;
	return b;
}

static void write_to_wnd(byte *window, int *wndoff, byte b)
{
	window[*wndoff] = b;
	*wndoff = (*wndoff + 1) & wndmask;
}



int lzkn1_decompress(unsigned char *input, unsigned char *output) {
	int BITS, READPOS, WRITEPOS, BIT, COMMAND, SEQ, REPS, FROM, MASK, i;

	READPOS = 2;
	BITS = WRITEPOS = 0;
	while (1) {
		if (!(BITS--)) {
			BITS = 7;
			COMMAND = input[READPOS++];
		}

		BIT = COMMAND & 1;
		COMMAND >>= 1;

		if (!BIT) {
			output[WRITEPOS++] = input[READPOS++];
		} else {
			SEQ = input[READPOS++];
			MASK = (SEQ & 0xC0) >> 6;

			if (SEQ == 0x1F)
				break;

			switch (MASK) {
			case 0: //00,01
					// (REPS >= 2 && REPS <= 0x22)
					// (FROM >= 1 && FROM <= 0x3FF)
			case 1: {
				REPS = (SEQ & 0x1F) + 3;
				FROM = ((SEQ << 3) & 0xFF00) | input[READPOS++];

				for (i = 0; i < REPS; i++) {
					output[WRITEPOS] = output[WRITEPOS - FROM];
					WRITEPOS++;
				}
			}
				break;

			case 2: //10
					// (REPS >= 2 && REPS <= 5)
					// (FROM >= 1 && FROM <= 0xF)
			{
				REPS = (SEQ >> 4) - 6;
				FROM = SEQ & 0x0F;

				for (i = 0; i < REPS; i++) {
					output[WRITEPOS] = output[WRITEPOS - FROM];
					WRITEPOS++;
				}
			}
				break;

			case 3: //11
					// (REPS >= 8 && REPS <= 0x47)
			{
				REPS = SEQ - 0xB8;

				for (i = 0; i < REPS; i++)
					output[WRITEPOS++] = input[READPOS++];
			}
				break;
			}
		}
	}

	return (WRITEPOS & 1) ? WRITEPOS + 1 : WRITEPOS;
}



int lzkn2_decompress(byte *input, byte *output)
{
	int i = 0, size = 0, bit = 0, readoff = 0, writeoff = 0, wndoff = 0, reps = 0, from = 0;
	byte bitscnt = 0, cmd = 0;
	ushort b = 0;
	byte *window, *reserve;

	init_wnd(&window, &reserve, &wndoff, 0x20, 0x3c0);

	readoff = 0;
	writeoff = 0;
	size = read_word(input, &readoff);

	bitscnt = 1;

	while (readoff - 2 < size)
	{
		bit = read_cmd_bit(input, &readoff, &bitscnt, &cmd);

		if (bit)
		{
			b = read_byte(input, &readoff);
			write_byte(output, &writeoff, (byte)b);
			write_to_wnd(window, &wndoff, (byte)b);
		}
		else
		{
			b = read_word(input, &readoff);
			reps = ((b & 0xFC00) >> 10) + 1;
			from = b & wndmask;

			for (i = 0; i < reps; ++i)
			{
				b = read_wnd_byte(window, &from);
				if (from == wndsize)
					from = 0;
				write_byte(output, &writeoff, (byte)b);
				write_to_wnd(window, &wndoff, (byte)b);
			}
		}
	}

	free(window);
	free(reserve);
	return writeoff;
}



int lzkn3_decompress(byte *input, byte *output)
{
	int /*size = 0,*/ bit = 0, readoff = 0, writeoff = 0, wndoff = 0, reps = 0, from = 0;
	byte bitscnt = 0, cmd = 0;
	ushort b = 0;
	byte *window, *reserve;

	init_wnd(&window, &reserve, &wndoff, 0x00, 0x3df);

	readoff = 0;
	writeoff = 0;
	/*size = */read_word(input, &readoff);

	bitscnt = 1;

	while (1)
	{
		bit = read_cmd_bit(input, &readoff, &bitscnt, &cmd);
		b = read_byte(input, &readoff);

		if (bit == 0) // pack: 1 byte; write: 1 byte
		{
			write_byte(output, &writeoff, (byte)b);
			write_to_wnd(window, &wndoff, (byte)b);
		}
		else
		{
			if (b < 0x80) // pack: 3..34 (wnd: 0..0x3FF); write: 2 bytes
			{
				if (b == 0x1F)
					break;

				// ??01101111 11100011?
				//    11111          (reps)
				//  11      11111111 (from)
				reps = (b & 0x1F) + 3;
				from = ((b & 0x60) << 3) | read_byte(input, &readoff);
			}
			else if (b >= 0x80 && b <= 0xC0) // pack: 2..5 byte (wnd: wndoff-F..wndoff-0); write: 1 byte
			{
				// ?1000 0011?
				//   11      (reps)
				//      1111 (from)
				reps = ((b >> 4) & 3) + 2;
				from = (wndoff - (b & 0xF)) & wndmask;
			}
			else // 0xC1.. // pack: 9..71; write: 10..72 bytes
			{
				reps = (b & 0x3F) + 8; // max == %111111 + 8 == 71 (0x47)

				while (reps > 0)
				{
					b = read_byte(input, &readoff);

					write_byte(output, &writeoff, (byte)b);
					write_to_wnd(window, &wndoff, (byte)b);

					reps--;
				}
			}

			while (reps > 0)
			{
				b = read_wnd_byte(window, &from);

				write_byte(output, &writeoff, (byte)b);
				write_to_wnd(window, &wndoff, (byte)b);

				reps--;
			}
		}
	}

	free(window);
	free(reserve);
	return writeoff;
}


