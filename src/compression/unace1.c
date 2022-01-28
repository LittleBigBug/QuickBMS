// modified by Luigi Auriemma
// Copyright by Marcel Lemke
/* ------------------------------------------------------------------------ */
/*                                                                          */
/*      These are the decompression algorithms.                             */
/*      Don't change here anything (apart from memory allocation perhaps).  */
/*      Any changes will very likely cause bugs!                            */
/*                                                                          */
/* ------------------------------------------------------------------------ */
/*  ML - 01/2004: changed licence to GPL                                    */
/* ------------------------------------------------------------------------ */

//#include "os.h"
#include <string.h>
typedef unsigned short USHORT;
typedef short          SHORT ;
typedef unsigned short UWORD ;
typedef short          WORD  ;
typedef unsigned long  ULONG ;
typedef long           LONG  ;
typedef unsigned char  UCHAR ;
typedef char           CHAR  ;
typedef unsigned       UINT  ;
typedef int            INT   ;

#if defined(AMIGA) || defined(UNIX)
 #include <string.h> // mem*()
#endif
#if defined(DOS) || defined(WIN16) || defined(WINNT) || defined(OS2)
 #if defined(VA_CPP)
  #include <memory.h>
 #else
  #include <mem.h>   // mem*()
 #endif
#endif
  
#include <stdio.h>   // printf()
#include <stdlib.h>  // malloc()

/*
#include "globals.h"
#include "portable.h"
#include "uac_comm.h"
#include "uac_crc.h"
#include "uac_dcpr.h"
#include "uac_sys.h"
*/


//--------- buffers: increase sizes when possible to speed up the program

#define size_rdb  1024
#define size_wrb  2048
//#define size_buf  1024

//--------- (de-)compressor constants

#define maxdic      22
#define maxwd_mn    11
#define maxwd_lg    11
#define maxwd_svwd   7
#define maxlength  259
#define maxdis2    255
#define maxdis3   8191
#define maxcode   (255+4+maxdic)
#define svwd_cnt    15
#define max_cd_mn (256+4+(maxdic+1)-1)
#define max_cd_lg (256-1)


//-------- buffers
static ULONG buf_rd[size_rdb];
static CHAR  buf_wr[size_wrb];
//static ULONG *buf_rd        =0;
//static CHAR  *buf           =0;
//static CHAR  *buf_wr        =0;
//static UCHAR *readbuf       =0;

//-------- decompressor variables
static SHORT rpos           =0,
      dcpr_do        =0,
      dcpr_do_max    =0,
      blocksize      =0,
      dcpr_dic       =0,
      dcpr_oldnum    =0,
      bits_rd        =0,
      dcpr_frst_file =0;
static USHORT dcpr_code_mn[1 << maxwd_mn],
       dcpr_code_lg[1 << maxwd_lg];
static UCHAR dcpr_wd_mn[maxcode + 2],
      dcpr_wd_lg[maxcode + 2],
      wd_svwd[svwd_cnt];
static ULONG dcpr_dpos      =0,
      //cpr_dpos2      =0,
      dcpr_dicsiz    =0,
      dcpr_dican     =0,
      dcpr_size      =0,
      dcpr_olddist[4]={0,0,0,0},
      code_rd        =0;

static CHAR *dcpr_text      =0;

//-------- quicksort
static USHORT sort_org[maxcode + 2];
static UCHAR sort_freq[(maxcode + 2) * 2];



static unsigned char    *static_in  = NULL,
                        *static_inl = NULL;
INT  read_adds_blk(CHAR * buffer, INT len) {
    memset(buffer, 0, len); // fixed size read
    INT     t = static_inl - static_in;
    if(t <= 0) return 0;
    if(len > t) len = t;
    memcpy(buffer, static_in, len);
    static_in += len;
    return len;
}



static void memset16(USHORT * dest, SHORT val, INT len)  // fills short-array with
{                                                 // value
   while (len--)
      *(dest++) = val;
}


//------------------------------ QUICKSORT ---------------------------------//
#define xchg_def(v1,v2) {INT dummy;\
                         dummy=v1; \
                         v1=v2;    \
                         v2=dummy;}

