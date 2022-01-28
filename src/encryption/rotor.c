/*

THIS IS THE ROTOR MODULE (with the Python functions removed) FROM THE PYTHON SOURCE CODE!!!
modified by Luigi Auriemma

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <math.h>

/* Rotor objects */

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

typedef struct {
	int seed[3];
    	short key[5];
	int  isinited;
	int  size;
	int  size_mask;
    	int  rotors;
	unsigned char *e_rotor;		     /* [num_rotors][size] */
	unsigned char *d_rotor;		     /* [num_rotors][size] */
	unsigned char *positions;	     /* [num_rotors] */
	unsigned char *advances;	     /* [num_rotors] */
} Rotorobj;


#define is_rotor(v)		((v)->ob_type == &Rotor_Type)


/* This defines the necessary routines to manage rotor objects */

static void
set_seed(Rotorobj *r)
{
	r->seed[0] = r->key[0];
	r->seed[1] = r->key[1];
	r->seed[2] = r->key[2];
	r->isinited = FALSE;
}
	
/* Return the next random number in the range [0.0 .. 1.0) */
static double
r_random(Rotorobj *r)
{
	int x, y, z;
	double val, term;

	x = r->seed[0];
	y = r->seed[1];
	z = r->seed[2];

	x = 171 * (x % 177) - 2 * (x/177);
	y = 172 * (y % 176) - 35 * (y/176);
	z = 170 * (z % 178) - 63 * (z/178);
	
	if (x < 0) x = x + 30269;
	if (y < 0) y = y + 30307;
	if (z < 0) z = z + 30323;
	
	r->seed[0] = x;
	r->seed[1] = y;
	r->seed[2] = z;

	term = (double)(
		(((double)x)/(double)30269.0) + 
		(((double)y)/(double)30307.0) + 
		(((double)z)/(double)30323.0)
		);
	val = term - (double)((int)term);   //(double)floor((double)term);

	if (val >= 1.0)
		val = 0.0;

	return val;
}

static short
r_rand(Rotorobj *r, short s)
{
	return (short)((short)(r_random(r) * (double)s) % s);
}

static void
set_key(Rotorobj *r, char *key, int len)
{
	unsigned long k1=995, k2=576, k3=767, k4=671, k5=463;
	size_t i;
	//size_t len = strlen(key);

	for (i = 0; i < len; i++) {
		unsigned short ki = key[i];

		k1 = (((k1<<3 | k1>>13) + ki) & 65535);
		k2 = (((k2<<3 | k2>>13) ^ ki) & 65535);
		k3 = (((k3<<3 | k3>>13) - ki) & 65535);
		k4 = ((ki - (k4<<3 | k4>>13)) & 65535);
		k5 = (((k5<<3 | k5>>13) ^ ~ki) & 65535);
	}
	r->key[0] = (short)k1;
	r->key[1] = (short)(k2|1);
	r->key[2] = (short)k3;
	r->key[3] = (short)k4;
	r->key[4] = (short)k5;

	set_seed(r);
}



/* These define the interface to a rotor object */
static Rotorobj *
rotorobj_new(int num_rotors, char *key, int keysz)
{
	Rotorobj    *xp;

//	xp = unsigned char_New(Rotorobj, &Rotor_Type);
//	if (xp == NULL)
//		return NULL;

    xp = malloc(sizeof(Rotorobj));
    memset(xp, 0, sizeof(Rotorobj));
    xp->size = sizeof(Rotorobj);
	set_key(xp, key, keysz);

	xp->size = 256;
	xp->size_mask = xp->size - 1;
	xp->size_mask = 0;
	xp->rotors = num_rotors;
	xp->e_rotor = NULL;
	xp->d_rotor = NULL;
	xp->positions = NULL;
	xp->advances = NULL;

	if (!(xp->e_rotor = malloc(num_rotors * xp->size)))
		goto finally;
	if (!(xp->d_rotor = malloc(num_rotors * xp->size)))
		goto finally;
	if (!(xp->positions = malloc(num_rotors)))
		goto finally;
	if (!(xp->advances = malloc(num_rotors)))
		goto finally;

	return xp;

  finally:
	if (xp->e_rotor)
		free(xp->e_rotor);
	if (xp->d_rotor)
		free(xp->d_rotor);
	if (xp->positions)
		free(xp->positions);
	if (xp->advances)
		free(xp->advances);
//	Py_DECREF(xp);
//	return (Rotorobj*)PyErr_NoMemory();
    return(xp);
}


/* These routines implement the rotor itself */

/*  Here is a fairly sophisticated {en,de}cryption system.  It is based on
    the idea of a "rotor" machine.  A bunch of rotors, each with a
    different permutation of the alphabet, rotate around a different amount
    after encrypting one character.  The current state of the rotors is
    used to encrypt one character.

    The code is smart enough to tell if your alphabet has a number of
    characters equal to a power of two.  If it does, it uses logical
    operations, if not it uses div and mod (both require a division).

    You will need to make two changes to the code 1) convert to c, and
    customize for an alphabet of 255 chars 2) add a filter at the begining,
    and end, which subtracts one on the way in, and adds one on the way
    out.

    You might wish to do some timing studies.  Another viable alternative
    is to "byte stuff" the encrypted data of a normal (perhaps this one)
    encryption routine.

    j'

 */

/* Note: the C code here is a fairly straightforward transliteration of a
 * rotor implemented in lisp.  The original lisp code has been removed from
 * this file to for simplification, but I've kept the docstrings as
 * comments in front of the functions.
 */


/* Set ROTOR to the identity permutation */
static void
RTR_make_id_rotor(Rotorobj *r, unsigned char *rtr)
{
	register int j;
	register int size = r->size;
	for (j = 0; j < size; j++) {
		rtr[j] = (unsigned char)j;
	}
}


