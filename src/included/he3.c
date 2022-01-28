// modified by Luigi Auriemma

/* DCTC - a Direct Connect text clone for Linux
 * Copyright (C) 2001 Eric Prevoteau
 *
 * he3.c: Copyright (C) Eric Prevoteau <www@a2pb.gotdns.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
/*
$Id: he3.c,v 1.4 2003/12/28 08:12:38 uid68112 Exp $
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================================= */
/* ============================================================================================= */
/* ==================================== Decoding functions ===================================== */
/* ============================================================================================= */
/* ============================================================================================= */

/******************************************************/
/*get 1 bit from the current bit position inside data */
/******************************************************/
static unsigned long he3_get_bit(unsigned char *data, unsigned long *cur_pos)
{
      unsigned long out;

      out=((unsigned long)(data[(*cur_pos)/8]>>((*cur_pos)&7)))&1;

      (*cur_pos)++;

      return out;
}

/*********************************************************/
/* get nb_bits from the current bit position inside data */
/*********************************************************/
static unsigned long he3_get_bits(unsigned char *data, unsigned long *cur_pos, int nb_bit)
{
      int i;
      unsigned long res=0;

      for(i=0;i<nb_bit;i++)
      {
            res=(res<<1)|he3_get_bit(data,cur_pos);
      }
      return res;
}

/**************************************************/
/* decompress data compressed using HE3 algorithm */
/**********************************************************/
/* input: a GByteArray containing HE3 compressed data     */
/* output: a GString containing uncompressed data or NULL */
/**********************************************************/
int decode_he3_data(unsigned char *data, unsigned char *output, int nb_output, int signature)
{
      int output_len = 0;

    if(signature) {
        if((data[0]=='H')&&(data[1]=='E')&&(data[2]=='3')&&(data[3]==0xD)) {
            // ok
        } else {
            return(-1);
        }

            /* compute the number of bytes to produce */
            nb_output=(((int)(data[8]))&255);
            nb_output<<=8;
            nb_output|=(((int)(data[7]))&255);
            nb_output<<=8;
            nb_output|=(((int)(data[6]))&255);
            nb_output<<=8;
            nb_output|=(((int)(data[5]))&255);

    } else {
        data -= 4 + 1 + 4;
    }

            unsigned char *decode_array=NULL;
            int pos;
            int nb_couple;
            int max_len=0;          /* max size of encoded pattern */
            int ttl_len=0;          /* total size of all encoded patterns */
            unsigned long offset_pattern;
            unsigned long offset_encoded;
            //int nb_output;

            /* compute the number of couples */
            nb_couple=data[9];
            nb_couple+=((((int)(data[10]))&255)<<8);

            for(pos=0;pos<nb_couple;pos++)
            {
                  int v;

                  v=(((int)(data[12+pos*2]))&255);
                  if(v>max_len)
                        max_len=v;
                  ttl_len+=v;
            }

            decode_array=malloc(1<<(max_len+1));        /* theorytically, max_len could reach up to 255 */
                                                                                                                                                /* but really, a  value above 15 is not oftenly encountered */
                                                                                                                                                /* thus the algorithm needs no more than ... 64KB */
                                                                                                                                                /* raisonnable value is upto 23 (requires 8MB) */
            /*{
                  GString *tmp;

                  tmp=g_string_new("");
                  g_string_sprintf(tmp,"he3:max_len: %d",max_len);

                  disp_msg(INFO_MSG,"decode_he3_data",tmp->str,NULL);
                  g_string_free(tmp,TRUE);
            }*/

            if(decode_array!=NULL)
            {
                  /* clear the decode array */
                  /* the decode array is technically a binary tree */
                  /* if the depth of the tree is big (let's say more than 10), */
                  /* storing the binary tree inside an array becomes very memory consumming */
                  /* but I am too lazy to program all binary tree creation/addition/navigation/destruction functions :) */
                  memset(decode_array,0,1<<(max_len+1));
                  
                  offset_pattern=8*(11+nb_couple*2);        /* position of the pattern block, it is just after the list of couples */
                  offset_encoded=offset_pattern + ((ttl_len+7)&~7);     /* the encoded data are just after */
                                                                                                                        /* the pattern block (rounded to upper full byte) */

                  /* decode_array is a binary tree. byte 0 is the level 0 of the tree. byte 2-3, the level 1, byte 4-7, the level 2, */
                  /* in decode array, a N bit length pattern having the value K is its data at the position: */
                  /* 2^N + (K&((2^N)-1)) */
                  /* due to the fact K has always N bit length, the formula can be simplified into: */
                  /* 2^N + K */
                  for(pos=0;pos<nb_couple;pos++)
                  {
                        unsigned int v_len;
                        unsigned long value;

                        v_len=(((int)(data[12+pos*2]))&255);      /* the number of bit required */

                        value=he3_get_bits(data,&offset_pattern,v_len);
                        decode_array[(1<<v_len)+ value]=data[11+pos*2];       /* the character */
                  }

                  /* now, its time to decode */
                  while(output_len!=nb_output)
                  {
                        unsigned long cur_val;
                        unsigned int nb_bit_val;

                        cur_val=he3_get_bit(data,&offset_encoded);          /* get one bit */
                        nb_bit_val=1;

                        while(decode_array[(1<<nb_bit_val) + cur_val ]==0)
                        {
                              cur_val=(cur_val<<1)|he3_get_bit(data,&offset_encoded);
                              nb_bit_val++;
                        }

                        if(output_len >= nb_output) break;    //return(-1);
                        output[output_len++] = decode_array[(1<<nb_bit_val) + cur_val ];
                  }

                  free(decode_array);
            }
      return output_len;
}

