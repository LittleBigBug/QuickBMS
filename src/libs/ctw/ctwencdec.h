/* ctwencdec - contains functions to Encode and Decode a byte
   Note: All the data structures have to be initialized first! */

void init_ctxstring();		/* put the first context from filebuffer in ctxstring */
void Encode(unsigned char u);	/* encodes u */
unsigned char Decode(void);	/* decodes and returns next byte */
