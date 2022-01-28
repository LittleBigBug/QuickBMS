// modified by Luigi Auriemma

// lzpxj.cpp, version: 1.2h, authors: ilia muraviev, jan ondrus
#include <cstdio>
#include <string.h>
//using namespace std;
//#pragma warn -8027 // turn off the 8027 warning

// use exe-filter
#define EXE_FILTER

static int mem;
static unsigned char *infile; // input file stream
static unsigned char *outfile; // output file stream

class cencoder {
private:
 unsigned int low;
 unsigned int range;
 unsigned int help;
 unsigned int buffer;
 unsigned int count;

 void shift() {
  if (count == 0) {
   if (low < (unsigned int)(255 << 24)) {
    *outfile++ = buffer;
    while (help > 0) {
     *outfile++ = 255;
     help--;
    }
    buffer = low >> 24;
   } else {
    help++;
   }
  } else {
   *outfile++ = buffer + 1;
   count = 0;
   while (help > 0) {
    *outfile++ = 0;
    help--;
   }
   buffer = low >> 24;
  }
  low <<= 8;
 }

public:
 void init() {
  low = 0;
  range = static_cast<unsigned int>(-1);
  help = 0;
  buffer = 0;
  count = 0;
 }

 void encode(unsigned int u, unsigned int f, unsigned int t) {
  unsigned int temp = low;
  low += u * (range /= t);
  if (low < temp) {
   count++;
  }
  range *= f;
  while (range < (1 << 24)) {
   range <<= 8;
   shift();
  }
 }

 void flush() {
  if (++low == 0) {
   count++;
  }
  for (int i = 0; i < 5; i++) {
   shift();
  }
 }
};

class cdecoder {
private:
 unsigned int low;
 unsigned int range;
 unsigned int buffer;

public:
 void init() {
  low = 0;
  range = static_cast<unsigned int>(-1);
  for (int i = 0; i < 5; i++) {
   buffer = (buffer << 8) | (*infile++);
  }
 }

 void update(unsigned int u, unsigned int f) {
  low += u * range;
  range *= f;
  while (range < (1 << 24)) {
   buffer = (buffer << 8) | (*infile++);
   low <<= 8;
   range <<= 8;
  }
 }

 unsigned int freq(unsigned int t) {
  return ((buffer - low) / (range /= t));
 }
};

class cstate1 {
public:
 unsigned char freq[504];
 unsigned int total;
 int ctx;

 void rescale() {
  total = 0;
  for (int i = 0; i < 504; i++) {
    freq[i] -= (freq[i] >> 1);
    total += freq[i];
  }
 }

 void init(int v = 0, int e = 1) {
  for (int i = 1; i < 504; i++) {
   freq[i] = v;
  }
  freq[0] = e;
  total = 503 * v + e;
  ctx = -1 - v;
 }

 void update(int s) {
  if (freq[s] > 245) {
   rescale();
  }
  total += 6;
  freq[s] += 6;
 }
};


class cstate2 {
private:
 void rescale() {
  total = 0;
  for (int i = 0; i < 256; i++) {
   freq[i] -= freq[i] >> 1;
   total += freq[i];
  }
 }

public:
 unsigned short freq[256];
 unsigned int total;
 int con_c;

 void init(int n) {
  for (int i = 0; i < n; i++) {
   freq[i] = 1;
  }
  total = n;
  con_c = 16;
 }

 void update(int s) {
  if (total > (1 << 15)) {
   rescale();
  }
  freq[s] += 512;
  total += 512;
 }
};

#define ADD_BITS(a,b) {\
 res = res * (a) / (b);\
 while (res > 4096) { bits++; res >>= 1; }\
}

class cmodel2 {
private:
 cencoder *encoder;
 cdecoder *decoder;
 cstate2 state[1298];
 int chain;
 bool esc;
 int stack;
 int o1, o2, o3, o4, t;
 int c1, c2, c3, c4;
 int gen_c[4];
 int res;
 int c, x, h, i;
 unsigned char *b;

 void exclude() {
  stack = state[c].freq[x];
  state[c].freq[x] = 0;
  state[c].total -= stack;
 }

 void restore() {
  state[c].freq[x] = stack;
  state[c].total += stack;
 }