/* The current set of encryption rotors */
static void
RTR_e_rotors(Rotorobj *r)
{
	int i;
	for (i = 0; i < r->rotors; i++) {
		RTR_make_id_rotor(r, &(r->e_rotor[(i*r->size)]));
	}
}

/* The current set of decryption rotors */
static void
RTR_d_rotors(Rotorobj *r)
{
	register int i, j;
	for (i = 0; i < r->rotors; i++) {
		for (j = 0; j < r->size; j++) {
			r->d_rotor[((i*r->size)+j)] = (unsigned char)j;
		}
	}
}

/* The positions of the rotors at this time */
static void
RTR_positions(Rotorobj *r)
{
	int i;
	for (i = 0; i < r->rotors; i++) {
		r->positions[i] = 1;
	}
}

/* The number of positions to advance the rotors at a time */
static void
RTR_advances(Rotorobj *r)
{
	int i;
	for (i = 0; i < r->rotors; i++) {
		r->advances[i] = 1;
	}
}

/* Permute the E rotor, and make the D rotor its inverse
 * see Knuth for explanation of algorithm.
 */
static void
RTR_permute_rotor(Rotorobj *r, unsigned char *e, unsigned char *d)
{
	short i = r->size;
	short q;
	unsigned char j;
	RTR_make_id_rotor(r,e);
	while (2 <= i) {
		q = r_rand(r,i);
		i--;
		j = e[q];
		e[q] = (unsigned char)e[i];
		e[i] = (unsigned char)j;
		d[j] = (unsigned char)i;
	}
	e[0] = (unsigned char)e[0];
	d[(e[0])] = (unsigned char)0;
}

/* Given KEY (a list of 5 16 bit numbers), initialize the rotor machine.
 * Set the advancement, position, and permutation of the rotors
 */
static void
RTR_init(Rotorobj *r)
{
	int i;
	set_seed(r);
	RTR_positions(r);
	RTR_advances(r);
	RTR_e_rotors(r);
	RTR_d_rotors(r);
	for (i = 0; i < r->rotors; i++) {
		r->positions[i] = (unsigned char) r_rand(r, (short)r->size);
		r->advances[i] = (1+(2*(r_rand(r, (short)(r->size/2)))));
		RTR_permute_rotor(r,
				  &(r->e_rotor[(i*r->size)]),
				  &(r->d_rotor[(i*r->size)]));
	}
	r->isinited = TRUE;
}

/* Change the RTR-positions vector, using the RTR-advances vector */
static void
RTR_advance(Rotorobj *r)
{
	register int i=0, temp=0;
	if (r->size_mask) {
		while (i < r->rotors) {
			temp = r->positions[i] + r->advances[i];
			r->positions[i] = temp & r->size_mask;
			if ((temp >= r->size) && (i < (r->rotors - 1))) {
				r->positions[(i+1)] = 1 + r->positions[(i+1)];
			}
			i++;
		}
	} else {
		while (i < r->rotors) {
			temp = r->positions[i] + r->advances[i];
			r->positions[i] = temp%r->size;
			if ((temp >= r->size) && (i < (r->rotors - 1))) {
				r->positions[(i+1)] = 1 + r->positions[(i+1)];
			}
			i++;
		}
	}
}

/* Encrypt the character P with the current rotor machine */
static unsigned char
RTR_e_char(Rotorobj *r, unsigned char p)
{
	register int i=0;
	register unsigned char tp=p;
	if (r->size_mask) {
		while (i < r->rotors) {
			tp = r->e_rotor[(i*r->size) +
				       (((r->positions[i] ^ tp) &
					 r->size_mask))];
			i++;
		}
	} else {
		while (i < r->rotors) {
			tp = r->e_rotor[(i*r->size) +
				       (((r->positions[i] ^ tp) %
					 (unsigned int) r->size))];
			i++;
		}
	}
	RTR_advance(r);
	return ((unsigned char)tp);
}

/* Decrypt the character C with the current rotor machine */
static unsigned char
RTR_d_char(Rotorobj *r, unsigned char c)
{
	register int i = r->rotors - 1;
	register unsigned char tc = c;

	if (r->size_mask) {
		while (0 <= i) {
			tc = (r->positions[i] ^
			      r->d_rotor[(i*r->size)+tc]) & r->size_mask;
			i--;
		}
	} else {
		while (0 <= i) {
			tc = (r->positions[i] ^
			      r->d_rotor[(i*r->size)+tc]) %
				(unsigned int) r->size;
			i--;
		}
	}
	RTR_advance(r);
	return(tc);
}

/* Perform a rotor encryption of the region from BEG to END by KEY */
static void
RTR_e_region(Rotorobj *r, unsigned char *beg, int len, int doinit)
{
	register int i;
	if (doinit || r->isinited == FALSE)
		RTR_init(r);
	for (i = 0; i < len; i++) {
		beg[i] = RTR_e_char(r, beg[i]);
	}
}

/* Perform a rotor decryption of the region from BEG to END by KEY */
static void
RTR_d_region(Rotorobj *r, unsigned char *beg, int len, int doinit)
{
	register int i;
	if (doinit || r->isinited == FALSE)
		RTR_init(r);
	for (i = 0; i < len; i++) {
		beg[i] = RTR_d_char(r, beg[i]);
	}
}



/* Rotor methods */
static void
rotor_dealloc(Rotorobj *xp)
{
	if (xp->e_rotor)
		free(xp->e_rotor);
	if (xp->d_rotor)
		free(xp->d_rotor);
	if (xp->positions)
		free(xp->positions);
	if (xp->advances)
		free(xp->advances);
    free(xp);
//	unsigned char_Del(xp);
}
