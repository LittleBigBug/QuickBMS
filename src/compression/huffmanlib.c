/*
 *  huffman - Encode/Decode files using Huffman encoding.
 *  http://huffman.sourceforge.net
 *  Copyright (C) 2003  Douglas Ryan Richardson
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
//#include "huffman.h"

//#ifdef WIN32
//#include <winsock2.h>
//#include <malloc.h>
//#define alloca _alloca
//#else
//#include <netinet/in.h>
//#endif
unsigned myhtonl(unsigned);
unsigned myntohl(unsigned);

typedef struct huffman_node_tag
{
	unsigned char isLeaf;
	unsigned long count;
	struct huffman_node_tag *parent;

	union
	{
		struct
		{
			struct huffman_node_tag *zero, *one;
		};
		unsigned char symbol;
	};
} huffman_node;

typedef struct huffman_code_tag
{
	/* The length of this code in bits. */
	unsigned long numbits;

	/* The bits that make up this code. The first
	   bit is at position 0 in bits[0]. The second
	   bit is at position 1 in bits[0]. The eighth
	   bit is at position 7 in bits[0]. The ninth
	   bit is at position 0 in bits[1]. */
	unsigned char *bits;
} huffman_code;

static unsigned long
numbytes_from_numbits(unsigned long numbits)
{
	return numbits / 8 + (numbits % 8 ? 1 : 0);
}

/*
 * get_bit returns the ith bit in the bits array
 * in the 0th position of the return value.
 */
static unsigned char
get_bit(unsigned char* bits, unsigned long i)
{
	return (bits[i / 8] >> i % 8) & 1;
}

static void
reverse_bits(unsigned char* bits, unsigned long numbits)
{
	unsigned long numbytes = numbytes_from_numbits(numbits);
	unsigned char tmp[numbytes];
	unsigned long curbit;
	long curbyte = 0;
	
	memset(tmp, 0, numbytes);

	for(curbit = 0; curbit < numbits; ++curbit)
	{
		unsigned int bitpos = curbit % 8;

		if(curbit > 0 && curbit % 8 == 0)
			++curbyte;
		
		tmp[curbyte] |= (get_bit(bits, numbits - curbit - 1) << bitpos);
	}

	memcpy(bits, tmp, numbytes);
}

/*
 * new_code builds a huffman_code from a leaf in
 * a Huffman tree.
 */
static huffman_code*
new_code(const huffman_node* leaf)
{
	/* Build the huffman code by walking up to
	 * the root node and then reversing the bits,
	 * since the Huffman code is calculated by
	 * walking down the tree. */
	unsigned long numbits = 0;
	unsigned char* bits = NULL;
	huffman_code *p;

	while(leaf && leaf->parent)
	{
		huffman_node *parent = leaf->parent;
		unsigned char cur_bit = (unsigned char)(numbits % 8);
		unsigned long cur_byte = numbits / 8;

		/* If we need another byte to hold the code,
		   then allocate it. */
		if(cur_bit == 0)
		{
			size_t newSize = cur_byte + 1;
			bits = (unsigned char*)realloc(bits, newSize);
			bits[newSize - 1] = 0; /* Initialize the new byte. */
		}

		/* If a one must be added then or it in. If a zero
		 * must be added then do nothing, since the byte
		 * was initialized to zero. */
		if(leaf == parent->one)
			bits[cur_byte] |= 1 << cur_bit;

		++numbits;
		leaf = parent;
	}

	if(bits)
		reverse_bits(bits, numbits);

	p = (huffman_code*)malloc(sizeof(huffman_code));
	p->numbits = numbits;
	p->bits = bits;
	return p;
}

#define MAX_SYMBOLS 256
typedef huffman_node* SymbolFrequencies[MAX_SYMBOLS];
typedef huffman_code* SymbolEncoder[MAX_SYMBOLS];

static huffman_node*
new_leaf_node(unsigned char symbol)
{
	huffman_node *p = (huffman_node*)malloc(sizeof(huffman_node));
	p->isLeaf = 1;
	p->symbol = symbol;
	p->count = 0;
	p->parent = 0;
	return p;
}

