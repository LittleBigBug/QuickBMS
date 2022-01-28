// modified by Luigi Auriemma
// irolz.cpp : Defines the entry point for the console application.
/// (C) 2010, Andrew Polar under GPL ver. 3.
//   LICENSE
//
//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 3 of
//   the License, or (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful, but
//   WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//   General Public License for more details at
//   Visit <http://www.gnu.org/copyleft/gpl.html>.
//
// This is complete DEMO for iROLZ algorithm. 
// Last modification 01.25.2013
//
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <math.h>
#include <string.h>
#include "../extra/mem2mem.h"

//Can be changed if understood
#define PREFIX_SIZE            2   //must be 1, 2 or 3
#define HISTORY_BUFFER_BITLEN  15  //trade between time and compression ratio
#define SUFFICIENT_MATCH       64  //does not make much difference
#define MINIMUM_MATCH          4   //must be 1 or larger
#define LONGEST_COUNT          255 //must be one byte value
#define BIT_LEN_FOR_STEPS      5   //these are offset indexes, typically 5 bits is enough, limited to 8 bits

static int TERMINATION_FLAG  = (1<<BIT_LEN_FOR_STEPS) - 1;     
static int MAX_STEPS         = TERMINATION_FLAG - 1; 
static int CHUNK_SIZE        = 0xffffff + 1;  //this is size of the processed chunk, the file can be larger    

static FILE* inCmp  = 0;
static FILE* outCmp = 0;

namespace irolz {

int getFileSize(char* fileName) {
	FILE* f;
	if ((f = fopen(fileName, "rb")) == NULL)
		return -1;
	fseek(f, 0, SEEK_END);
	int bytes = ftell(f);
	fclose(f);
	return bytes;
}

//Input/Output functions
inline void writeByte(unsigned char byte) {
	fputc(byte, outCmp);
}

inline unsigned char readByte() {
	return fgetc(inCmp);
}
//end

//This is my portable experimental Dictionary class.
//It can be taken and used with outher programs.
//The encoder and decoder can have multiple dictionaries and they
//may not necessarily contain past data, but be freely populated.
class Dictionary {
public:
	Dictionary(int PrefixSize, int OffsetLen);
	~Dictionary();
	void updateDictionary(unsigned char byte);
	int  getNextPosition(int position);
	void eraseData();
private:
	int* m_last_position_lookup;
	int* m_self_addressed_dictionary;
	int m_prefix_mask, m_buffer_mask, m_context, m_index;
};

Dictionary::Dictionary(int PrefixSize, int OffsetLen) {
	if (PrefixSize == 1) m_prefix_mask = 0xff;
	else if (PrefixSize == 2) m_prefix_mask = 0xffff;
	else m_prefix_mask = 0xffffff;
	m_buffer_mask = (1<<OffsetLen) - 1;
	m_last_position_lookup = (int*)malloc((m_prefix_mask + 1) * sizeof(int));
	m_self_addressed_dictionary = (int*)malloc((m_buffer_mask + 1) * sizeof(int));
	eraseData();
}

Dictionary::~Dictionary() {
	if (m_self_addressed_dictionary) free(m_self_addressed_dictionary);
	if (m_last_position_lookup)      free(m_last_position_lookup);
}

void Dictionary::updateDictionary(unsigned char byte) {
	m_context <<= 8;
	m_context |= byte;
	m_context &= m_prefix_mask;
	m_self_addressed_dictionary[m_index & m_buffer_mask] = m_last_position_lookup[m_context];
	m_last_position_lookup[m_context] = m_index;
	++m_index;
}

int Dictionary::getNextPosition(int position) {
	return m_self_addressed_dictionary[position & m_buffer_mask];
}

void Dictionary::eraseData() {
	memset(m_last_position_lookup,      0x00, (m_prefix_mask + 1) * sizeof(int));
	memset(m_self_addressed_dictionary, 0x00, (m_buffer_mask + 1) * sizeof(int));
	m_context = m_index = 0;
}

//Several following functions and classes are taken from open source
//http://balz.sourceforge.net/ and written by Ilia Muraviev. I found
//it very convenient. I added only some cosmetic changes.
static void exetransform(unsigned char* buf, int y, int n) {
	const int end=n-8;
	int i=0;
	// search for pe file header
	while ((reinterpret_cast<int&>(buf[i])!=0x4550)&&(++i<end));

	// perform call/jmp address translation
	while (i<end) {
		if ((buf[i++]&254)==0xe8) {
			int &addr=reinterpret_cast<int&>(buf[i]);
			if (y) {
				if ((addr>=-i)&&(addr<(n-i))) addr+=i;
				else if ((addr>0)&&(addr<n)) addr-=n;
			}
			else {
				if (addr<0) {
					if ((addr+i)>=0) addr+=n;
				}
				else if (addr<n) addr-=i;
			}
			i+=4;
		}
	}
}

//source: http://balz.sourceforge.net/
class TPredictor {
private:
	unsigned short p1, p2;
public:
	TPredictor(): p1(1 << 15), p2(1 << 15) {} 
	~TPredictor() {}
	int P()	{
		return (p1 + p2); 
	}
	void Update(int bit) { 
		if (bit) {
			p1 += (unsigned short)(~p1) >> 3; 
			p2 += (unsigned short)(~p2) >> 6; 
		}
		else {
			p1 -= p1 >> 3; 
			p2 -= p2 >> 6; 
		}
	}
};

//This coder Ilia Muraviev derived from Mahoney's fpaq0
//source: http://balz.sourceforge.net/
class TEncoder {
private:
	unsigned int x1, x2;
public:
	TEncoder(): x1(0), x2(-1) {} 

