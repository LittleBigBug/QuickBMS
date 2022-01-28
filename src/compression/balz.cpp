// modified by Luigi Auriemma
// balz.cpp is written and placed in the public domain by Ilya Muravyov
//

#ifdef __GNUC__
#define _FILE_OFFSET_BITS 64
#define _fseeki64 fseeko64
#define _ftelli64 ftello64
#endif

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_DISABLE_PERFCRIT_LOCKS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../extra/mem2mem.h"

typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long long ulonglong;

namespace balz {

const int magic=0xba; // baLZ

FILE* in;
FILE* out;

class Counter
{
public:
	ushort p1;
	ushort p2;

	Counter()
		: p1(1<<15), p2(1<<15)
	{}

	uint P() const
	{
		return p1+p2;
	}

	void Update0()
	{
		p1-=p1>>3;
		p2-=p2>>6;
	}

	void Update1()
	{
		p1+=(p1^65535)>>3;
		p2+=(p2^65535)>>6;
	}
};

class Encoder
{
public:
	uint code;
	uint low;
	uint high;

	Encoder()
		: code(0), low(0), high(-1)
	{}

	void Encode(int bit, Counter& counter)
	{
		const uint mid=low+((ulonglong(high-low)*(counter.P()<<15))>>32);

		if (bit)
		{
			high=mid;
			counter.Update1();
		}
		else
		{
			low=mid+1;
			counter.Update0();
		}

		while ((low^high)<(1<<24))
		{
			putc(low>>24, out);
			low<<=8;
			high=(high<<8)|255;
		}
	}

	void Flush()
	{
		for (int i=0; i<4; ++i)
		{
			putc(low>>24, out);
			low<<=8;
		}
	}

	void Init()
	{
		for (int i=0; i<4; ++i)
			code=(code<<8)|getc(in);
	}

	int Decode(Counter& counter)
	{
		const uint mid=low+((ulonglong(high-low)*(counter.P()<<15))>>32);

		const int bit=(code<=mid);
		if (bit)
		{
			high=mid;
			counter.Update1();
		}
		else
		{
			low=mid+1;
			counter.Update0();
		}

		while ((low^high)<(1<<24))
		{
			code=(code<<8)|getc(in);
			low<<=8;
			high=(high<<8)|255;
		}

		return bit;
	}
};

const int TAB_BITS=7;
const int TAB_SIZE=1<<TAB_BITS;
const int TAB_MASK=TAB_SIZE-1;

class CM: public Encoder
{
public:
	Counter counter1[256][512];
	Counter counter2[256][TAB_SIZE];

	void Encode(int t, int c1)
	{
		int ctx=1;
		while (ctx<512)
		{
			const int bit=((t&256)!=0);
			t+=t;
			Encoder::Encode(bit, counter1[c1][ctx]);
			ctx+=ctx+bit;
		}
	}

	void EncodeIdx(int x, int c2)
	{
		int ctx=1;
		while (ctx<TAB_SIZE)
		{
			const int bit=((x&(TAB_SIZE>>1))!=0);
			x+=x;
			Encoder::Encode(bit, counter2[c2][ctx]);
			ctx+=ctx+bit;
		}
	}

	int Decode(int c1)
	{
		int ctx=1;
		while (ctx<512)
			ctx+=ctx+Encoder::Decode(counter1[c1][ctx]);

		return ctx-512;
	}