static huffman_node*
new_nonleaf_node(unsigned long count, huffman_node *zero, huffman_node *one)
{
	huffman_node *p = (huffman_node*)malloc(sizeof(huffman_node));
	p->isLeaf = 0;
	p->count = count;
	p->zero = zero;
	p->one = one;
	p->parent = 0;
	
	return p;
}

static void
free_huffman_tree(huffman_node *subtree)
{
	if(subtree == NULL)
		return;

	if(!subtree->isLeaf)
	{
		free_huffman_tree(subtree->zero);
		free_huffman_tree(subtree->one);
	}
	
	free(subtree);
}

static void
free_code(huffman_code* p)
{
	free(p->bits);
	free(p);
}

static void
free_encoder(SymbolEncoder *pSE)
{
	unsigned long i;
	for(i = 0; i < MAX_SYMBOLS; ++i)
	{
		huffman_code *p = (*pSE)[i];
		if(p)
			free_code(p);
	}

	free(pSE);
}

static void
init_frequencies(SymbolFrequencies *pSF)
{
	memset(*pSF, 0, sizeof(SymbolFrequencies));
#if 0
	unsigned int i;
	for(i = 0; i < MAX_SYMBOLS; ++i)
	{
		unsigned char uc = (unsigned char)i;
		(*pSF)[i] = new_leaf_node(uc);
	}
#endif
}

typedef struct buf_cache_tag
{
	unsigned char *cache;
	unsigned int cache_len;
	unsigned int cache_cur;
	unsigned char **pbufout;
	unsigned int *pbufoutlen;
} buf_cache;

static int init_cache(buf_cache* pc,
					  unsigned int cache_size,
					  unsigned char **pbufout,
					  unsigned int *pbufoutlen)
{
	assert(pc && pbufout && pbufoutlen);
	if(!pbufout || !pbufoutlen)
		return 1;
	
	pc->cache = (unsigned char*)malloc(cache_size);
	pc->cache_len = cache_size;
	pc->cache_cur = 0;
	pc->pbufout = pbufout;
	*pbufout = NULL;
	pc->pbufoutlen = pbufoutlen;
	*pbufoutlen = 0;

	return pc->cache ? 0 : 1;
}

static void free_cache(buf_cache* pc)
{
	assert(pc);
	if(pc->cache)
	{
		free(pc->cache);
		pc->cache = NULL;
	}
}

static int flush_cache(buf_cache* pc)
{
	assert(pc);
	
	if(pc->cache_cur > 0)
	{
		unsigned int newlen = pc->cache_cur + *pc->pbufoutlen;
		unsigned char* tmp = realloc(*pc->pbufout, newlen);
		if(!tmp)
			return 1;

		memcpy(tmp + *pc->pbufoutlen, pc->cache, pc->cache_cur);

		*pc->pbufout = tmp;
		*pc->pbufoutlen = newlen;
		pc->cache_cur = 0;
	}

	return 0;
}

static int write_cache(buf_cache* pc,
					   const void *to_write,
					   unsigned int to_write_len)
{
	unsigned char* tmp;

	assert(pc && to_write);
	assert(pc->cache_len >= pc->cache_cur);
	
	/* If trying to write more than the cache will hold
	 * flush the cache and allocate enough space immediately,
	 * that is, don't use the cache. */
	if(to_write_len > pc->cache_len - pc->cache_cur)
	{
		unsigned int newlen;
		flush_cache(pc);
		newlen = *pc->pbufoutlen + to_write_len;
		tmp = realloc(*pc->pbufout, newlen);
		if(!tmp)
			return 1;
		memcpy(tmp + *pc->pbufoutlen, to_write, to_write_len);
		*pc->pbufout = tmp;
		*pc->pbufoutlen = newlen;
	}
	else
	{
		/* Write the data to the cache. */
		memcpy(pc->cache + pc->cache_cur, to_write, to_write_len);
		pc->cache_cur += to_write_len;
	}

	return 0;
}