static void sortrange(INT left, INT right)
{
   INT  zl = left,
        zr = right,
        hyphen;

   hyphen = sort_freq[right];

   //divides by hyphen the given range into 2 parts
   do
   {
      while (sort_freq[zl] > hyphen)
         zl++;
      while (sort_freq[zr] < hyphen)
         zr--;
      //found a too small (left side) and
      //a too big (right side) element-->exchange them
      if (zl <= zr)
      {
         xchg_def(sort_freq[zl], sort_freq[zr]);
         xchg_def(sort_org[zl], sort_org[zr]);
         zl++;
         zr--;
      }
   }
   while (zl < zr);

   //sort partial ranges - when very small, sort directly
   if (left < zr) {
      if (left < zr - 1)
         sortrange(left, zr);
      else if (sort_freq[left] < sort_freq[zr])
      {
         xchg_def(sort_freq[left], sort_freq[zr]);
         xchg_def(sort_org[left], sort_org[zr]);
      }
   }

   if (right > zl) {
      if (zl < right - 1)
         sortrange(zl, right);
      else if (sort_freq[zl] < sort_freq[right])
      {
         xchg_def(sort_freq[zl], sort_freq[right]);
         xchg_def(sort_org[zl], sort_org[right]);
      }
   }
}

static void quicksort(INT n)
{
   INT  i;

   for (i = n + 1; i--;)
      sort_org[i] = i;
   sortrange(0, n);
}

//------------------------------ read bits ---------------------------------//
static void readdat(void)
{
   UINT i;

   i = (size_rdb - 2) << 2;
   rpos -= size_rdb - 2;
   buf_rd[0] = buf_rd[size_rdb - 2];
   buf_rd[1] = buf_rd[size_rdb - 1];
   read_adds_blk((CHAR *) & buf_rd[2], i);
#ifdef HI_LO_BYTE_ORDER
   {
      ULONG *p;
      i>>=2;    // count LONGs not BYTEs
      p=&buf_rd[2];
      while (i--)
      {
         LONGswap(p);
         p++; 
      }
   }
#endif
}

#define addbits(bits)                                   \
{                                                       \
  rpos+=(bits_rd+=bits)>>5;                             \
  bits_rd&=31;                                          \
  if (rpos==(size_rdb-2)) readdat();                    \
  code_rd=(buf_rd[rpos] << bits_rd)                     \
    +((buf_rd[rpos+1] >> (32-bits_rd))&(!bits_rd-1));   \
}

//-------------------------- HUFFMAN ROUTINES ------------------------------//
static INT  makecode(UINT maxwd, UINT size1_t, UCHAR * wd, USHORT * code)
{
   UINT maxc,
        size2_t,
        l,
        c,
        i,
        max_make_code;

   memcpy(&sort_freq, wd, (size1_t + 1) * sizeof(CHAR));
   if (size1_t)
      quicksort(size1_t);
   else
      sort_org[0] = 0;
   sort_freq[size1_t + 1] = size2_t = c = 0;
   while (sort_freq[size2_t])
      size2_t++;
   if (size2_t < 2)
   {
      i = sort_org[0];
      wd[i] = 1;
      size2_t += (size2_t == 0);
   }
   size2_t--;

   max_make_code = 1 << maxwd;
   for (i = size2_t + 1; i-- && c < max_make_code;)
   {
      maxc = 1 << (maxwd - sort_freq[i]);
      l = sort_org[i];
      if (c + maxc > max_make_code)
      {
         dcpr_do = dcpr_do_max;
         return (0);
      }
      memset16(&code[c], l, maxc);
      c += maxc;
   }
   return (1);
}

