#ifndef __DECOMPRESS_H__
#define __DECOMPRESS_H__

#include "btree.h"

class cDecompressor
{
public:
  cDecompressor ();
  ~cDecompressor ();
  int Decompress (Uint8 * source, Uint8 * dest, int  & srclen, int dstlen);
  void Reset();
private:

    bTree tree,			/**< The big bTree for decompression */
   *readCur;			/**< Current location in the bTree */
  int readBits;			/**< Current bit read */
  Uint16 readValue;		/**< Current value read */
};

#endif