static unsigned int
get_symbol_frequencies(SymbolFrequencies *pSF, FILE *in)
{
	int c;
	unsigned int total_count = 0;
	
	/* Set all frequencies to 0. */
	init_frequencies(pSF);
	
	/* Count the frequency of each symbol in the input file. */
	while((c = fgetc(in)) != EOF)
	{
		unsigned char uc = c;
		if(!(*pSF)[uc])
			(*pSF)[uc] = new_leaf_node(uc);
		++(*pSF)[uc]->count;
		++total_count;
	}

	return total_count;
}

static unsigned int
get_symbol_frequencies_from_memory(SymbolFrequencies *pSF,
								   const unsigned char *bufin,
								   unsigned int bufinlen)
{
	unsigned int i;
	unsigned int total_count = 0;
	
	/* Set all frequencies to 0. */
	init_frequencies(pSF);
	
	/* Count the frequency of each symbol in the input file. */
	for(i = 0; i < bufinlen; ++i)
	{
		unsigned char uc = bufin[i];
		if(!(*pSF)[uc])
			(*pSF)[uc] = new_leaf_node(uc);
		++(*pSF)[uc]->count;
		++total_count;
	}

	return total_count;
}

/*
 * When used by qsort, SFComp sorts the array so that
 * the symbol with the lowest frequency is first. Any
 * NULL entries will be sorted to the end of the list.
 */
static int
SFComp(const void *p1, const void *p2)
{
	const huffman_node *hn1 = *(const huffman_node**)p1;
	const huffman_node *hn2 = *(const huffman_node**)p2;

	/* Sort all NULLs to the end. */
	if(hn1 == NULL && hn2 == NULL)
		return 0;
	if(hn1 == NULL)
		return 1;
	if(hn2 == NULL)
		return -1;
	
	if(hn1->count > hn2->count)
		return 1;
	else if(hn1->count < hn2->count)
		return -1;

	return 0;
}

#if 0
static void
print_freqs(SymbolFrequencies * pSF)
{
	size_t i;
	for(i = 0; i < MAX_SYMBOLS; ++i)
	{
		if((*pSF)[i])
			printf("%d, %ld\n", (*pSF)[i]->symbol, (*pSF)[i]->count);
		else
			printf("NULL\n");
	}
}
#endif

/*
 * build_symbol_encoder builds a SymbolEncoder by walking
 * down to the leaves of the Huffman tree and then,
 * for each leaf, determines its code.
 */
static void
build_symbol_encoder(huffman_node *subtree, SymbolEncoder *pSF)
{
	if(subtree == NULL)
		return;

	if(subtree->isLeaf)
		(*pSF)[subtree->symbol] = new_code(subtree);
	else
	{
		build_symbol_encoder(subtree->zero, pSF);
		build_symbol_encoder(subtree->one, pSF);
	}
}

/*
 * calculate_huffman_codes turns pSF into an array
 * with a single entry that is the root of the
 * huffman tree. The return value is a SymbolEncoder,
 * which is an array of huffman codes index by symbol value.
 */
static SymbolEncoder*
calculate_huffman_codes(SymbolFrequencies * pSF)
{
	unsigned int i = 0;
	unsigned int n = 0;
	huffman_node *m1 = NULL, *m2 = NULL;
	SymbolEncoder *pSE = NULL;
	
#if 0
	printf("BEFORE SORT\n");
	print_freqs(pSF);
#endif

	/* Sort the symbol frequency array by ascending frequency. */
	qsort((*pSF), MAX_SYMBOLS, sizeof((*pSF)[0]), SFComp);

#if 0	
	printf("AFTER SORT\n");
	print_freqs(pSF);
#endif

	/* Get the number of symbols. */
	for(n = 0; n < MAX_SYMBOLS && (*pSF)[n]; ++n)
		;

	/*
	 * Construct a Huffman tree. This code is based
	 * on the algorithm given in Managing Gigabytes
	 * by Ian Witten et al, 2nd edition, page 34.
	 * Note that this implementation uses a simple
	 * count instead of probability.
	 */
	for(i = 0; i < n - 1; ++i)
	{
		/* Set m1 and m2 to the two subsets of least probability. */
		m1 = (*pSF)[0];
		m2 = (*pSF)[1];

		/* Replace m1 and m2 with a set {m1, m2} whose probability
		 * is the sum of that of m1 and m2. */
		(*pSF)[0] = m1->parent = m2->parent =
			new_nonleaf_node(m1->count + m2->count, m1, m2);
		(*pSF)[1] = NULL;
		
		/* Put newSet into the correct count position in pSF. */
		qsort((*pSF), n, sizeof((*pSF)[0]), SFComp);
	}

	/* Build the SymbolEncoder array from the tree. */
	pSE = (SymbolEncoder*)malloc(sizeof(SymbolEncoder));
	memset(pSE, 0, sizeof(SymbolEncoder));
	build_symbol_encoder((*pSF)[0], pSE);
	return pSE;
}

