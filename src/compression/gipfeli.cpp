#include "../libs/gipfeli/gipfeli.h"
#include <string.h>

using std::string;

extern "C" int gipfeli_uncompress(void *in, int insz, void *out, int outsz) {
    int     ret = -1;
    size_t  size;
    string tmp_in, tmp_out;
    util::compression::Compressor *z = util::compression::NewGipfeliCompressor();
    tmp_in.assign((const char *)in, (size_t)insz);
    if(!z->GetUncompressedLength(tmp_in, &size)) { ret = -1; goto quit; }
    if(size > outsz) { ret = -2; goto quit; }
    if(!z->Uncompress(tmp_in, &tmp_out)) { ret = -3; goto quit; }
    memcpy(out, tmp_out.data(), size);
    ret = size;
quit:
    delete z;
    return ret;
}

extern "C" int gipfeli_compress(void *in, int insz, void *out, int outsz) {
    int     ret = -1;
    string tmp_in, tmp_out;
    util::compression::Compressor *z = util::compression::NewGipfeliCompressor();
    tmp_in.assign((const char *)in, (size_t)insz);
    if(!z->Compress(tmp_in, &tmp_out)) { ret = -1; goto quit; }
    if(tmp_out.size() > outsz) { ret = -2; goto quit; }
    memcpy(out, tmp_out.data(), tmp_out.size());
    ret = tmp_out.size();
quit:
    delete z;
    return ret;
}