	int DecodeIdx(int c2)
	{
		int ctx=1;
		while (ctx<TAB_SIZE)
			ctx+=ctx+Encoder::Decode(counter2[c2][ctx]);

		return ctx-TAB_SIZE;
	}
} cm;

const int MIN_MATCH=3;
const int MAX_MATCH=255+MIN_MATCH;

const int BUF_BITS=25;
const int BUF_SIZE=1<<BUF_BITS;
const int BUF_MASK=BUF_SIZE-1;

byte *buf=NULL;     //[BUF_SIZE];
uint **tab=NULL;    //[1<<16][TAB_SIZE];
int *cnt=NULL;      //[1<<16];

// balz wastes a lot of memory!!! so do it only when needed
static void lame_init(void) {
    static int init = 0;
    if(!init) {
        init = 1;
        buf = new byte[BUF_SIZE];
        tab = new uint*[1<<16];
        for(int i = 0; i < (1<<16); i++)
            tab[i] = new uint[TAB_SIZE];
        cnt = new int[1<<16];
    }
        for(int i = 0; i < (1<<16); i++)
            memset(tab[i], 0, sizeof(uint) * TAB_SIZE);
}

template<bool FWD>
void e8e9_transform(int n)
{
	const int end=n-8;
	int p=0;

	while ((reinterpret_cast<int&>(buf[p])!=0x4550)&&(++p<end));

	while (p<end)
	{
		if ((buf[p++]&254)==0xe8)
		{
			int& addr=reinterpret_cast<int&>(buf[p]);
			if (FWD)
			{
				if ((addr>=-p)&&(addr<(n-p)))
					addr+=p;
				else if ((addr>0)&&(addr<n))
					addr-=n;
			}
			else
			{
				if (addr<0)
				{
					if ((addr+p)>=0)
						addr+=n;
				}
				else if (addr<n)
					addr-=p;
			}
			p+=4;
		}
	}
}

inline uint get_hash(int p)
{
	return ((reinterpret_cast<uint&>(buf[p])&0xffffff) // Little-endian
		*2654435769UL)&~BUF_MASK;
}

inline int get_pts(int len, int x)
{
	return len>=MIN_MATCH
		?(len<<TAB_BITS)-x
		:((MIN_MATCH-1)<<TAB_BITS)-8;
}

int get_pts_at(int p, int n)
{
	const int c2=reinterpret_cast<ushort&>(buf[p-2]);
	const uint hash=get_hash(p);

	int len=MIN_MATCH-1;
	int idx=TAB_SIZE;

	int max_match=n-p;
	if (max_match>MAX_MATCH)
		max_match=MAX_MATCH;

	for (int x=0; x<TAB_SIZE; ++x)
	{
		const uint d=tab[c2][(cnt[c2]-x)&TAB_MASK];
		if (!d)
			break;

		if ((d&~BUF_MASK)!=hash)
			continue;

		const int s=d&BUF_MASK;
		if ((buf[s+len]!=buf[p+len])||(buf[s]!=buf[p]))
			continue;

		int l=0;
		while (++l<max_match)
			if (buf[s+l]!=buf[p+l])
				break;
		if (l>len)
		{
			idx=x;
			len=l;
			if (l==max_match)
				break;
		}
	}

	return get_pts(len, idx);
}

void compress(bool max)
{
    lame_init();

    /*
	if (_fseeki64(in, 0, SEEK_END)!=0)
	{
		perror("Fseek failed");
		exit(1);
	}
	const long long flen=_ftelli64(in);
	if (flen<0)
	{
		perror("Ftell failed");
		exit(1);
	}
	rewind(in);

	putc(magic, out);
	fwrite((unsigned char *)&flen, 1, sizeof(flen), out); // Little-endian
    */

	int best_idx[MAX_MATCH+1];

	int n;
	while ((n=fread(buf, 1, BUF_SIZE, in))>0)
	{
		e8e9_transform<true>(n);

		//memset(tab, 0, sizeof(tab));
        for(int i = 0; i < (1<<16); i++) {
            memset(tab[i], 0, TAB_SIZE * sizeof(uint));
        }

		int p=0;

		while ((p<2)&&(p<n))
			cm.Encode(buf[p++], 0);

		while (p<n)
		{
			const int c2=reinterpret_cast<ushort&>(buf[p-2]);
			const uint hash=get_hash(p);

			int len=MIN_MATCH-1;
			int idx=TAB_SIZE;

			int max_match=n-p;
			if (max_match>MAX_MATCH)
				max_match=MAX_MATCH;

			for (int x=0; x<TAB_SIZE; ++x)
			{
				const uint d=tab[c2][(cnt[c2]-x)&TAB_MASK];
				if (!d)
					break;

				if ((d&~BUF_MASK)!=hash)
					continue;

				const int s=d&BUF_MASK;
				if ((buf[s+len]!=buf[p+len])||(buf[s]!=buf[p]))
					continue;

				int l=0;
				while (++l<max_match)
					if (buf[s+l]!=buf[p+l])
						break;
				if (l>len)
				{
					for (int i=l; i>len; --i)
						best_idx[i]=x;
					idx=x;
					len=l;
					if (l==max_match)
						break;
				}
			}

			if ((max)&&(len>=MIN_MATCH))
			{
				int sum=get_pts(len, idx)+get_pts_at(p+len, n);

				if (sum<get_pts(len+MAX_MATCH, 0))
				{
					const int lookahead=len;

					for (int i=1; i<lookahead; ++i)
					{
						const int tmp=get_pts(i, best_idx[i])+get_pts_at(p+i, n);
						if (tmp>sum)
						{
							sum=tmp;
							len=i;
						}
					}

					idx=best_idx[len];
				}
			}

			tab[c2][++cnt[c2]&TAB_MASK]=hash|p;

			if (len>=MIN_MATCH)
			{
				cm.Encode((256-MIN_MATCH)+len, buf[p-1]);
				cm.EncodeIdx(idx, buf[p-2]);
				p+=len;
			}
			else
			{
				cm.Encode(buf[p], buf[p-1]);
				++p;
			}
		}
	}

	cm.Flush();
    /*
	if (_ftelli64(in)!=flen)
	{
		fprintf(stderr, "Size mismatch\n");
		exit(1);
	}
    */
}

void decompress(int flen)
{
    lame_init();

    /*
	if (getc(in)!=magic)
	{
		fprintf(stderr, "Not in BALZ format\n");
		exit(1);
	}
	
	long long flen;
	if ((fread((unsigned char *)&flen, 1, sizeof(flen), in)!=sizeof(flen)) // Little-endian
		||(flen<0))
	{
		fprintf(stderr, "File corrupted\n");
		exit(1);
	}
    */

	cm.Init();

	while (flen>0)
	{
		int p=0;

		while ((p<2)&&(p<flen))
		{
			const int t=cm.Decode(0);
			if (t>=256)
			{
				fprintf(stderr, "File corrupted\n");
				return; //exit(1);
			}
			buf[p++]=t;
		}

		while ((p<BUF_SIZE)&&(p<flen))
		{
			const int tmp=p;
			const int c2=reinterpret_cast<ushort&>(buf[p-2]);

			const int t=cm.Decode(buf[p-1]);
			if (t>=256)
			{
				int len=t-256;
				int s=tab[c2][(cnt[c2]-cm.DecodeIdx(buf[p-2]))&TAB_MASK];

				buf[p++]=buf[s++];
				buf[p++]=buf[s++];
				buf[p++]=buf[s++];
				while (len--)
					buf[p++]=buf[s++];
			}
			else
				buf[p++]=t;

			tab[c2][++cnt[c2]&TAB_MASK]=tmp;
		}

		e8e9_transform<false>(p);

		fwrite(buf, 1, p, out);

		flen-=p;
	}
}

}