 void h_value() {
  c1 = b[-1] + 18;
  c2 = b[-2] + 274;
  c3 = b[-3] + 530;
  c4 = b[-4] + (i & 1) * 256 + 786;
  if (o2 > o1 && o3 > o1 && o4 > o1) c = c1;
  else if (o3 > o2 && o4 > o2) c = c2;
  else if (o3 > o4) c = c4;
  else c = c3;
  if (x != -1) exclude();

  unsigned int z = (gen_c[i & 3] + state[c].con_c);
  if (z > 1024) z = 1024;
  h = (z * z / 48 * state[c].total) >> 18;
 }

 void encode(int s, int c, unsigned int h = 0) {
  unsigned int u = 0;
  for (int i = 0; i < s; i++) {
   u += state[c].freq[i];
  }
  encoder->encode(u + s * h, state[c].freq[s] + h, state[c].total + h * 256);
 }

 unsigned char decode(int c, unsigned int h = 0) {
  unsigned int f = decoder->freq(state[c].total + h * 256);
  unsigned int u = 0;
  unsigned char s = 0;
  while ((u += state[c].freq[s] + h) <= f) s++;
  decoder->update(u - state[c].freq[s] - h, state[c].freq[s] + h);
  return s;
 }

public:
 void init(cencoder *e, cdecoder *d) {
  encoder = e;
  decoder = d;
  for (int i = 0; i < 1298; i++) {
	  state[i].init(i < 16 ? 3 : 256);
  }
  chain = 0;
  esc = false;
  o1 = 0; o2 = 0; o3 = 0; o4 = 0; t = 0;
  for (int j = 0; j < 4; j++) {
    gen_c[j] = 16;
  }
  res = 4096;
 }

 void free() {
 }
 
 void context(unsigned char *b0, int x0, int i0) {
  x = (x0 > 0?x0 - 1:-1);
  b = b0;
  i = i0;
  h = -1;
 }

 void enc(int s) {
  int f = (s > 256?1:(s == 0?2:0));
  encode(f, chain);
  if (f == 1) {
   encode(s - 257, 16);
  } else if (f == 0) {
   h_value();
   encode(s - 1, c, h);
  }
 }

 int dec() {
  int f = decode(chain);
  int s = 0;
  if (f == 1) {
   s = 257 + decode(16);
  } else if (f == 0) {
   h_value();
   s = 1 + decode(c, h);
  }
  return s;
 }

 void upd(int s, int &bits) {
  int f = (s > 256?1:(s == 0?2:0));

  ADD_BITS(state[chain].total, state[chain].freq[f]);
  state[chain].update(f);
  if (f == 1) {
   ADD_BITS(state[16].total, state[16].freq[s - 257]);
   state[16].update(s - 257);
  } else if (f == 0) {
   if (h == -1) h_value();
   s--;
   ADD_BITS(state[c].total + h * 256, state[c].freq[s] + h);
   if (x != -1) restore();
   int x1 = 8 * state[c1].total / state[c1].freq[s];
   int x2 = 8 * state[c2].total / state[c2].freq[s];
   int x3 = 8 * state[c3].total / state[c3].freq[s];
   int x4 = 8 * state[c4].total / state[c4].freq[s];
   if (x1 > x2) o1++, o2--; else o1--, o2++;
   if (x1 > x3) o1++, o3--; else o1--, o3++;
   if (x1 > x4) o1++, o4--; else o1--, o4++;
   if (x2 > x3) o2++, o3--; else o2--, o3++;
   if (x2 > x4) o2++, o4--; else o2--, o4++;
   if (x3 > x4) o3++, o4--; else o3--, o4++;
   if (t++ >= 256) {
    t = 0;
    o1 -= (o1 >> 1);
    o2 -= (o2 >> 1);
    o3 -= (o3 >> 1);
    o4 -= (o4 >> 1);
   }

   int y = state[c].total / state[c].freq[s];
   if (y < 185) {
    gen_c[i & 3] = gen_c[i & 3] * 9 / 10;
    state[c].con_c = state[c].con_c * 9 / 10;
   } else {
    y = (y >> 11)?20:9;
    gen_c[i & 3] += y;
    state[c].con_c += y;
   }

   state[c1].update(s);
   state[c2].update(s);
   state[c3].update(s);
   state[c4].update(s);
  }
  chain = (chain * 2 + f) % 16;
 }
};

class cmodel1 {
private:
 cencoder *encoder;
 cdecoder *decoder;
 cstate1 *state;
 int excl[504];
 int e, x, cx, c2, c3;
 bool cond;
 int res;
 int ms;
 int stack;
 int c;

 void exclude() {
  stack = state[c].freq[x];
  state[c].freq[x] = 0;
  state[c].total -= stack;
 }