static INT  read_wd(UINT maxwd, USHORT * code, UCHAR * wd, INT max_el)
{
   UINT c,
        i,
        j,
        num_el,
        l,
        uplim,
        lolim;

   memset(wd, 0, max_el * sizeof(CHAR));
   memset(code, 0, (1 << maxwd) * sizeof(SHORT));

   num_el = code_rd >> (32 - 9);
   addbits(9);
   if (num_el > max_el)
      num_el = max_el;

   lolim = code_rd >> (32 - 4);
   addbits(4);
   uplim = code_rd >> (32 - 4);
   addbits(4);

   for (i = -1; ++i <= uplim;)
   {
      wd_svwd[i] = code_rd >> (32 - 3);
      addbits(3);
   }
   if (!makecode(maxwd_svwd, uplim, wd_svwd, code))
      return (0);
   j = 0;
   while (j <= num_el)
   {
      c = code[code_rd >> (32 - maxwd_svwd)];
      addbits(wd_svwd[c]);
      if (c < uplim)
         wd[j++] = c;
      else
      {
         l = (code_rd >> 28) + 4;
         addbits(4);
         while (l-- && j <= num_el)
            wd[j++] = 0;
      }
   }
   if (uplim)
      for (i = 0; ++i <= num_el;)
         wd[i] = (wd[i] + wd[i - 1]) % uplim;
   for (i = -1; ++i <= num_el;)
      if (wd[i])
         wd[i] += lolim;

   return (makecode(maxwd, num_el, wd, code));

}

static INT  calc_dectabs(void)
{
   if (!read_wd(maxwd_mn, dcpr_code_mn, dcpr_wd_mn, max_cd_mn)
       || !read_wd(maxwd_lg, dcpr_code_lg, dcpr_wd_lg, max_cd_lg))
      return (0);

   blocksize = code_rd >> (32 - 15);
   addbits(15);

   return (1);
}

//------------------------- LZW DECOMPRESSION ------------------------------//
static void wrchar(CHAR ch)
{
   dcpr_do++;

   dcpr_text[dcpr_dpos] = ch;
   dcpr_dpos++;
   dcpr_dpos &= dcpr_dican;
}

static void copystr(LONG d, INT l)
{
   INT  mpos;

   dcpr_do += l;

   mpos = dcpr_dpos - d;
   mpos &= dcpr_dican;

   if ((mpos >= dcpr_dicsiz - maxlength) || (dcpr_dpos >= dcpr_dicsiz - maxlength))
   {
      while (l--)
      {
         dcpr_text[dcpr_dpos] = dcpr_text[mpos];
         dcpr_dpos++;
         dcpr_dpos &= dcpr_dican;
         mpos++;
         mpos &= dcpr_dican;
      }
   }
   else
   {
      while (l--)
         dcpr_text[dcpr_dpos++] = dcpr_text[mpos++];
      dcpr_dpos &= dcpr_dican;
   }
}

static void decompress(void)
{
   INT  c,
        lg,
        i,
        k;
   ULONG dist;

   while (dcpr_do < dcpr_do_max)
   {
      if (!blocksize)
         if (!calc_dectabs())
            return;

      addbits(dcpr_wd_mn[(c = dcpr_code_mn[code_rd >> (32 - maxwd_mn)])]);
      blocksize--;
      if (c > 255)
      {
         if (c > 259)
         {
            if ((c -= 260) > 1)
            {
               dist = (code_rd >> (33 - c)) + (1L << (c - 1));
               addbits(c - 1);
            }
            else
               dist = c;
            dcpr_olddist[(dcpr_oldnum = (dcpr_oldnum + 1) & 3)] = dist;
            i = 2;
            if (dist > maxdis2)
            {
               i++;
               if (dist > maxdis3)
                  i++;
            }
         }
         else
         {
            dist = dcpr_olddist[(dcpr_oldnum - (c &= 255)) & 3];
            for (k = c + 1; k--;)
               dcpr_olddist[(dcpr_oldnum - k) & 3] = dcpr_olddist[(dcpr_oldnum - k + 1) & 3];
            dcpr_olddist[dcpr_oldnum] = dist;
            i = 2;
            if (c > 1)
               i++;
         }
         addbits(dcpr_wd_lg[(lg = dcpr_code_lg[code_rd >> (32 - maxwd_lg)])]);
         dist++;
         lg += i;
         copystr(dist, lg);
      }
      else
         wrchar(c);
   }
}