/*
 * Write the huffman code table. The format is:
 * 4 byte code count in network byte order.
 * 4 byte number of bytes encoded
 *   (if you decode the data, you should get this number of bytes)
 * code1
 * ...
 * codeN, where N is the count read at the begginning of the file.
 * Each codeI has the following format:
 * 1 byte symbol, 1 byte code bit length, code bytes.
 * Each entry has numbytes_from_numbits code bytes.
 * The last byte of each code may have extra bits, if the number of
 * bits in the code is not a multiple of 8.
 */
static int
write_code_table(FILE* out, SymbolEncoder *se, unsigned int symbol_count)
{
	unsigned long i, count = 0;
	
	/* Determine the number of entries in se. */
	for(i = 0; i < MAX_SYMBOLS; ++i)
	{
		if((*se)[i])
			++count;
	}

	/* Write the number of entries in network byte order. */
	i = myhtonl(count);
	if(fwrite(&i, sizeof(i), 1, out) != 1)
		return 1;

	/* Write the number of bytes that will be encoded. */
	symbol_count = myhtonl(symbol_count);
	if(fwrite(&symbol_count, sizeof(symbol_count), 1, out) != 1)
		return 1;

	/* Write the entries. */
	for(i = 0; i < MAX_SYMBOLS; ++i)
	{
		huffman_code *p = (*se)[i];
		if(p)
		{
			unsigned int numbytes;
			/* Write the 1 byte symbol. */
			fputc((unsigned char)i, out);
			/* Write the 1 byte code bit length. */
			fputc(p->numbits, out);
			/* Write the code bytes. */
			numbytes = numbytes_from_numbits(p->numbits);
			if(fwrite(p->bits, 1, numbytes, out) != numbytes)
				return 1;
		}
	}

	return 0;
}

/*
 * Allocates memory and sets *pbufout to point to it. The memory
 * contains the code table.
 */
static int
write_code_table_to_memory(buf_cache *pc,
						   SymbolEncoder *se,
						   unsigned int symbol_count)
{
	unsigned long i, count = 0;

	/* Determine the number of entries in se. */
	for(i = 0; i < MAX_SYMBOLS; ++i)
	{
		if((*se)[i])
			++count;
	}

	/* Write the number of entries in network byte order. */
	i = myhtonl(count);
	
	if(write_cache(pc, &i, sizeof(i)))
		return 1;

	/* Write the number of bytes that will be encoded. */
	symbol_count = myhtonl(symbol_count);
	if(write_cache(pc, &symbol_count, sizeof(symbol_count)))
		return 1;

	/* Write the entries. */
	for(i = 0; i < MAX_SYMBOLS; ++i)
	{
		huffman_code *p = (*se)[i];
		if(p)
		{
			unsigned int numbytes;
			/* The value of i is < MAX_SYMBOLS (256), so it can
			be stored in an unsigned char. */
			unsigned char uc = (unsigned char)i;
			/* Write the 1 byte symbol. */
			if(write_cache(pc, &uc, sizeof(uc)))
				return 1;
			/* Write the 1 byte code bit length. */
			uc = (unsigned char)p->numbits;
			if(write_cache(pc, &uc, sizeof(uc)))
				return 1;
			/* Write the code bytes. */
			numbytes = numbytes_from_numbits(p->numbits);
			if(write_cache(pc, p->bits, numbytes))
				return 1;
		}
	}

	return 0;
}