	void Encode(int P, int bit) { 
		const unsigned int xmid = x1 + (unsigned int)(((long long)(x2 - x1) * (long long)(P)) >> 17);
		if (bit) x2 = xmid;
		else x1 = xmid + 1;

		while ((x1 ^ x2) < (1 << 24)) {
			writeByte((unsigned char)(x2 >> 24));
			x1 <<= 8;
			x2 = (x2 << 8) + 255;
		}
	}

	void Flush() { 
		for (int i=0; i<4; i++) {
			writeByte((unsigned char)(x2 >> 24));
			x2 <<= 8;
		}
	}
};

//source: http://balz.sourceforge.net/
class TDecoder {
private:
	unsigned int x1, x2, x;
public:
	TDecoder(): x1(0), x2(-1) {} 

	void Init() { 
		for (int i=0; i<4; i++) {
			x = (x << 8) + readByte();
		}
	}

	int Decode(int P) {    
		const unsigned int xmid = x1 + (unsigned int)(((long long)(x2 - x1) * (long long)(P)) >> 17);
		int bit = (x <= xmid);
		if (bit) x2 = xmid;
		else x1 = xmid + 1;

		while ((x1 ^ x2) < (1 << 24)) { 
			x1 <<= 8;
			x2 = (x2 << 8) + 255;
			x = (x << 8) + readByte();
		}
		return (bit);
	}
};

//source: http://balz.sourceforge.net/
static class TPPM {
private:
	TPredictor pliteral[256][256];
	TPredictor plength [256][256];
	TPredictor pbit[2];
	TPredictor pstep;
public:
	TPPM() {
	}
	~TPPM() {
	}

	TEncoder encoder;
	TDecoder decoder;

	void EncodeBit(unsigned char bit, unsigned char context) {
		encoder.Encode(pbit[context].P(), bit);
		pbit[context].Update(bit);
	}

	void EncodeLiteral(unsigned char value, unsigned char context) { 
		for (int i=7, j=1; i>=0; i--) {   //i must be 8 when 512
			int bit = (value >> i) & 1;
			int P1B = pliteral[context][j].P();
			int P = P1B;
			encoder.Encode(P, bit);
			pliteral[context][j].Update(bit);
			j += j + bit;
		}
	}

	void EncodeLength(unsigned char value, unsigned char context) {
		for (int i=7, j=1; i>=0; i--) {   
			int bit = (value >> i) & 1;
			encoder.Encode(plength[context][j].P(), bit);
			plength[context][j].Update(bit);
			j += j + bit;
		}
	}

	void EncodeStep(unsigned char value) { 
		for (int i=BIT_LEN_FOR_STEPS-1, j=1; i>=0; i--) {  
			int bit = (value >> i) & 1;
			encoder.Encode(pstep.P(), bit);
			pstep.Update(bit);
			j += j + bit;
		}
	}

	int DecodeBit(unsigned char context) {
		int bit = decoder.Decode(pbit[context].P());
		pbit[context].Update(bit);
		return bit;
	}

