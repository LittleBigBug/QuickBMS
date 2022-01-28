// modified by Luigi Auriemma
/***********************************************************
*
*    bpe2.c
*
*      Byte Pair Encoding ファイル符号化のテスト
*      ブロックのヘッダを省略できる場合省略する
*
***********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

static
unsigned char   *infile   = NULL,
                *outfile  = NULL,
                *infilel  = NULL,
                *outfilel = NULL;
static int xgetc(void *skip) {
    if(infile >= infilel) return(-1);
    return(*infile++);
}
//static int xputc(int chr, void *skip) {
    //if(outfile >= outfilel) return(-1);
    //*outfile++ = chr;
    //return(chr);
//}
static int xread(void *buff, int a, int b, void *X) {
    int     len,
            inlen;

    len = a * b;
    inlen = (infilel - infile);
    if(len > inlen) len = inlen;
    memcpy(buff, infile, len);
    infile += len;
    return(len);
}
static int xwrite(void *buff, int a, int b, void *X) {
    int     len,
            outlen;

    len = a * b;
    outlen = (outfilel - outfile);
    if(len > outlen) len = outlen;
    memcpy(outfile, buff, len);
    outfile += len;
    return(len);
}

typedef unsigned char  Uchar;
typedef unsigned short Ushort;
typedef unsigned int   Uint;

/* グローバル変数の定義 */
#define  HEADER_SIZE 4
char file_header[HEADER_SIZE] = "BPE2";
char file_ext[] = ".bpe2";
char input_fname[FILENAME_MAX];
char output_fname[FILENAME_MAX];
int  mode;      /* 'e':encode 'd':decode */

FILE *infp, *outfp;      /* 入力ファイル, 出力ファイル */

#define TRUE    1
#define FALSE   0
#define BUFMIN  128
#define BUFMAX  32511         /* 0x7eff */
int bufsize = 4096;           /* データ作業領域のサイズ */

Uchar  ptsize;                /* ペア表の大きさ */
Uchar  ptbuf[256*3];          /* 符号化用ペア表バッファ */

Uchar  pairtable1[256];       /* 一時的ペア表(1番目の文字) 符号化モードでは文字が使われているかどうかの判定用 */
Uchar  pairtable2[256];       /* 一時的ペア表(2番目の文字) */

Uchar  srcbuf[BUFMAX + 16];   /* 元のデータのバッファ */
Uchar  workbuf[BUFMAX + 16];  /* 圧縮データのバッファ */
Ushort *paircount;            /* ペア出現用カウンタ(mallocで領域確保する, 符号化モードのみ使用) */
int    ob_count = 0;          /* *outbuf[] を使っている数 */
Uchar  *outbuf[256];          /* データ出力用バッファメモリ(mallocで領域確保する, 符号化モードのみ使用) */

/*--------------------------------------------------------*/
/*    復号処理                                            */
/*--------------------------------------------------------*/

/* workbuf[]を展開しsrcbuf[]へ書込 */
static int decode_buf(int bfs, int isize)
{
	int wpos = 0, spos = 0;
	Uchar stackbuf[256], stackhead = 0;  /* デコード用スタック */
	
	while (wpos < isize || stackhead > 0)
	{
		Uchar ch;
		if (!stackhead) {
			/* スタックが空の時、データから1Byte読込 */
			ch = workbuf[wpos++];
		} else {
			/* スタックから1Byte読込 */
			ch = stackbuf[--stackhead];
		}

		while (TRUE) {
			/* ペア表から文字を得る */
			if (ch == pairtable1[ch]) {
				/* データをそのまま1Byte書込 */
				if (spos >= bfs)  return(-1);
				srcbuf[spos++] = ch;
				break;
			}
			/* データをスタックへ入れる */
			stackbuf[stackhead++] = pairtable2[ch];
			ch = pairtable1[ch];
		}
	}
	return spos;
}

/* 復号 */
static int decode(void)
{
	int osize = 0;
	
	/* バッファサイズの読込 */
	bufsize = xgetc(infp);
	bufsize = (bufsize << 8) | xgetc(infp);
	if (bufsize < BUFMIN || BUFMAX < bufsize)  return(-1);
	//printf("作業領域用メモリバッファ: %dByte\n", bufsize);
	while (TRUE)
	{
		int i, typ;
		int pts, ch, c2;
		int isize, srcsize;
		/* ブロックヘッダ読込 */
		if (ob_count > 0) {
			if (xread(workbuf, 1, bufsize, infp) != (size_t)bufsize)  return(-1);
			xwrite(workbuf, 1, bufsize, outfp);
			srcsize = bufsize;
			ob_count--;
		} else {
			int headcode = xgetc(infp);
			if (headcode == EOF)  break;
			if (headcode == 0x7f) {
				ob_count = xgetc(infp);
				headcode = bufsize;
			} else {
				headcode = (headcode << 8) | xgetc(infp);
			}
			
			typ = headcode >> 15;
			isize = headcode & 0x7fff;
			/* ブロックデータの読込 */
			if (xread(workbuf, 1, isize, infp) != (size_t)isize)  return(-1);

			/* ペア表を初期化 */
			for (i = 0; i < 256; i++) {
				pairtable1[i] = i;
			}
			if (typ) {
				/* ペア表の読込 */
				if ((pts = xgetc(infp)) < 0)  return(-1);
				for (i = 0; i < pts; i++) {
					if ((ch = xgetc(infp)) < 0)  return(-1);
					if ((c2 = xgetc(infp)) < 0)  return(-1);
					pairtable1[ch] = c2;
					if ((c2 = xgetc(infp)) < 0)  return(-1);
					pairtable2[ch] = c2;
				}
			}
			srcsize = decode_buf(bufsize, isize);  /* 復号処理 */
            if(srcsize < 0) return(-1);
			xwrite(srcbuf, 1, srcsize, outfp);
		}
		
		/* 途中経過表示 */
		osize += srcsize;
		//printf("             %d\r", osize);
	}
    return(0);
}



int unbpe2(unsigned char *in, int insz, unsigned char *out, int outsz) {
    infile   = in;
    infilel  = in + insz;
    outfile  = out;
    outfilel = out + outsz;
    if(decode() < 0) return(-1);
    return(outfile - out);
}