/*
 * read_code_table builds a Huffman tree from the code
 * in the in file. This function returns NULL on error.
 * The returned value should be freed with free_huffman_tree.
 */
static huffman_node*
read_code_table(FILE* in, unsigned int *pDataBytes)
{
	huffman_node *root = new_nonleaf_node(0, NULL, NULL);
	unsigned int count;
	
	/* Read the number of entries.
	   (it is stored in network byte order). */
	if(fread(&count, sizeof(count), 1, in) != 1)
	{
		free_huffman_tree(root);
		return NULL;
	}

	count = myntohl(count);

	/* Read the number of data bytes this encoding represents. */
	if(fread(pDataBytes, sizeof(*pDataBytes), 1, in) != 1)
	{
		free_huffman_tree(root);
		return NULL;
	}

	*pDataBytes = myntohl(*pDataBytes);


	/* Read the entries. */
	while(count-- > 0)
	{
		int c;
		unsigned int curbit;
		unsigned char symbol;
		unsigned char numbits;
		unsigned char numbytes;
		unsigned char *bytes;
		huffman_node *p = root;
		
		if((c = fgetc(in)) == EOF)
		{
			free_huffman_tree(root);
			return NULL;
		}
		symbol = (unsigned char)c;
		
		if((c = fgetc(in)) == EOF)
		{
			free_huffman_tree(root);
			return NULL;
		}
		
		numbits = (unsigned char)c;
		numbytes = (unsigned char)numbytes_from_numbits(numbits);
		bytes = (unsigned char*)malloc(numbytes);
		if(fread(bytes, 1, numbytes, in) != numbytes)
		{
			free(bytes);
			free_huffman_tree(root);
			return NULL;
		}

		/*
		 * Add the entry to the Huffman tree. The value
		 * of the current bit is used switch between
		 * zero and one child nodes in the tree. New nodes
		 * are added as needed in the tree.
		 */
		for(curbit = 0; curbit < numbits; ++curbit)
		{
			if(get_bit(bytes, curbit))
			{
				if(p->one == NULL)
				{
					p->one = curbit == (unsigned char)(numbits - 1)
						? new_leaf_node(symbol)
						: new_nonleaf_node(0, NULL, NULL);
					p->one->parent = p;
				}
				p = p->one;
			}
			else
			{
				if(p->zero == NULL)
				{
					p->zero = curbit == (unsigned char)(numbits - 1)
						? new_leaf_node(symbol)
						: new_nonleaf_node(0, NULL, NULL);
					p->zero->parent = p;
				}
				p = p->zero;
			}
		}
		
		free(bytes);
	}

	return root;
}

static int
memread(const unsigned char* buf,
		unsigned int buflen,
		unsigned int *pindex,
		void* bufout,
		unsigned int readlen)
{
	assert(buf && pindex && bufout);
	assert(buflen >= *pindex);
	if(buflen < *pindex)
		return 1;
	if(readlen + *pindex >= buflen)
		return 1;
	memcpy(bufout, buf + *pindex, readlen);
	*pindex += readlen;
	return 0;
}