 void restore() {
  state[c].freq[x] = stack;
  state[c].total += stack;
 }

 void encode(int s) {
  if (x != -1) exclude();
  unsigned int u = 0;
  for (int i = 0; i < s; i++) u += state[c].freq[i];
  encoder->encode(u, state[c].freq[s], state[c].total);
  if (x != -1) restore();
  }

 unsigned int decode(int c0) {
  c = c0;
  if (x != -1) exclude();
  unsigned int s = 0;
  unsigned int f = decoder->freq(state[c].total);
  unsigned int u = 0;
  while ((u += state[c].freq[s]) <= f) s++;
  decoder->update(u - state[c].freq[s], state[c].freq[s]);
  if (x != -1) restore();
  return s;
 }

public:
 void init(cencoder *e, cdecoder *d) {
  encoder = e;
  decoder = d;
  ms = (1 << (11 + mem));
  state = new cstate1[ms + 257];
  for (int i = 0; i < ms + 257; i++) {
   state[i].init((i == 0 ? 1 : 0), 1);
  }
  res = 4096;
 }

 void free() {
  delete[ ] state;
 }

 void context(unsigned char *b, int x0, int i) {
  x = x0;
  cx = b[-1] + (b[-2] << 8) + (b[-3] << 16);
  c3 = 257 + ((cx / 3737 + cx * 3737 + cx / 523 + cx) & ((ms >> 1) - 1));
  if (state[c3].ctx != cx && state[c3].ctx != -1) {
   cx = b[-1] + (b[-2] << 8);
   c3 = 257 + ((cx * 11 + cx / 11 + cx) & ((ms >> 1) - 1)) + (ms >> 1);
  }
  c2 = b[-1] + 1;
  cond = (state[c3].ctx != cx);
 }

 void sign_excl(int c) {
  int i;
  if (e == 0) for (i = 0; i < 504; i++) excl[i] = 0;
  for (i = 1; i < 504; i++) if (state[c].freq[i]!=0) excl[i] = 1;
  e = 1;
 }


 void do_excl(int part) {
  int i;
  if (e == 0) return;
  switch (part) {
	 case 0:
		 for (i = 0; i < 504; i++) if (excl[i] != 0) {
			 excl[i] = state[c].freq[i] + 1;
			 state[c].total-=state[c].freq[i];
			 state[c].freq[i] = 0;
		 }
		 break;
	 case 1:
		 for (i = 0; i < 504; i++) if (excl[i] != 0) {
			 state[c].freq[i] = excl[i] - 1;
			 state[c].total+=state[c].freq[i];
		 }

  }
 }

 void enc(int s) {
  c = c3;
  e = 0;

  if (state[c].freq[s] == 0 || s == 0 || cond) {
	  if (!cond) {
		  encode(0);
		  if (state[c].freq[s] == 0) sign_excl(c);
	  }

	  c = c2;
	  if (state[c].freq[s] == 0 || s == 0) {
		  do_excl(0); encode(0); do_excl(1);
		  if (state[c].freq[s] == 0) sign_excl(c);
		  c = 0;
	  }
  }
  do_excl(0); encode(s); do_excl(1);
 }

 int dec() {
  int s = 0;
  e = 0;
  if (!cond) {
	  s = decode(c3);
	  if (s == 0) sign_excl(c3);
  }
  if (s == 0) {
	  c=c2;
	  do_excl(0); s = decode(c2); do_excl(1);
      if (s == 0) {
		  sign_excl(c2);
		  c=0;
		  do_excl(0); s = decode(0); do_excl(1);
	  }
  }
  return s;
 }

 void upd(int s, int &bits) {
  c = c3;
  if (state[c].freq[s] == 0 || s == 0 || cond) {
   if (!cond) ADD_BITS(state[c].total, state[c].freq[0]);
   state[c].ctx = cx;
   state[c].update(0);
   state[c].update(s);
   c = c2;
   if (state[c].freq[s] == 0 || s == 0) {
    ADD_BITS(state[c].total, state[c].freq[0]);
    state[c].update(0);
    state[c].update(s);
    c = 0;
   }
  }
  ADD_BITS(state[c].total, state[c].freq[s]);
  state[c].update(s);
 }
};

class cmodel {
private:
 cmodel1 m1;
 cmodel2 m2;
 int o1[257], o2[257];
 int ac;

