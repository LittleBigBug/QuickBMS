// modified by Luigi Auriemma

/* Known to compile and work with tcc in win32 & gcc on Linux (with warnings)
------------------------------------------------------------------------------
readable.c: My random number generator, ISAAC.
(c) Bob Jenkins, March 1996, Public Domain
You may use this code in any way you wish, and it is free.  No warrantee.
------------------------------------------------------------------------------
*/
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>



    /* external results */
    static uint32_t randrsl[256], randcnt;
     
    /* internal state */
    static    uint32_t mm[256];
    static    uint32_t aa=0, bb=0, cc=0;

    static void isaac()
    {
       uint32_t i,x,y;
     
       cc = cc + 1;    /* cc just gets incremented once per 256 results */
       bb = bb + cc;   /* then combined with bb */
     
       for (i=0; i<256; ++i)
       {
         x = mm[i];
         switch (i%4)
         {
         case 0: aa = aa^(aa<<13); break;
         case 1: aa = aa^(aa>>6); break;
         case 2: aa = aa^(aa<<2); break;
         case 3: aa = aa^(aa>>16); break;
         }
         aa              = mm[(i+128)%256] + aa;
         mm[i]      = y  = mm[(x>>2)%256] + aa + bb;
         randrsl[i] = bb = mm[(y>>10)%256] + x;
       }
       // not in original readable.c
       randcnt = 0;
    }
     
    /* if (flag!=0), then use the contents of randrsl[] to initialize mm[]. */
    #define mix(a,b,c,d,e,f,g,h) \
    { \
       a^=b<<11; d+=a; b+=c; \
       b^=c>>2;  e+=b; c+=d; \
       c^=d<<8;  f+=c; d+=e; \
       d^=e>>16; g+=d; e+=f; \
       e^=f<<10; h+=e; f+=g; \
       f^=g>>4;  a+=f; g+=h; \
       g^=h<<8;  b+=g; h+=a; \
       h^=a>>9;  c+=h; a+=b; \
    }
     
    static void randinit(int flag)
    {
       int i;
       uint32_t a,b,c,d,e,f,g,h;
       aa=bb=cc=0;
       a=b=c=d=e=f=g=h=0x9e3779b9;  /* the golden ratio */
     
       for (i=0; i<4; ++i)          /* scramble it */
       {
         mix(a,b,c,d,e,f,g,h);
       }
     
       for (i=0; i<256; i+=8)   /* fill in mm[] with messy stuff */
       {
         if (flag)                  /* use all the information in the seed */
         {
           a+=randrsl[i  ]; b+=randrsl[i+1]; c+=randrsl[i+2]; d+=randrsl[i+3];
           e+=randrsl[i+4]; f+=randrsl[i+5]; g+=randrsl[i+6]; h+=randrsl[i+7];
         }
         mix(a,b,c,d,e,f,g,h);
         mm[i  ]=a; mm[i+1]=b; mm[i+2]=c; mm[i+3]=d;
         mm[i+4]=e; mm[i+5]=f; mm[i+6]=g; mm[i+7]=h;
       }
     
       if (flag)
       {        /* do a second pass to make all of the seed affect all of mm */
         for (i=0; i<256; i+=8)
         {
           a+=mm[i  ]; b+=mm[i+1]; c+=mm[i+2]; d+=mm[i+3];
           e+=mm[i+4]; f+=mm[i+5]; g+=mm[i+6]; h+=mm[i+7];
           mix(a,b,c,d,e,f,g,h);
           mm[i  ]=a; mm[i+1]=b; mm[i+2]=c; mm[i+3]=d;
           mm[i+4]=e; mm[i+5]=f; mm[i+6]=g; mm[i+7]=h;
         }
       }
     
       isaac();            /* fill in the first set of results */
       randcnt=0;        /* prepare to use the first set of results */
    }
     
     
    // Get a random 32-bit value 0..MAXINT
    static uint32_t iRandom()
    {
        uint32_t r = randrsl[randcnt];
        ++randcnt;
        if (randcnt >255) {
            isaac();
            randcnt = 0;
        }
        return r;
    }
     
     
    // Get a random character in printable ASCII range
    static char iRandA(char modulo, char start)
    {	
        return iRandom() % modulo + start;
    }
     
     
    // Seed ISAAC with a string
    static void iSeed(char *seed, int m, int flag)
    {
        uint32_t i;
        for (i=0; i<256; i++) mm[i]=0;
        for (i=0; i<256; i++)
        {
        // in case seed has less than 256 elements
            if (i>m) randrsl[i]=0;  else randrsl[i] = seed[i];
        }
        // initialize ISAAC with seed
        randinit(flag);
    }
     
     
    #define MOD 95
    #define START 32
    // cipher modes for Caesar
    enum ciphermode {
        mEncipher, mDecipher, mNone 
    };
     
     
    // XOR cipher on random stream. Output: ASCII string
    static void Vernam(char *msg, int l, char modulo, char start)
        {
            uint32_t i;
            // XOR message
            for (i=0; i<l; i++) 
                msg[i] = iRandA(modulo,start) ^ msg[i];
        }
     
    // Caesar-shift a printable character
    static char Caesar(enum ciphermode m, char ch, char shift, char modulo, char start)
        {
            int n;
            if (m == mDecipher) shift = -shift;
            n = (ch-start) + shift;
            n = n % modulo;
            if (n<0) n += modulo;
            return start+n;
        }
     
    // Caesar-shift a string on a pseudo-random stream
    static void CaesarStr(enum ciphermode m, char *msg, int l, char modulo, char start)
        {
            int i;
            // Caesar-shift message
            for (i=0; i<l; i++) 
                msg[i] = Caesar(m, msg[i], iRandA(modulo,start), modulo, start);
        }


// use do_encrypt -1 for vernam
void isaacx_crypt(unsigned char *key, int keylen, unsigned char *data, int datasz, int do_encrypt) {
    // LA: the code was terrible and I'm lazy so let's make one non-threadsafe
    //     function that can be used indipendently to set key or encrypt or
    //     doing both

    if(keylen >= 0) {
        iSeed(key, keylen, 1);
    }

    if(datasz > 0) {
        if(do_encrypt < 0) {
            Vernam(data, datasz, MOD, START);
        } else {
            CaesarStr(do_encrypt ? mEncipher : mDecipher, data, datasz, MOD, START);
        }
    }
}
