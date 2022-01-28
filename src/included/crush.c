// modified by Luigi Auriemma
// the original algorithm was chunk-based, this version works on one chunk only

// crush.cpp
// Written and placed in the public domain by Ilya Muravyov
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int bit_buf;
static int bit_count;
static unsigned char *in;

static void crush_init_bits()
{
	bit_count=bit_buf=0;
}

static int crush_get_bits(int n)
{
	while (bit_count<n)
	{
		bit_buf|=(*in++)<<bit_count;
		bit_count+=8;
	}
	const int x=bit_buf&((1<<n)-1);
	bit_buf>>=n;
	bit_count-=n;
	return x;
}

int crush_decompress(unsigned char *_in, int insz, unsigned char *out, int outsz) {

int W_BITS=21; // Window size (17..23)
//int W_SIZE=1<<W_BITS;
//int W_MASK=W_SIZE-1;
int SLOT_BITS=4;
int NUM_SLOTS=1<<SLOT_BITS;

int A_BITS=2; // 1 xx
int B_BITS=2; // 01 xx
int C_BITS=2; // 001 xx
int D_BITS=3; // 0001 xxx
int E_BITS=5; // 00001 xxxxx
int F_BITS=9; // 00000 xxxxxxxxx
int A=1<<A_BITS;
int B=(1<<B_BITS)+A;
int C=(1<<C_BITS)+B;
int D=(1<<D_BITS)+C;
int E=(1<<E_BITS)+D;
//int F=(1<<F_BITS)+E;

    in = _in;
	crush_init_bits();

    int size = outsz;
        
		int p=0;
		while (p<size)
		{
			if (crush_get_bits(1))
			{
				int len;
				if (crush_get_bits(1))
					len=crush_get_bits(A_BITS);
				else if (crush_get_bits(1))
					len=crush_get_bits(B_BITS)+A;
				else if (crush_get_bits(1))
					len=crush_get_bits(C_BITS)+B;
				else if (crush_get_bits(1))
					len=crush_get_bits(D_BITS)+C;
				else if (crush_get_bits(1))
					len=crush_get_bits(E_BITS)+D;
				else
					len=crush_get_bits(F_BITS)+E;

				const int log=crush_get_bits(SLOT_BITS)+(W_BITS-NUM_SLOTS);
				int s=~(log>(W_BITS-NUM_SLOTS)
					?crush_get_bits(log)+(1<<log)
					:crush_get_bits(W_BITS-(NUM_SLOTS-1)))+p;
				if (s<0)
				{
                    return -1;
					//fprintf(stderr, "File corrupted: s=%d\n", s);
					//exit(1);
				}

				out[p++]=out[s++];
				out[p++]=out[s++];
				out[p++]=out[s++];
				while (len--!=0)
					out[p++]=out[s++];
			}
			else
				out[p++]=crush_get_bits(8);
		}

    return p;
}

