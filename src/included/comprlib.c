// modified by Luigi Auriemma
// all the credits go to David Bourgin because comprlib is just a (untested) collection of public domain code.

/* File: codrle1.c
   Author: David Bourgin
   Creation date: 28/1/94
   Last update: 22/5/94
   Purpose: Example of RLE type 1 encoding with a file source to compress.
*/



#define comprlib_function(X) \
    int X(unsigned char *in, int insz, unsigned char *out, int outsz) { \
        comprlib_in     = in; \
        comprlib_inl    = in + insz; \
        comprlib_out    = out; \
        comprlib_outl   = out + outsz; \
        comprlib_inx    = in;



#define comprlib_block_copy(source,destination,source_size,nb_of_time)  { register unsigned int i, index=0;\
                                                                 for (i=1;i<=(nb_of_time);i++)\
                                                                     { (void)memcpy(&((destination)[index]),(source),(source_size));\
                                                                       index += (source_size);\
                                                                     }\
                                                               }
#define comprlib_fill_array(array,nb_to_fill,value)  ((void)memset((array),(value),(nb_to_fill)))



unsigned char   *comprlib_in = NULL,
                *comprlib_inl = NULL,
                *comprlib_out = NULL,
                *comprlib_outl = NULL,
                *comprlib_inx = NULL;



static int comprlib_read_byte(void) {
    unsigned char   c;

    if(comprlib_in >= comprlib_inl) return(-1);
    c = comprlib_in[0];
    comprlib_in++;
    return(c);
}
static int comprlib_read_array(unsigned char *buff, int size) {
    int     i;
    for(i = 0; i < size; i++) {
        buff[i] = comprlib_read_byte();
    }
    return(i);
}
static int comprlib_write_byte(int c) {
    if(comprlib_out >= comprlib_outl) return(-1);
    comprlib_out[0] = c;
    comprlib_out++;
    return(c);
}
static int comprlib_write_array(unsigned char *buff, int size) {
    int     i;
    for(i = 0; i < size; i++) {
        comprlib_write_byte(buff[i]);
    }
    return(i);
}
static int comprlib_write_block(int c, int size) {
    int     i;
    for(i = 0; i < size; i++) comprlib_write_byte(c);
    return(i);
}
static int comprlib_end_of_data(void) {
    if(comprlib_in >= comprlib_inl) return(1);
    return(0);
}



comprlib_function(decode_spread)
    int i, c, lucky = 0, mask = 1;

    while(!comprlib_end_of_data())
    {
        for (i = 0; i < 7; ++i) /* Read unlucky with lucky bytes spread over*/
        {
            c = comprlib_read_byte();
            if (c & 0x80)
            {
                lucky |= mask;  /* Construct lucky byte */
                c -= 0x80;      /* Remove MSB */
            }
            comprlib_write_byte(c);
            mask *= 2;
        }
        mask = 0;
        comprlib_write_byte(lucky);
    }
    return(comprlib_out - out);
    return 0;
}



comprlib_function(decode_rle1)
  unsigned char header;
  register unsigned char i;

  while (!comprlib_end_of_data())
        { header=comprlib_read_byte();
          switch (header & 128)
          { case 0:for (i=0;i<=header;i++)
                       comprlib_write_byte(comprlib_read_byte());
                   break;
            case 128:comprlib_write_block(comprlib_read_byte(),(header & 127)+2);
          }
        }
        return(comprlib_out - out);
        return 0;
}



comprlib_function(decode_rle2)
  unsigned char header_byte,byte_read,byte_to_repeat;
  register unsigned int i;

  if (!comprlib_end_of_data())
     { header_byte=comprlib_read_byte();
       do {                  /* Being that header byte is present, then there are bytes to decompress */
            if ((byte_read=comprlib_read_byte())==header_byte)
               { byte_read=comprlib_read_byte();
                 if (byte_read<3)
                    byte_to_repeat=header_byte;
                 else byte_to_repeat=comprlib_read_byte();
                 for (i=0;i<=byte_read;i++)
                     comprlib_write_byte(byte_to_repeat);
               }
            else comprlib_write_byte(byte_read);
          }
       while (!comprlib_end_of_data());
     }
     return(comprlib_out - out);
     return 0;
}



comprlib_function(decode_rle3)
  unsigned char header_byte,byte_read,nb_repetitions,
                raster[256];
  register unsigned int frame_length,
                        i;

  if (!comprlib_end_of_data())
     { header_byte=comprlib_read_byte();
       do {                  /* Being that header byte is present, then there are bytes to decompress */
            byte_read=comprlib_read_byte();
            if (byte_read==header_byte)
                             /* Encoding of a repetition of the header byte */
               { nb_repetitions=comprlib_read_byte();
                 frame_length=((unsigned int)comprlib_read_byte())+1;
                 if (!nb_repetitions)
                    for (i=1;i<=frame_length;i++)
                        comprlib_write_byte(header_byte);
                 else { comprlib_read_array(raster,frame_length);
                        for (i=0;i<=nb_repetitions;i++)
                            comprlib_write_array(raster,frame_length);
                      }
               }
            else comprlib_write_byte(byte_read);
          }
       while (!comprlib_end_of_data());
     }
    return(comprlib_out - out);
}



comprlib_function(rle4decoding)
  unsigned char byte_code;
  unsigned int frame_size=0,frame_nb;
  unsigned char frame[16705];

/*
  if (comprlib_read_byte() != 'R' || comprlib_read_byte() != 'L' || comprlib_read_byte() != '4')
  {
    fprintf (stderr, "Not encoded with CODRLE4.EXE");
    exit (BAD_FILE);
  }
*/
  while (!comprlib_end_of_data())
        { byte_code=comprlib_read_byte();
          switch (byte_code & 192)
          { case 0:          /* Frames repetition of 1 byte
                                Encoding [00xxxxxx|1 byte] */
                   frame_size=(byte_code & 63)+2;
                   comprlib_fill_array(frame,frame_size,comprlib_read_byte());
                   break;
            case 64:         /* Frames repetition of less 66 bytes
                                Encoding [01xxxxxx|xxxxxxxx|1 byte] */
                    frame_size=(((unsigned int)(byte_code & 63)) << 8)+comprlib_read_byte()+66;
                    comprlib_fill_array(frame,frame_size,comprlib_read_byte());
                    break;
            case 128:        /* Frame with several bytes
                                Encoding [10xxxxxx|yyyyyyyy|n bytes] */
                     frame_size=(byte_code & 63)+2;
                     frame_nb=((unsigned int)comprlib_read_byte())+2;
                     comprlib_read_array(frame,frame_size);
                     comprlib_block_copy(frame,frame+frame_size,frame_size,frame_nb);
                     frame_size *= frame_nb;
                     break;
            case 192:        /* No repetition
                                Encoding [110xxxxxx|n octets] or [111xxxxxx|yyyyyyyy|n octets] */
                     frame_size=byte_code & 31;
                     if (!(byte_code & 32))
                             /* Non repetition of less 33 bytes [110xxxxxx|n bytes] ? */
                        frame_size++;
                     else frame_size=(frame_size << 8)+comprlib_read_byte()+33;
                     comprlib_read_array(frame,frame_size);
                     break;
          }
          comprlib_write_array(frame,frame_size);
        }
    return(comprlib_out - out);
}

