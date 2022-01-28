/*
original code by RupertAvery:
https://gbatemp.net/threads/tvc-fpk-tool.207232/
http://www.mediafire.com/?vty5jymlmm2

modified by Luigi Auriemma
*/

static int ctrl_ptr;
static int data_ptr;
static int ctrl_bit;

//#include "stdio.h"
//#include "common.h"
//#include "compress.h"
//#include "cache.h"

// Tests the current buffer position for RLE encoding
// This will scan ahead of the current position until
// the byte changes or the maximum of 256 is reached
static void init_comp()
{
	// set to initial values
	ctrl_ptr = 0;
	data_ptr = 1;
	ctrl_bit = 7;
}

static int check_rle(char *sbuf, int sptr, int *length, int *pos)
{
	int tlength = 0;
	char *p = &sbuf[sptr];
	(*length)=tlength;
	(*pos)= 1;

    if(sptr < *pos) return 0;
	while((memcmp(p, &sbuf[sptr - *pos],1)==0) && ((*length) < 256))
	{
		(*length)++;
		p++;
	}

	//while( (*(p+tlength)==sbuf[sptr - 1]) && (tlength < 256) )
	//	tlength++;

	// set outgoing variables
	//(*length)=tlength;
	//(*pos)= 1;

	return((*length)>4);
}

// Scans backwards from the current buffer position
// for a similar byte sequence. Scanning area is limited
// to last 8192 bytes.  
static int check_window(char *sbuf, int sptr, int *length, int *pos)
{
   int tlength = 2;
   int tpos = -1;

   if(sptr < tlength) return 0;
   char *p = &sbuf[sptr - tlength];
   char *curr = &sbuf[sptr];
   char *savep = p;

   // scan area = 8192 bytes
   // no scanning beyond start of buffer
   // limit to 256 bytes match length
   while (((curr - p) < 8192) && (p >= sbuf) && (tlength < 256))
   {
       while((memcmp(curr, p, tlength)==0) && (tlength < 256))
       {
            savep = p;
            tlength++;
       }
       p--;
   }

   tlength--;
   tpos = (curr - savep);

   *length = tlength;
   *pos = tpos;

   if((tlength==2) && (tpos > 255)) return 0;

   return(tlength >= 2);
}

static void write_bit(char *dbuf, int bit)
{
    if(ctrl_bit==-1)
    {
        ctrl_bit=7;
        ctrl_ptr=data_ptr;
        data_ptr=ctrl_ptr+1;
    }
    dbuf[ctrl_ptr] |= bit << ctrl_bit;
    ctrl_bit--;
}


static void write_comp_a(char *dbuf, int length, int pos)
{
    int ctr = 2;
    write_bit(dbuf, 0);
    write_bit(dbuf, 0);

    length -= 2;
    while(ctr>0)
    {
        write_bit(dbuf, (length >> 1) & 0x01);
        length = (length << 1) & 0x02;
        ctr--;
    }

    dbuf[data_ptr++] = (~pos + 1) & 0xFF;
}

static void write_comp_b(char *dbuf, int length, int pos)
{
    write_bit(dbuf, 0);
    write_bit(dbuf, 1);

    pos = (~pos + 1) << 3;

    if(length<=9)
    {
        pos |= ((length - 2) & 0x07);
    }
    // else lower 3 bits are empty...

    dbuf[data_ptr++] = (pos >> 8) & 0xFF;
    dbuf[data_ptr++] = pos & 0xFF;

    // ... and next byte encodes full length
    if(length>9)
    {
        dbuf[data_ptr++] = (length - 1) & 0xFF;
    }
}

static void write_comp(char *dbuf, int length, int pos)
{
	if((pos > 255) || (length > 5))
    {
        write_comp_b(dbuf,length,pos);
    } else
    {
        write_comp_a(dbuf,length,pos);
    }
}

static void write_nocomp(char *dbuf, char *sbuf, int sptr)
{
    write_bit(dbuf,1);
    dbuf[data_ptr++] = sbuf[sptr] & 0xFF;
}

int prs_8ing_compress(unsigned char *_sbuf, int slen, unsigned char *_dbuf) {
    int length;
    int pos;
    int sptr = 0;

	init_comp();

//	filetitle = filename + strlen(filename);
//	while(*filetitle!='\\')filetitle--;
//  filetitle++;

    while(sptr < slen)
    {
		//if(callback) callback(file, (int)(((float)sptr / (float)slen) * 100.0));

		if(check_rle(_sbuf,sptr,&length,&pos)>0)
        {
			write_comp(_dbuf,length,pos);

/*			if((pos > 255) || (length > 5))
            {
                write_comp_b(_dbuf,length,pos);
            } else
            {
                write_comp_a(_dbuf,length,pos);
            }*/
            sptr += length;

		} else {
            if (check_window(_sbuf,sptr,&length,&pos))
            {
				write_comp(_dbuf,length,pos);
/*				if((pos > 255) || (length > 5))
				{
					write_comp_b(_dbuf,length,pos);
				} else
				{
					write_comp_a(_dbuf,length,pos);
				}
	*/
				sptr += length;
            } else {
                write_nocomp(_dbuf,_sbuf,sptr);
                sptr++;
            }
        }
    }

    return data_ptr;

	//show_cache();
	//free_cache();
    //(*dlen) = data_ptr;
	//(*flen) = slen;
/*
	memset(sbuf, 0, slen);
	uncomp(sbuf, slen, dbuf, *dlen);

	fp = fopen("C:\\temp.bin", "wb");
    fwrite(sbuf, slen, 1, fp);
    fclose(fp);
*/
    //free(sbuf);
	//return dbuf;
}