 int model_number() {
  int q = (o1[ac] + o1[256] > o2[ac] + o2[256]);
  if (o1[ac] + o2[ac] > 1024) {
   o1[ac] -= (o1[ac] >> 2);
   o2[ac] -= (o2[ac] >> 2);
  }
  if (o1[256] + o2[256] > 1024) {
   o1[256] -= (o1[256] >> 2);
   o2[256] -= (o2[256] >> 2);
  }
  return q;
 }

 void update_models(int s) {
  int io1 = 0, io2 = 0;
  m1.upd(s, io1);
  m2.upd(s, io2);
  o1[ac] += io1*2;
  o1[256] += io1;
  o2[ac] += io2*2;
  o2[256] += io2;
 }

public:
 void init(cencoder *e, cdecoder *d) {
  m1.init(e, d);
  m2.init(e, d);
  for (ac = 0; ac < 257; ac++) {
	  o1[ac] = 0;
	  o2[ac] = 0;
  }
 }

 void free() {
  m1.free();
  m2.free();
 }

 void context(unsigned char *b, int x, int i) {
  ac = b[-1];
  m1.context(b, x, i);
  m2.context(b, x, i);
 }

 void encode_symbol(int s) {
  if (model_number() == 0) m1.enc(s); else m2.enc(s);
  update_models(s);
 }

 int decode_symbol() {
  int s;
  if (model_number() == 0) s = m1.dec(); else s = m2.dec();
  update_models(s);
  return s;
 }
};

class ctable {
private:
 enum {
  N2 = (1 << 16)
 };
 unsigned int *tab8;
 unsigned int *tab4;
 unsigned int tab2[N2];
 unsigned int tab1[256];
 unsigned char *buf;
 int N4;
 
public:
 void dealloc() {
  delete[ ] tab8;
  delete[ ] tab4;
 }

 void alloc() {  
  N4 = (1 << (16 + mem));
  tab8 = new unsigned int[N4];
  tab4 = new unsigned int[N4];
 }

 void init(unsigned char *b) {
  buf = b;
  for (int i = 0; i < N4; i++) {
   tab4[i] = 0;
   tab8[i] = 0;
  }
  for (int j = 0; j < N2; j++) {
   tab2[j] = 0;
  }
  for (int k = 0; k < 256; k++) {
   tab1[k] = 0;
  }
 }

 int pos(int i, bool update) {
  unsigned int c4 = *(reinterpret_cast<unsigned int *>(buf + i - 4));
  unsigned int c2 = *(reinterpret_cast<unsigned short *>(buf + i - 2));
  unsigned char c1 = *(buf + i - 1);
  unsigned int h4 = ((c4 >> 12) ^ (c4 << 11) ^ (c4 >> 20) ^ (c4 << 3) ^ c4) & (N4 - 1);

  unsigned int c8 = *(reinterpret_cast<unsigned int *>(buf + i - 8));
  unsigned int h8 = ((c8 >> 11) ^ (c8 << 12) ^ (c8 >> 19) ^ (c8 << 4) ^ c8) & (N4 - 1);
  h8 = h4 ^ h8;

  int t8 = tab8[h8];
  int t4 = tab4[h4];
  int t2 = tab2[c2];
  int t1 = tab1[c1];
  if (update) {
    tab8[h8] = i;
    tab4[h4] = i;
    tab2[c2] = i;
    tab1[c1] = i;
  }

  if (t8 != 0 && *(reinterpret_cast<unsigned int *>(buf + t8 - 8)) == c8
	          && *(reinterpret_cast<unsigned int *>(buf + t8 - 4)) == c4) return t8;
  if (t4 != 0 && *(reinterpret_cast<unsigned int *>(buf + t4 - 4)) == c4) return t4;
  if (t2 != 0) return t2; else return t1;
 }
};

static cmodel model;
static ctable table;

static int N;
static unsigned char *buf;
//static int n;
static int i;
static int j;

static int p; // match position
static int l; // match length
static int x;

#ifdef EXE_FILTER
static int offset;