static huffman_node*
read_code_table_from_memory(const unsigned char* bufin,
							unsigned int bufinlen,
							unsigned int *pindex,
							unsigned int *pDataBytes)
{
	huffman_node *root = new_nonleaf_node(0, NULL, NULL);
	unsigned int count;
	
	/* Read the number of entries.
	   (it is stored in network byte order). */
	if(memread(bufin, bufinlen, pindex, &count, sizeof(count)))
	{
		free_huffman_tree(root);
		return NULL;
	}

	count = myntohl(count);

	/* Read the number of data bytes this encoding represents. */
	if(memread(bufin, bufinlen, pindex, pDataBytes, sizeof(*pDataBytes)))
	{
		free_huffman_tree(root);
		return NULL;
	}

	*pDataBytes = myntohl(*pDataBytes);

	/* Read the entries. */
	while(count-- > 0)
	{
		unsigned int curbit;
		unsigned char symbol;
		unsigned char numbits;
		unsigned char numbytes;
		unsigned char *bytes;
		huffman_node *p = root;

		if(memread(bufin, bufinlen, pindex, &symbol, sizeof(symbol)))
		{
			free_huffman_tree(root);
			return NULL;
		}

		if(memread(bufin, bufinlen, pindex, &numbits, sizeof(numbits)))
		{
			free_huffman_tree(root);
			return NULL;
		}
		
		numbytes = (unsigned char)numbytes_from_numbits(numbits);
		bytes = (unsigned char*)malloc(numbytes);
		if(memread(bufin, bufinlen, pindex, bytes, numbytes))
		{
			free(bytes);
			free_huffman_tree(root);
			return NULL;
		}

		/*
		 * Add the entry to the Huffman tree. The value
		 * of the current bit is used switch between
		 * zero and one child nodes in the tree. New nodes
		 * are added as needed in the tree.
		 */
		for(curbit = 0; curbit < numbits; ++curbit)
		{
			if(get_bit(bytes, curbit))
			{
				if(p->one == NULL)
				{
					p->one = curbit == (unsigned char)(numbits - 1)
						? new_leaf_node(symbol)
						: new_nonleaf_node(0, NULL, NULL);
					p->one->parent = p;
				}
				p = p->one;
			}
			else
			{
				if(p->zero == NULL)
				{
					p->zero = curbit == (unsigned char)(numbits - 1)
						? new_leaf_node(symbol)
						: new_nonleaf_node(0, NULL, NULL);
					p->zero->parent = p;
				}
				p = p->zero;
			}
		}
		
		free(bytes);
	}

	return root;
}

static int
do_file_encode(FILE* in, FILE* out, SymbolEncoder *se)
{
	unsigned char curbyte = 0;
	unsigned char curbit = 0;
	int c;
	
	while((c = fgetc(in)) != EOF)
	{
		unsigned char uc = (unsigned char)c;
		huffman_code *code = (*se)[uc];
		unsigned long i;
		
		for(i = 0; i < code->numbits; ++i)
		{
			/* Add the current bit to curbyte. */
			curbyte |= get_bit(code->bits, i) << curbit;

			/* If this byte is filled up then write it
			 * out and reset the curbit and curbyte. */
			if(++curbit == 8)
			{
				fputc(curbyte, out);
				curbyte = 0;
				curbit = 0;
			}
		}
	}

	/*
	 * If there is data in curbyte that has not been
	 * output yet, which means that the last encoded
	 * character did not fall on a byte boundary,
	 * then output it.
	 */
	if(curbit > 0)
		fputc(curbyte, out);

	return 0;
}

static int
do_memory_encode(buf_cache *pc,
				 const unsigned char* bufin,
				 unsigned int bufinlen,
				 SymbolEncoder *se)
{
	unsigned char curbyte = 0;
	unsigned char curbit = 0;
	unsigned int i;
	
	for(i = 0; i < bufinlen; ++i)
	{
		unsigned char uc = bufin[i];
		huffman_code *code = (*se)[uc];
		unsigned long i;
		
		for(i = 0; i < code->numbits; ++i)
		{
			/* Add the current bit to curbyte. */
			curbyte |= get_bit(code->bits, i) << curbit;

			/* If this byte is filled up then write it
			 * out and reset the curbit and curbyte. */
			if(++curbit == 8)
			{
				if(write_cache(pc, &curbyte, sizeof(curbyte)))
					return 1;
				curbyte = 0;
				curbit = 0;
			}
		}
	}

	/*
	 * If there is data in curbyte that has not been
	 * output yet, which means that the last encoded
	 * character did not fall on a byte boundary,
	 * then output it.
	 */
	return curbit > 0 ? write_cache(pc, &curbyte, sizeof(curbyte)) : 0;
}

/*
 * huffman_encode_file huffman encodes in to out.
 */
