// modified by Luigi Auriemma
// I adopted a very lame solution to do the work very quickly since there is no interest in supporting this compression, but it works

/*
*	Dark-src-A (C)kvark, Nov 2006
*	the BWT-DC scheme universal compressor
*	*** Open Linux+Win32 version ***
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>

namespace undark {

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

static void assert(int test) { }
#define MENC	0x01
#define MDEC	0x02

typedef uint tbord;

#define BOT	(1<<14)
#define RESTUP	8

#define NB	32
#define LIN	27
typedef ushort tfreq;
//max dist = 128m
typedef tfreq bicon[32];
typedef tfreq locon[LIN+1];
static int rb[0x100];

	static tbord lo,hi,rng,code;
	static unsigned char *f; char act;
	//write encoded byte
	static void nben()	{ *f++ = lo>>24; }
	//read byte to decode
	static void nbde()	{ code = (code<<8)|(*f); f++; /*fgetc(f)*/; }
	//set work file and mode
	static void Set(char nact, unsigned char *nf)	{ act=nact; f=nf; }
	static void Init()	{ lo=0; rng=(uint)(-1); }
	static void StartDecode()	{ for(char i=0;i<4;i++) nbde(); }

static void parse(tfreq toff, tfreq tran)	{
	lo += rng*toff; rng *= tran;
	//main [en|de]coding loop
	do	{ hi = lo+rng;
		if((lo^hi)>=1<<24)	{
			if(rng>BOT) break;
			tbord lim = hi&0xFF000000;
			if(hi-lim >= lim-lo) lo=lim;
				else hi=lim-1;
		}do	{//shift
			act==MDEC ? nbde():nben();
			lo<<=8; hi<<=8;
		}while((lo^hi) < 1<<24);
		rng = hi-lo;
	}while(rng<BOT);
}


class EilerCoder	{
private:
	bicon cbit[NB];
	void Update(tfreq*,uchar);
	void PutLog(int);
	int GetLog();
	void Parse(tfreq,uchar);
public:
	tfreq *tt;
	void Start();
	void InitBits(tfreq*,int);
	void InitFreq(locon*,int);
	void Finish();
	void EncodeEl(long,int*);
	long DecodeEl(int*);
};

#define FMB	12
#define FMAX	(1<<FMB)
//number of bits with contexts
#define LOBIT	(3+1)


//update frequences table
void EilerCoder::Update(tfreq *tab, uchar log)	{
	tfreq add = 5; tab[log] += add;
	if((tab[0] += add) >= FMAX)	{ int i;
		for(tab[0]=0,i=1; i<=LIN; i++)
			tab[0] += (++tab[i] >>= 1);
	}
}
//parse log
void EilerCoder::Parse(tfreq off, uchar log)	{
	parse(off, tt[log]);
	Update(tt,log);
}
//decode distance log
int EilerCoder::GetLog()	{ int log;
	rng /= tt[0];
	tfreq fcur,val = (code - lo)/rng;
	for(fcur=0,log=1; fcur+tt[log] <= val; log++)
		fcur += tt[log];
	Parse(fcur,log); return log;
}
void EilerCoder::InitBits(tfreq *v0, int num)	{
	num <<= 5; // for 32 bits
	while(num--) *v0++ = FMAX>>1;
}
void EilerCoder::Start()	{
	InitBits(cbit[0], NB);
}
void EilerCoder::InitFreq(locon *vr, int num)	{
	for(int i=0; i<num; i++)	{
		for(int j=1; j <= LIN; j++)
			vr[i][j] = 1;
		vr[i][0] = LIN;
	}
}
//Decoding routine
long EilerCoder::DecodeEl(int *pv)	{
	tbord val; ulong ran;
	tfreq *u; uchar log;
	log = GetLog();
	assert(log<LIN);
	u = cbit[log]; ran = 1<<(log-1);
	for(int i=log-2; i>=0; i--,u++)	{
		bool upd = (i>=log-LOBIT);
		rng >>= FMB;
		val = code - lo;
		if(val >= u[0] * rng)	{
			ran |= 1<<i;
			parse(u[0], FMAX-u[0]);
			if(upd) u[0] -= u[0]>>RESTUP;
		}else	{
			parse(0, u[0]);
			if(upd) u[0] += (FMAX-u[0])>>RESTUP;
		}
	}pv[0]=log;
	return ran;
}


class Ptax	{
	struct SYMBOL {
		long fir;
		uchar pref;
		//context
		long df;
	}sym[256];
	locon dis[4][NB];
	long las[256];
	long lp,tm;
	int num,arm;
	EilerCoder bc;
	//mtf operations
	uchar m[256],cat[256];
	int pov,was;
	void setcon(SYMBOL*);
	void postup(SYMBOL*,long);
public:
	uchar *bin;
	void ran_encode(ulong,uchar);
	ulong ran_decode(uchar);
	void Beready();
	void Perform(int*,uchar*,int);
	uint Decode(uchar*);
};

//group hashing constants
#define GRLOG	4
#define GRSIZE	(256>>GRLOG)
#define GRNUM	(1<<GRLOG)
//cat[char] = char's group id

static int dc;

