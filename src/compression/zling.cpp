#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../libs/libzling/libzling.h"
#include "../libs/libzling/libzling_utils.h"

namespace baidu {
namespace zling {

    struct memInputter: public baidu::zling::Inputter {
        memInputter(unsigned char *fp, int fpsz):
            m_fp(fp),
            m_fpl(fp + fpsz),
            m_total(0) {}

        size_t GetData(unsigned char* buf, size_t len) {
            if(len > (m_fpl - m_fp)) len = m_fpl - m_fp;
            memcpy(buf, m_fp, len);
            m_fp += len;
            m_total += len;
            return len;
        }
        bool IsEnd() { return (m_fp >= m_fpl); }
        bool IsErr() { return 0; }
        size_t GetInputSize() { return m_total; }
    private:
        unsigned char *m_fp;
        unsigned char *m_fpl;
        size_t m_total;
    };

    struct memOutputter: public baidu::zling::Outputter {
        memOutputter(unsigned char *fp, int fpsz):
            m_fp(fp),
            m_fpl(fp + fpsz),
            m_total(0) {}

        size_t PutData(unsigned char* buf, size_t len) {
            if(len > (m_fpl - m_fp)) len = m_fpl - m_fp;
            memcpy(m_fp, buf, len);
            m_fp += len;
            m_total += len;
            return len;
        }
        bool IsErr() { return 0; }
        size_t GetOutputSize() { return m_total; }
    private:
        unsigned char *m_fp;
        unsigned char *m_fpl;
        size_t m_total;
    };

/*
    int Inputter::GetChar() {
        unsigned char ch;
        GetData(&ch, 1);
        return ch;
    }
    uint32_t Inputter::GetUInt32() {
        uint32_t v = 0;
        v += GetChar() << 24;
        v += GetChar() << 16;
        v += GetChar() << 8;
        v += GetChar();
        return v;
    }

    int Outputter::PutChar(int v) {
        unsigned char ch = v;
        PutData(&ch, 1);
        return ch;
    }
    uint32_t Outputter::PutUInt32(uint32_t v) {
        PutChar((v >> 24) & 0xff);
        PutChar((v >> 16) & 0xff);
        PutChar((v >> 8) & 0xff);
        PutChar((v) & 0xff);
        return v;
    }
*/

}  // namespace zling
}  // namespace baidu



extern "C"
int zling_compress(unsigned char *in, int insz, unsigned char *out, int outsz) {
    baidu::zling::memInputter  inputter(in, insz);
    baidu::zling::memOutputter outputter(out, outsz);
    baidu::zling::Encode(&inputter, &outputter, NULL, 4);
    return outputter.GetOutputSize();
}

extern "C"
int zling_decompress(unsigned char *in, int insz, unsigned char *out, int outsz) {
    baidu::zling::memInputter  inputter(in, insz);
    baidu::zling::memOutputter outputter(out, outsz);
    baidu::zling::Decode(&inputter, &outputter, NULL);
    return outputter.GetOutputSize();
}