	int DecodeLiteral(unsigned char context) { 
		int value = 1;
		do {
			int P1B = pliteral[context][value].P();
			int P = P1B;
			int bit = decoder.Decode(P);
			pliteral[context][value].Update(bit);
			value += value + bit;
		} while (value < 256); 
		return (value - 256);
	}

	int DecodeLength(unsigned char context) { 
		int value = 1;
		do {
			int bit = decoder.Decode(plength[context][value].P());
			plength[context][value].Update(bit);
			value += value + bit;
		} while (value < 256); 
		return (value - 256);
	}

	int DecodeStep() { 
		int value = 1;
		do {
			int bit = decoder.Decode(pstep.P());
			pstep.Update(bit);
			value += value + bit;
		} while (value < (1<<BIT_LEN_FOR_STEPS)); 
		return (value - (1<<BIT_LEN_FOR_STEPS));
	}
} *ppm = NULL;
//end of Ilia Muraviev binary entropy coder

inline int getLongestMatch(Dictionary* dictionary, const unsigned char* data, int index, int data_size, int& nsteps) {
	int max_count, max_step, step, position, count;
	position = index;
	max_count = max_step = step = 0;
	while (true) {
		int new_position = dictionary->getNextPosition(position);
		if (new_position >= position) break; 
		count = 0;
		while (true) {
			if (index + 1 + count >= data_size) break;
			if (data[index + 1 + count] == data[new_position + 1 + count]) {
				++count;
			}
			else break;
			if (count >= LONGEST_COUNT) break;
		}
		if (count > max_count) {
			max_count = count;
			max_step  = step;
		}
		if (max_count > SUFFICIENT_MATCH) break;
		if (step++ >= MAX_STEPS) break;
		position = new_position;
	} 
	nsteps = max_step;
	return max_count;
}

bool process_compress(Dictionary* dictionary, const unsigned char* data, const int data_size, bool isLast) {
	int max_count, max_step;
	int index = 0;
	//
	int prev_flag  = 0;
	int prev_count = 0;
	unsigned char context = 0;
	do {
		if (data_size == 0) break;
		dictionary->updateDictionary(data[index]);
		//
		ppm->EncodeBit(0, prev_flag);
		prev_flag = 0;
		//
		ppm->EncodeLiteral(data[index], context);
		context = data[index];

		while (true) {
			max_count = getLongestMatch(dictionary, data, index, data_size, max_step);
			if (max_count >= MINIMUM_MATCH) {
				ppm->EncodeBit(1, prev_flag);
				prev_flag = 1;
				ppm->EncodeStep(max_step);
				ppm->EncodeLength(max_count, prev_count);
				prev_count = max_count;
				//This fragment populates dictionay with values that sitting in 
				//<offset, length> couple. 
				for (int k=0; k<max_count; ++k) {
					++index;
					dictionary->updateDictionary(data[index]);
				}
				context = data[index];
			}
			else {
				break;
			}
		}
		++index;
	} while (index < data_size);
	//We set termination flag that can be recognized without mistake. 
	if (isLast) {
		ppm->EncodeBit(1, prev_flag);
		ppm->EncodeStep(TERMINATION_FLAG);
		ppm->encoder.Flush();
	}
	/////////////////////////////////////
	return true;
}

int process_decompress(Dictionary* dictionary, unsigned char* data, int& index) {
	int flag = 0;
	int prev_count = 0;
	unsigned char context = 0;
	index = 0;
	while (true) {
		flag = ppm->DecodeBit(flag);
		if (flag == 0) {
			data[index] = ppm->DecodeLiteral(context);
			context = data[index];
			dictionary->updateDictionary(data[index]);
			++index;
			if (index >= CHUNK_SIZE) return 1;
		}
		else {
			int step = ppm->DecodeStep();
			if (step == TERMINATION_FLAG)  return 0;
			//
			int length = ppm->DecodeLength(prev_count);
			prev_count = length;

			int offset = 0; 
			int position = index - 1;
			for (int k=0; k<=step; ++k) {
				position = dictionary->getNextPosition(position);
			}
			offset = index - 1 - position;
			for (int k=0; k<length; ++k) {
				data[index] = data[index - offset];
				dictionary->updateDictionary(data[index]);
				context = data[index]; 
				++index;
				if (index >= CHUNK_SIZE) return 1;
			}
		}
	}
}

static void ppm_init(void) {
    if(!ppm) ppm = new TPPM;
}
}

