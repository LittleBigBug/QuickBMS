#ifndef __BTREE_H__
#define __BTREE_H__

//#include <SDL/SDL_types.h>

#include <stdint.h>
typedef enum {
	SDL_FALSE = 0,
	SDL_TRUE  = 1
} SDL_bool;
typedef int8_t		Sint8;
typedef uint8_t		Uint8;
typedef int16_t		Sint16;
typedef uint16_t	Uint16;
typedef int32_t		Sint32;
typedef uint32_t	Uint32;


class bTree
{
public:
  bTree ();
  ~bTree ();
  class bTree *zero;	/**< right hand side */
  class bTree *one;	/**< left hand side */
  Uint16 value;		/**< node value */
  bool hit;		/**< leaf or branch? */
};

#endif
