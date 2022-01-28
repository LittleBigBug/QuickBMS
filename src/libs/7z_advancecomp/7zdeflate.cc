#include "7z.h"

#include "DeflateEncoder.h"
#include "DeflateDecoder.h"

#include "zlib.h"

bool compress_deflate_7z(const unsigned char* in_data, unsigned in_size, unsigned char* out_data, unsigned& out_size, unsigned num_passes, unsigned num_fast_bytes) throw ()
{
	try {
		NDeflate::NEncoder::CCoder cc;

		if (cc.SetEncoderNumPasses(num_passes) != S_OK)
			return false;

		if (cc.SetEncoderNumFastBytes(num_fast_bytes) != S_OK)
			return false;

		ISequentialInStream in(reinterpret_cast<const char*>(in_data), in_size);
		ISequentialOutStream out(reinterpret_cast<char*>(out_data), out_size);

		UINT64 in_size_l = in_size;

		if (cc.Code(&in, &out, &in_size_l) != S_OK)
			return false;

		out_size = out.size_get();

		if (out.overflow_get())
			return false;

		return true;
	} catch (...) {
		return false;
	}
}

bool decompress_deflate_7z(const unsigned char* in_data, unsigned in_size, unsigned char* out_data, unsigned out_size) throw () {
	try {
		NDeflate::NDecoder::CCoder cc;

		ISequentialInStream in(reinterpret_cast<const char*>(in_data), in_size);
		ISequentialOutStream out(reinterpret_cast<char*>(out_data), out_size);

		UINT64 in_size_l = in_size;
		UINT64 out_size_l = out_size;

		if (cc.Code(&in, &out, &in_size_l, &out_size_l) != S_OK)
			return false;

		if (out.size_get() != out_size || out.overflow_get())
			return false;

		return true;
	} catch (...) {
		return false;
	}
}

bool compress_rfc1950_7z(const unsigned char* in_data, unsigned in_size, unsigned char* out_data, unsigned& out_size, unsigned num_passes, unsigned num_fast_bytes) throw ()
{
	if (out_size < 6)
		return false;

	// 8 - deflate
	// 7 - 32k window
	// 3 - max compression
	unsigned header = (8 << 8) | (7 << 12) | (3 << 6);

	header += 31 - (header % 31);

	out_data[0] = (header >> 8) & 0xFF;
	out_data[1] = header & 0xFF;
	out_data += 2;

	unsigned size = out_size - 6;
	if (!compress_deflate_7z(in_data, in_size, out_data, size, num_passes, num_fast_bytes)) {
		return false;
	}
	out_data += size;

	unsigned adler = adler32(adler32(0,0,0), in_data, in_size);

	out_data[0] = (adler >> 24) & 0xFF;
	out_data[1] = (adler >> 16) & 0xFF;
	out_data[2] = (adler >> 8) & 0xFF;
	out_data[3] = adler & 0xFF;

	out_size = size + 6;

	return true;
}