int
huffman_encode_file(FILE *in, FILE *out)
{
	SymbolFrequencies sf;
	SymbolEncoder *se;
	huffman_node *root = NULL;
	int rc;
	unsigned int symbol_count;

	/* Get the frequency of each symbol in the input file. */
	symbol_count = get_symbol_frequencies(&sf, in);

	/* Build an optimal table from the symbolCount. */
	se = calculate_huffman_codes(&sf);
	root = sf[0];

	/* Scan the file again and, using the table
	   previously built, encode it into the output file. */
	rewind(in);
	rc = write_code_table(out, se, symbol_count);
	if(rc == 0)
		rc = do_file_encode(in, out, se);

	/* Free the Huffman tree. */
	free_huffman_tree(root);
	free_encoder(se);
	return rc;
}

int
huffman_decode_file(FILE *in, FILE *out)
{
	huffman_node *root, *p;
	int c;
	unsigned int data_count;
	
	/* Read the Huffman code table. */
	root = read_code_table(in, &data_count);
	if(!root)
		return 1;

	/* Decode the file. */
	p = root;
	while(data_count > 0 && (c = fgetc(in)) != EOF)
	{
		unsigned char byte = (unsigned char)c;
		unsigned char mask = 1;
		while(data_count > 0 && mask)
		{
			p = byte & mask ? p->one : p->zero;
			mask <<= 1;

			if(p->isLeaf)
			{
				fputc(p->symbol, out);
				p = root;
				--data_count;
			}
		}
	}

	free_huffman_tree(root);
	return 0;
}

#define CACHE_SIZE 1024

int huffman_encode_memory(const unsigned char *bufin,
						  unsigned int bufinlen,
						  unsigned char **pbufout,
						  unsigned int *pbufoutlen)
{
	SymbolFrequencies sf;
	SymbolEncoder *se;
	huffman_node *root = NULL;
	int rc;
	unsigned int symbol_count;
	buf_cache cache;

	/* Ensure the arguments are valid. */
	if(!pbufout || !pbufoutlen)
		return 1;

	if(init_cache(&cache, CACHE_SIZE, pbufout, pbufoutlen))
		return 1;

	/* Get the frequency of each symbol in the input memory. */
	symbol_count = get_symbol_frequencies_from_memory(&sf, bufin, bufinlen);

	/* Build an optimal table from the symbolCount. */
	se = calculate_huffman_codes(&sf);
	root = sf[0];

	/* Scan the memory again and, using the table
	   previously built, encode it into the output memory. */
	rc = write_code_table_to_memory(&cache, se, symbol_count);
	if(rc == 0)
		rc = do_memory_encode(&cache, bufin, bufinlen, se);

	/* Flush the cache. */
	flush_cache(&cache);
	
	/* Free the Huffman tree. */
	free_huffman_tree(root);
	free_encoder(se);
	free_cache(&cache);
	return rc;
}

int huffman_decode_memory(const unsigned char *bufin,
						  unsigned int bufinlen,
						  unsigned char **pbufout,
						  unsigned int *pbufoutlen)
{
	huffman_node *root, *p;
	unsigned int data_count;
	unsigned int i = 0;
	unsigned char *buf;
	unsigned int bufcur = 0;

	/* Ensure the arguments are valid. */
	if(!pbufout || !pbufoutlen)
		return 1;

	/* Read the Huffman code table. */
	root = read_code_table_from_memory(bufin, bufinlen, &i, &data_count);
	if(!root)
		return 1;

	buf = (unsigned char*)malloc(data_count);

	/* Decode the memory. */
	p = root;
	for(; i < bufinlen && data_count > 0; ++i) 
	{
		unsigned char byte = bufin[i];
		unsigned char mask = 1;
		while(data_count > 0 && mask)
		{
			p = byte & mask ? p->one : p->zero;
			mask <<= 1;

			if(p->isLeaf)
			{
				buf[bufcur++] = p->symbol;
				p = root;
				--data_count;
			}
		}
	}

	free_huffman_tree(root);
	*pbufout = buf;
	*pbufoutlen = bufcur;
	return 0;
}