extern "C" bool irolz_compress(char* inFile, char* outFile) {
    irolz::ppm_init();
	FILE* in;
	if ((in = fopen(inFile, "rb")) == NULL) {
		printf("Failed to open in file %s\n", inFile);
		return false;
	}
	if ((outCmp = fopen(outFile, "wb")) == NULL) {
		printf("Failed to open %s out file\n", outFile);
		return false;
	}
	unsigned char* data = (unsigned char*)malloc(CHUNK_SIZE);
	irolz::Dictionary* dictionary = new irolz::Dictionary(PREFIX_SIZE, HISTORY_BUFFER_BITLEN);
	//we read and process data by large chunks
	while (true) {
		int data_size = fread(data, 1, CHUNK_SIZE, in);
		bool isLast = false;
		if (data_size < CHUNK_SIZE) isLast = true;
		irolz::exetransform(data, 1, data_size);
		dictionary->eraseData();
		bool res_flag = irolz::process_compress(dictionary, data, data_size, isLast);
		if (isLast) break;
	}
	delete dictionary;
	if (data) free(data);
	fclose(in);
	fflush(outCmp);
	fclose(outCmp);

	return true;
}

extern "C" bool irolz_decompress(char* inFile, char* outFile) {
    irolz::ppm_init();
	if ((inCmp = fopen(inFile, "rb")) == NULL) {
		printf("Failed to open in file %s\n", inFile);
		return false;
	}
	FILE* out;
	if ((out = fopen(outFile, "wb")) == NULL) {
		printf("Failed to open %s out file\n", outFile);
		return false;
	}

	unsigned char* data = (unsigned char*)malloc(CHUNK_SIZE);
	int data_size = CHUNK_SIZE;
	irolz::Dictionary* dictionary = new irolz::Dictionary(PREFIX_SIZE, HISTORY_BUFFER_BITLEN);
	irolz::ppm->decoder.Init();
	//we decode data by large chunks
	while (true) {
		dictionary->eraseData();
		int status = irolz::process_decompress(dictionary, data, data_size);
		irolz::exetransform(data, 0, data_size);
		int res_size = fwrite(data, 1, data_size, out);
		if (res_size != data_size) {
			printf("Failed to write data\n");
			if (data) free(data);
			return false;
		}
		if (status == 0) break;
	}
	delete dictionary;
	if (data) free(data);
	fflush(out);
	fclose(out);
	return true;
}

/*
int main(int argc, char** argv) {

	printf("Program name: iROLZ, author: Andrew Polar under GPL ver. 3. LICENSE\n");
	if (argc != 4) {
		printf("Usage for compression  :   irolz e input output\n");
		printf("Usage for decompression:   irolz d input output\n");
		exit(0);
	}

	if (argv[1][0] == 'e') {
		clock_t start_encoding = clock();
		bool res = irolz_compress(argv[2], argv[3]);
		clock_t end_encoding = clock();
		printf("Time for encoding %2.3f sec.\n", (double)(end_encoding - start_encoding)/CLOCKS_PER_SEC);
		if (res) {
			int res_size = getFileSize(argv[3]);
			int src_size = getFileSize(argv[2]);
			if (src_size == 0) src_size = 1;
			printf("Compression ratio %f relative to original\n", ((double)(res_size))/((double)(src_size)));
			exit(0);
		}
		else {
			printf("Failed to compress file\n");
			exit(1);
		}
	}
	else if (argv[1][0] == 'd') {
		clock_t start_encoding = clock();
		bool res = irolz_decompress(argv[2], argv[3]);
		clock_t end_encoding = clock();
		printf("Time for decoding %2.3f sec.\n", (double)(end_encoding - start_encoding)/CLOCKS_PER_SEC);
		if (res) {
			int res_size = getFileSize(argv[3]);
			int src_size = getFileSize(argv[2]);
			if (res_size == 0) res_size = 1;
			printf("Compression ratio %f relative to original\n", ((double)(src_size))/((double)(res_size)));
			exit(0);
		}
		else {
			printf("Failed to decompress file\n");
			exit(1);
		}
	}
	else {
		printf("Misformatted command string\n");
		exit(1);
	}

	return 0;
}
*/


extern "C" int irolz_uncompress(unsigned char *in, int insz, unsigned char *out, int outsz) {
    mem2mem_init(in, insz, out, outsz);
    if(irolz_decompress ((char*)in, (char*)out) == false) return -1;
    return mem2mem_ret();
}


