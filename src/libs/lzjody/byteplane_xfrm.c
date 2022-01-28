/*
 * Byte plane transformation
 *
 * Copyright (C) 2014-2020 by Jody Bruchon <jody@jodybruchon.com>
 * Released under The MIT License
 *
 * This code performs an n-plane transformation on a block of data.
 * For example, a 4-plane transform on "1200120112021023" would change
 * that string into "1111222200000123", a string which is easily
 * compressible, unlike the original. The resulting string has three
 * RLE runs and one incremental sequence.
 * Passing a negative num_planes reverses the transformation.
 */

extern int byteplane_transform(const unsigned char * const in,
		unsigned char * const out, int length,
		int num_planes)
{
	int i;
	int plane = 0;
	int opos = 0;

	if (num_planes > 1) {
		/* Split 'in' to byteplanes, placing result in 'out' */
		while (plane < num_planes) {
			i = plane;
			while (i < length) {
				*(out + opos) = *(in + i);
				opos++;
				i += num_planes;
			}
			plane++;
		}
	} else if (num_planes > -1) return -1;
	else {
		num_planes = -num_planes;
		while (plane < num_planes) {
			i = plane;
			while (i < length) {
				*(out + i) = *(in + opos);
				opos++;
				i += num_planes;
			}
			plane++;
		}

	}
	if (opos != length) return -1;
	return 0;

}