static void exe_filter(int size, bool decode_mode) {
 int j = 0;
 int c = 0, a0 = 1024, a1 = 1024;
 while (j < (size - 4)) {
  // detect
  switch (buf[j]) {
   case 131: a0 -= a0 / 5 * 2; break;
   case 232: a1 -= a1 / 5 * 2; break;
   case 137: case 139: case 8: case 192: case 4:case 133: case 141:
   case 0: case 1: case 69: case 80: case 116: case 117: case 255: break;
   default: if (a0 < 16*1024) a0++; if (a1 < 16*1024) a1++;
  }
  if (a0+a1 < 256) c = 16*1024; else
  if (a0+a1 > 16*1024) c = 0; else if (c > 0) c--;

  // filter
  if ((c != 0) && (buf[j] == 0xe8 || buf[j] == 0xe9) && (buf[j+4] == 255 || buf[j+4] == 0)) {
   int x = (buf[j + 3] << 16) + (buf[j + 2] << 8) + (buf[j + 1]);
   if (decode_mode) {
    x += ((1<<24) - (offset + j));
   } else {
    if ((buf[j+3] > 127 && buf[j+4]==255) || (buf[j+3] <= 127 && buf[j+4]==0)) buf[j+4]=0; else buf[j+4]=255;
    x += offset + j;
   }
   buf[j + 3] = (x >> 16) & 255;
   buf[j + 2] = (x >> 8) & 255;
   buf[j + 1] = x & 255;
   if (decode_mode) {
    if ((buf[j+3] > 127 && buf[j+4]==255) || (buf[j+3] <= 127 && buf[j+4]==0)) buf[j+4]=0; else buf[j+4]=255;
   } 
   j += 5;
  } else if ((c != 0) && (buf[j] == 0xe8 || buf[j] == 0xe9)) {
   j += 5;
  } else {
   j++;
  }
 }
 offset += N;
}

#endif

/*
static void encode() {
 cencoder e;
 e.init();
 model.init(&e, 0);
 table.alloc();
 while ((n = fread(buf, 1, N, infile)) > 0) {

#ifdef EXE_FILTER
  exe_filter(n, false);
#endif

  table.init(buf);

  x = -1;
  for (i = 0; (i < 8) && (i < n); i++) {
   e.encode(buf[i] + 1, 1, 257);
  }

  while (i < n) {
   p = table.pos(i, true);
   model.context(&buf[i], x, i);

   j = i;
   l = 0;
   while ((buf[p] == buf[j]) && (j < n)) {
    p++;
    j++;
    l++; // increment match length
   }

   if (l >= 3) {
    int qp,qj,ql,qq;
    int max = 0, chosen_l;
    for (qq = l; qq >= (3 > l-24?3:l-24); qq--) {
	 qp = table.pos(i+qq, false);
	 qj = i+qq;
	 ql = 0;
	 while ((buf[qp] == buf[qj]) && (qj < n)) {
	  qp++;
	  qj++;
	  ql++; // increment match length
	 }
	 if (max==0 || (ql+qq > max && ql >= 3)) {
	  max = ql+qq;
	  chosen_l = qq;
	 }
	}
   p -= l;
   l = chosen_l;
   p += l;
   
    j = i;
    i += l;
    while (l >=249) {
     model.encode_symbol(503);
     l -= 246;
    }
    model.encode_symbol(254 + l); 
    x = buf[p] + 1;
   } else { // not long enough match
    model.encode_symbol(buf[i] + 1);
    i++;
    x = -1;
   }
  }
 }
 if (i >= 8 && i < N) {
  model.context(&buf[i], x, i);
  model.encode_symbol(0);
 } else {
  e.encode(0, 1, 257);
 }
 e.flush();
 table.dealloc();
 model.free();
}
*/

static void decode() {
 cdecoder d;
 d.init();
 model.init(0, &d);
 table.alloc();
 j = -1;
 while (j != 0) {
  table.init(buf);
  x = -1;
  for (i = 0; (i < 8); i++) { 
   j = d.freq(257);
   d.update(j, 1);
   if (j == 0) break;
   buf[i] = j - 1;
  }

  if (j != 0) while (i < N) {
   p = table.pos(i, true);
   model.context(&buf[i], x, i);

   if ((j = model.decode_symbol()) > 256) {
    l = -254;
    while (j == 503) {
     j = model.decode_symbol();
     l += 246;
    }
    l += j;
    while ((l > 0) && (i < N)) {
     buf[i++] = buf[p];
     p++;
     l--;
    }
    x = buf[p] + 1;
   } else if (j != 0) {
    buf[i++] = j - 1;
    x = -1;
   } else break;
  }
  
#ifdef EXE_FILTER
  exe_filter(i, true);
#endif

  memcpy(outfile, buf, i);
  outfile += i;
 }
 table.dealloc();
 model.free();
}

extern "C" int lzpx_unpack(unsigned char *in, unsigned char *out) {
    infile   = in;
    outfile  = out;

 mem = (*infile++) - '0';
 N = (1 << (20 + mem));
 buf = new unsigned char[N];

  decode();
 
 delete[ ] buf;

 return (outfile - out);
}