/*
int main(int argc, char* argv[])
{
	const clock_t start=clock();

	if (argc!=4)
	{
		fprintf(stderr,
			"BALZ - A ROLZ-based file compressor, v1.20\n"
			"\n"
			"Usage: BALZ command infile outfile\n"
			"\n"
			"Commands:\n"
			"  c|cx Compress (Normal|Maximum)\n"
			"  d    Decompress\n");
		exit(1);
	}

	in=fopen(argv[2], "rb");
	if (!in)
	{
		perror(argv[2]);
		exit(1);
	}

	out=fopen(argv[3], "wb");
	if (!out)
	{
		perror(argv[3]);
		exit(1);
	}

	if ((argv[1][0]=='c')
		||(argv[1][0]=='e')) // For compatibility with v1.15
	{
		printf("Compressing %s...\n", argv[2]);
		compress(argv[1][1]=='x');
	}
	else if (argv[1][0]=='d')
	{
		printf("Decompressing %s...\n", argv[2]);
		decompress();
	}
	else
	{
		fprintf(stderr, "Unknown command: %s\n", argv[1]);
		exit(1);
	}

	printf("%lld -> %lld in %.3fs\n",
		_ftelli64(in), _ftelli64(out),
		double(clock()-start)/CLOCKS_PER_SEC);

	fclose(in);
	fclose(out);

	return 0;
}
*/

extern "C" int unbalz(unsigned char *in, int insz, unsigned char *out, int outsz) {
    mem2mem_init(in, insz, out, outsz);
    balz::decompress(outsz);
    return mem2mem_ret();
}