static int getlog(long ran)	{ int log;
	for(log=0; ran>>log; log++);
	return log;
}
//prepare context
void Ptax::setcon(SYMBOL *ps)	{
	dc = getlog(ps->df);
	if(dc>11) dc=11;
	int fl = (pov<2?0:(pov<8?1:2));
	bc.tt = dis[fl][dc];
}
//update context 
void Ptax::postup(SYMBOL *ps, long num)	{
	ps->df = (ps->df + num)>>1;
	//fprintf(fd,"%lu\t%d\n",ran,ps-sym);
}
ulong Ptax::ran_decode(uchar cs)	{
	ulong ran; setcon(sym+cs);
	ran = bc.DecodeEl(&pov)-1;
	postup(sym+cs,ran);
	return ran;
}
//init frequences tabs
void Ptax::Beready()	{ int i;
	for(i=0;i<256;i++) sym[i].df = 1000;
	pov = 2; bc.Start();
	for(i=0; i<4; i++)	{
		bc.InitFreq(dis[i], NB);
	}
}

uint Ptax::Decode(uchar *bot)	{
	int i,cs,n = ran_decode(0);
	if(!n) return 0;
	//read init & sort by dist
	for(num=0,cs=0; cs<256; cs++)	{
		rb[cs] = ran_decode(0);
		if(!rb[cs]) continue;
		las[cs] = lp = ran_decode(cs);
		for(i=num; i>0 && lp<las[m[i-1]]; i--)
			m[i] = m[i-1];
		m[i] = cs; num++;
	}
	if(num == 1)	{
		memset(bot,m[0],n); return n;
	}//read all others
	for(i=0; i<n;)	{
		int j,lim; ulong ra;
		cs = m[0]; tm = las[m[1]];
		while(i<tm) bot[i++] = cs;
		if(!--rb[cs]) tm = n;
		else tm += ran_decode(cs);
		//cmp border & move dword
		ra = *(ulong*)(m+1);
		for(j=0;;)	{
			if( (j+=4) >= num ) { lim=num; break; }
			if(tm+j <= las[m[j]]) { lim=j; break; }
			*(ulong*)(m+j-4) = ra;
			ra = *(ulong*)(m+j+1);
		}//the rest
		for(j-=3; j<lim && tm+j > las[m[j]]; j++)
			m[j-1] = m[j];
		las[ m[j-1]=cs ] = tm+j-1;
	}return n;
}



static int block,n;
static int *p;
static int baza;
static uchar *bin;
static Ptax px;

static void step()	{}
static void Reset()	{ px.Beready(); }

static int InitAll(int NBS, uchar act, unsigned char *ff, int *mem)	{
	p = (int*)malloc(sizeof(int)*((block=NBS)+1));
	if(p == NULL) return -1;
	//mem[0] += sizeof(r) + sizeof(rb) + sizeof(r2a);
	//mem[0] += sizeof(px) + block*sizeof(int);
	if(n == -1) return -1;
	Set(act,ff); return 0;
}

//count cumulated frequences
static void cumulate()	{ int i,cl;
	i=n; cl=256; do	{ cl--;
		i -= rb[cl], rb[cl] = i;
	}while(cl);
}

static unsigned char *fs;
static uint DecodeBlock(uchar *bin)	{
	register int i,pos; step();
	if(!(n=px.Decode(bin))) return 0;
	baza = px.ran_decode(0);
	memset(rb, 0, 0x100*sizeof(int));
	step(); //Heh
	for(i=0; i<n; i++) rb[bin[i]]++;
	#define nextp(id) p[rb[bin[id]]++] = id
	cumulate(); nextp(baza);
	for(i=0; i<baza; i++)	nextp(i);
	for(i=baza+1; i<n; i++)	nextp(i);
	#undef nextp
	for(pos=baza,i=0; i<n; i++)	{
		*fs++ = bin[pos = p[pos]];
		assert(pos>=0 && pos<n);
	}return n;
}



//number of zero terminating bytes
#define TERM	320
//must be approximately >= 2*DEEP+8

static uchar *s_bin;
static int BS; uchar *cbin;

static void Prepare(uchar act, unsigned char *ff, int st_block)	{
	int memory = 0; BS = st_block;
	cbin = TERM + (s_bin = (uchar*)malloc(BS+TERM+1));
	memory += (BS+TERM+1)*sizeof(uchar);
	int rez = InitAll(BS,act,ff,&memory);
	if(!s_bin || rez) exit(1);
	//here = 0;
}
static void Extract(bool single, int len)	{
	while(len)	{
		int n = DecodeBlock(cbin); 
		len -= n; //rendicate(n); 
	}
}



int DecodeFile(unsigned char *in, int insz, unsigned char *out, int outsz) {
	int pred_len = outsz;
    int     st_block = 0x400000;
    if(!memcmp(in, "!dark", 5)) {
        in += 5;
        st_block = *(int *)in; in += 4;
        do { in++; } while(in[-1]);
        in += 4;
        pred_len = *(int *)in; in += 4;
    }
	fs = out;
	Prepare(MDEC,in, st_block);
	Reset();
	Init();
    StartDecode();
	Extract(true, pred_len);
	return fs - out;
}


}



extern "C" int opendark_DecodeFile(unsigned char *in, int insz, unsigned char *out, int outsz) {
    return undark::DecodeFile(in, insz, out, outsz);
}