//---------------------------- BLOCK ROUTINES ------------------------------//
static INT  decompress_blk(CHAR * buf, UINT len)
{
   LONG old_pos = dcpr_dpos;
   INT  i;

   dcpr_do = 0;
   if ((dcpr_do_max = len - maxlength) > dcpr_size)
      dcpr_do_max = dcpr_size;
   if ((LONG) dcpr_size > 0 && dcpr_do_max)
   {
      decompress();
      if (dcpr_do <= len)
      {
         if (old_pos + dcpr_do > dcpr_dicsiz)
         {
            i = dcpr_dicsiz - old_pos;
            memcpy(buf, &dcpr_text[old_pos], i);
            memcpy(&buf[i], dcpr_text, dcpr_do - i);
         }
         else
            memcpy(buf, &dcpr_text[old_pos], dcpr_do);
      }
   }
   dcpr_size -= dcpr_do;
   return (dcpr_do);
}
/*
static INT unstore(CHAR * buf, UINT len)
{
   UINT rd = 0,
        i,
        pos = 0;

   while ((i = read_adds_blk((CHAR *) buf_rd, (INT) ((i = ((len > dcpr_size) ? dcpr_size : len)) > size_rdb ? size_rdb : i))) != 0)
   {
      rd += i;
      len -= i;
      dcpr_size -= i;
      memcpy(&buf[pos], buf_rd, i);
      pos += i;
   }
   for (i = 0; i < rd; i++)
   {
      dcpr_text[dcpr_dpos] = buf[i];
      dcpr_dpos++;
      dcpr_dpos &= dcpr_dican;
   }
   return (INT)rd;
}

static INT  dcpr_adds_blk(CHAR * buf, UINT len)
{
   INT  r;

   switch (fhead.TECH.TYPE)
   {
      case TYPE_STORE:
         r = unstore(buf, len);
         break;
      case TYPE_LZ1:
         r = decompress_blk(buf, len);
         break;
      default:
         printf("\nFile compressed with unknown method. Decompression not possible.\n");
         f_err = ERR_OTHER;
         r = 0;
   }
   rd_crc = getcrc(rd_crc, buf, r);
   return r;
}
*/

//----------------------------- INIT ROUTINES ------------------------------//
static void dcpr_init(void)
{
   dcpr_frst_file = 1;

   dcpr_dic = 20;
   while ((dcpr_text = malloc(dcpr_dicsiz = (LONG) 1 << dcpr_dic))==NULL)
      dcpr_dic--;
   dcpr_dican = dcpr_dicsiz - 1;
}
/*
static void dcpr_init_file(void)
{
   UINT i;

   if (head.HEAD_FLAGS & ACE_PASSW)
   {
      printf("\nFound passworded file. Decryption not supported.\n");
      f_err = ERR_OTHER;
      return;
   }

   rd_crc = CRC_MASK;
   dcpr_size = fhead.SIZE;
   if (fhead.TECH.TYPE == TYPE_LZ1)
   {
      if ((fhead.TECH.PARM & 15) + 10 > dcpr_dic)
      {
         printf("\nNot enough memory or dictionary of archive too large.\n");
         f_err = ERR_MEM;
         return;
      }

      i = size_rdb * sizeof(LONG);
      read_adds_blk((CHAR *) buf_rd, i);
#ifdef HI_LO_BYTE_ORDER
      {
         ULONG *p;
         i>>=2;    // count LONGs not BYTEs
         p=buf_rd;
         while (i--)
         {
            LONGswap(p);
            p++; 
         }
      }
#endif
      code_rd = buf_rd[0];
      bits_rd = rpos = 0;

      blocksize = 0;
   }
   if (!adat.sol || dcpr_frst_file)
      dcpr_dpos = 0;

   dcpr_oldnum = 0;
   memset(&dcpr_olddist, 0, sizeof(dcpr_olddist));

   dcpr_frst_file = 0;
}
*/



int unace1(unsigned char *in, int insz, unsigned char *out, int outsz) {
    //unsigned char   buf_wr[size_wrb];
    unsigned char   *o = out;

    static_in  = in;
    static_inl = in + insz;

    INT rd,
        ret = -1;

    dcpr_size = outsz;

    dcpr_init();

    read_adds_blk((CHAR *) buf_rd, size_rdb * sizeof(LONG));
      code_rd = buf_rd[0];
      bits_rd = rpos = 0;
      blocksize = 0;

    for(;;) {
        rd = decompress_blk(buf_wr, size_wrb);
        if((int)rd <= 0) break;
        if((o + rd) > (out + outsz)) break; //goto quit;    // I prefer to not quit
        memcpy(o, buf_wr, rd);
        o += rd;
    }
    ret = o - out;
//quit:
    if (dcpr_text) free(dcpr_text);
    return ret;
}


