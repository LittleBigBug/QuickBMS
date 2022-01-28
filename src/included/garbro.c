// code from GARBro by morkt
// converted by Luigi Auriemma
// My code has NOT been tested and some functions may give incorrect output.
// Some of these functions may also be duplicates of other algorithms already available.
// The code is meant to work on raw data (no headers)



void Binary_CopyOverlapped(u8 *data, int src, int dst, int count) {
    mymemmove(data + dst, data + src, count);
}

void Buffer_BlockCopy (u8 *input, int src, u8 *output, int dst, int count) {
    mymemmove(output + dst, input + src, count);
}



// GARBro ArcVCT
int vct_lzs_unpack(unsigned char *input, int packed_size, unsigned char *output, int unpacked_size) {
    //*(u32*)input; input += sizeof(u32);
    //int unpacked_size = *(u32*)input; input += sizeof(u32);
    //int packed_size = *(u32*)input; input += sizeof(u32);
    int ctl_size = *(u32*)input;
    input += 4;
    byte *ctl = input;
    input += ctl_size;
    byte frame[0x1000]="";
    int frame_pos = 1;
    int src = 0;
    int dst = 0;
    int bits = 2;
    while (dst < unpacked_size)
    {
        bits >>= 1;
        if (1 == bits)
        {
            bits = ctl[src++] | 0x100;
        }
        if (0 != (bits & 1))
        {
            output[dst++] = frame[frame_pos++ & 0xFFF] = *input++;
        }
        else
        {
            int offset = *(u16*)input; input += sizeof(u16);
            int count = (offset >> 12) + 2;
            while (count --> 0)
            {
                u8 b = frame[offset++ & 0xFFF];
                output[dst++] = frame[frame_pos++ & 0xFFF] = b;
            }
        }
    }
    return dst;
}



// GARBro ArcBIN
int umesoft_unpack(unsigned char *m_input, unsigned char *m_output, int m_output_Length) {
#if defined(__GNUC__) && !defined(__clang__)
    int m_bits;

    int GetBit ()
    {
        int bit = m_bits & 1;
        m_bits >>= 1;
        if (1 == m_bits)
        {
            m_bits = *(u16*)m_input | 0x10000;
            m_input += sizeof(u16);
        }
        return bit;
    }

    m_bits = 2;
    GetBit();
    int dst = 0;
    while (dst < m_output_Length)
    {
        int offset, shift, count;
        if (GetBit() != 0)
        {
            m_output[dst++] = *m_input++;
            continue;
        }
        if (GetBit() != 0)
        {
            offset = *m_input++ | -0x100;
            shift = 0;
            if (GetBit() == 0)
                shift += 0x100;
            if (GetBit() == 0)
            {
                offset -= 0x200;
                if (GetBit() == 0)
                {
                    shift <<= 1;
                    if (GetBit() == 0)
                        shift += 0x100;
                    offset -= 0x200;
                    if (GetBit() == 0)
                    {
                        shift <<= 1;
                        if (GetBit() == 0)
                            shift += 0x100;
                        offset -= 0x400;
                        if (GetBit() == 0)
                        {
                            offset -= 0x800;
                            shift <<= 1;
                            if (GetBit() == 0)
                                shift += 0x100;
                        }
                    }
                }
            }
            offset -= shift;
            if (GetBit() != 0)
            {
                count = 3;
            }
            else if (GetBit() != 0)
            {
                count = 4;
            }
            else if (GetBit() != 0)
            {
                count = 5;
            }
            else if (GetBit() != 0)
            {
                count = 6;
            }
            else if (GetBit() != 0)
            {
                if (GetBit() != 0)
                    count = 8;
                else
                    count = 7;
            }
            else if (GetBit() != 0)
            {
                count = *m_input++ + 17;
            }
            else
            {
                count = 9;
                if (GetBit() != 0)
                    count = 13;
                if (GetBit() != 0)
                    count += 2;
                if (GetBit() != 0)
                    count++;
            }
        }
        else
        {
            offset = *m_input++ | -0x100;
            if (GetBit() != 0)
            {
                offset -= 0x100;
                if (GetBit() == 0)
                    offset -= 0x400;
                if (GetBit() == 0)
                    offset -= 0x200;
                if (GetBit() == 0)
                    offset -= 0x100;
            }
            else if (offset == -1)
            {
                if (GetBit() == 0)
                    break;
                else
                    continue;
            }
            count = 2;
        }
        count = MIN (count, m_output_Length - dst);
        Binary_CopyOverlapped(m_output, dst+offset, dst, count);
        dst += count;
    }
    return dst;
#else
    return -1;
#endif
}



// GARBro ArcDAT
int systemaqua_catf_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    u8 bitchr=0,bitpos=0;

    byte frame[0x4000]="";
    int frame_pos = 1;
    int dst = 0;
    while (dst < output_Length)
    {
        int ctl = UNZBITS(input, 1);
        if (-1 == ctl)
            break;
        if (ctl != 0)
        {
            int v = UNZBITS(input, 8);
            output[dst++] = frame[frame_pos++ & 0x3FFF] = (byte)v;
        }
        else
        {
            int offset = UNZBITS(input, 14);
            int count = UNZBITS(input, 4) + 3;
            while (count --> 0)
            {
                byte v = frame[offset++ & 0x3FFF];
                output[dst++] = frame[frame_pos++ & 0x3FFF] = v;
            }
        }
    }
    return dst;
}



// GARBro ArcBIN/ArcSGS
int sogna_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    int dst = 0;
    int bits = 0;
    byte mask = 0;
    while (dst < output_Length)
    {
        mask >>= 1;
        if (0 == mask)
        {
            bits = *input++;
            if (-1 == bits)
                break;
            mask = 0x80;
        }
        if ((mask & bits) != 0)
        {
            int offset = *(u16*)input; input += sizeof(u16);
            int count = (offset >> 12) + 1;
            offset &= 0xFFF;
            Binary_CopyOverlapped(output, dst-offset, dst, count);
            dst += count;
        }
        else
        {
            output[dst++] = *input++;
        }
    }
    return dst;
}



// GARBro ArcPAC
int pac_ads_unpack(unsigned char *m_input, unsigned char *m_buffer, int m_length) {
    u8 buffer[8] = "";
    int chunk_size = 3;
    int i;
    int m_pos = 0;
    for (;;)
    {
        int ctl = *m_input++;
        if (-1 == ctl)
            break;
        if (ctl != 0)
        {
            memcpy(buffer + 4, m_input, 4);
            m_input += 4;
            int count = (*(u32*)(buffer + 4)) - 1;
            while (count --> 0)
            {
                for (i = 0; i < chunk_size; ++i)
                {
                    m_buffer[m_pos++] = buffer[i];
                    if (0 == --m_length)
                        return m_pos;
                }
            }
        }
        else
        {
            chunk_size = 3;
            memcpy(buffer, m_input, chunk_size); m_input += chunk_size;
            if (0 == chunk_size)
                break;
            for (i = 0; i < chunk_size; ++i)
            {
                m_buffer[m_pos++] = buffer[i];
                if (0 == --m_length)
                    return m_pos;
            }
        }
    }
    return m_pos;
}



// GARBro ArcAil
int ail_lzs_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    int frame_pos = 0xfee;
    byte frame[0x1000]="";
    int i;
    for (i = 0; i < frame_pos; ++i)
        frame[i] = 0x20;
    int dst = 0;
    int ctl = 0;

    while (dst < output_Length)
    {
        ctl >>= 1;
        if (0 == (ctl & 0x100))
        {
            ctl = *input++;
            if (-1 == ctl)
                break;
            ctl |= 0xff00;
        }
        if (0 == (ctl & 1))
        {
            int v = *input++;
            if (-1 == v)
                break;
            output[dst++] = (byte)v;
            frame[frame_pos++] = (byte)v;
            frame_pos &= 0xfff;
        }
        else
        {
            int offset = *input++;
            if (-1 == offset)
                break;
            int count = *input++;
            if (-1 == count)
                break;
            offset |= (count & 0xf0) << 4;
            count   = (count & 0x0f) + 3;

            for (i = 0; i < count; i++)
            {	
                if (dst >= output_Length)
                    break;
                byte v = frame[offset++];
                offset &= 0xfff;
                frame[frame_pos++] = v;
                frame_pos &= 0xfff;
                output[dst++] = v;
            }
        }
    }
    return dst;
}



// GARBro ArcPAK
int agsi_unpack(unsigned char *m_input, unsigned char *m_buffer, int m_length) {
    u8 bitchr=0,bitpos=0;

    byte frame[0x1000]="";
    int dst = 0;
    int frame_pos = 1;
    int m_pos = 0;
    while (dst < m_length)
    {
        int bit = UNZBITS(m_input, 1);
        if (bit != 0)
        {
            if (-1 == bit)
                break;
            int v = UNZBITS(m_input, 8);
            if (-1 == v)
                break;
            frame[frame_pos++ & 0xFFF] = m_buffer[m_pos++] = (byte)v;
            dst++;
            if (0 == --m_length)
                return m_pos;
        }
        else
        {
            int offset = UNZBITS(m_input, 12);
            if (-1 == offset)
                break;
            int count = UNZBITS(m_input, 4);
            if (-1 == count)
                break;
            count += 2;
            dst += count;
            while (count --> 0)
            {
                byte v = frame[offset++ & 0xFFF];
                frame[frame_pos++ & 0xFFF] = v;
                m_buffer[m_pos++] = v;
                if (0 == --m_length)
                    return m_pos;
            }
        }
    }
    return m_pos;
}



int foster_fa2_unpack(unsigned char *m_input, unsigned char *m_output, int m_output_Length) {
#if defined(__GNUC__) && !defined(__clang__)
    uint    m_bits;
    int     m_bit_count;

    void FetchBits ()
    {
        m_bits = *(u32*)m_input; m_input += sizeof(u32);
        m_bit_count = 32;
    }

    int GetNextBit ()
    {
        if (0 == m_bit_count)
            FetchBits();
        int bit = (int)((m_bits >> 31) & 1);
        m_bits <<= 1;
        --m_bit_count;
        return bit;
    }

    int GetBits (int count)
    {
        uint bits = 0;
        int avail_bits = MIN (count, m_bit_count);
        if (avail_bits > 0)
        {
            bits = m_bits >> (32 - avail_bits);
            m_bits <<= avail_bits;
            m_bit_count -= avail_bits;
            count -= avail_bits;
        }
        if (count > 0)
        {
            FetchBits();
            bits = bits << count | m_bits >> (32 - count);
            m_bits <<= count;
            m_bit_count -= count;
        }
        return (int)bits;
    }

    m_bit_count = 0;
    int dst = 0;
    while (dst < m_output_Length)
    {
        if (GetNextBit() != 0)
        {
            m_output[dst++] = *m_input++;
            continue;
        }
        int offset;
        if (GetNextBit() != 0)
        {
            if (GetNextBit() != 0)
            {
                offset = *m_input++ << 3;
                offset |= GetBits (3);
                offset += 0x100;
                if (offset >= 0x8FF)
                    break;
            }
            else
            {
                offset = *m_input++;
            }
            m_output[dst  ] = m_output[dst-offset-1];
            m_output[dst+1] = m_output[dst-offset  ];
            dst += 2;
        }
        else
        {
            if (GetNextBit() != 0)
            {
                offset = *m_input++ << 1;
                offset |= GetNextBit();
            }
            else
            {
                offset = 0x100;
                if (GetNextBit() != 0)
                {
                    offset |= *m_input++;
                    offset <<= 1;
                    offset |= GetNextBit();
                }
                else if (GetNextBit() != 0)
                {
                    offset |= *m_input++;
                    offset <<= 2;
                    offset |= GetBits (2);
                }
                else if (GetNextBit() != 0)
                {
                    offset |= *m_input++;
                    offset <<= 3;
                    offset |= GetBits (3);
                }
                else
                {
                    offset |= *m_input++;
                    offset <<= 4;
                    offset |= GetBits (4);
                }
            }
            int count = 0;
            if (GetNextBit() != 0)
            {
                count = 3;
            }
            else if (GetNextBit() != 0)
            {
                count = 4;
            }
            else if (GetNextBit() != 0)
            {
                count = 5 + GetNextBit();
            }
            else if (GetNextBit() != 0)
            {
                count = 7 + GetBits (2);
            }
            else if (GetNextBit() != 0)
            {
                count = 11 + GetBits (4);
            }
            else
            {
                count = 27 + *m_input++;
            }
            Binary_CopyOverlapped (m_output, dst - offset - 1, dst, count);
            dst += count;
        }
    }
    return dst;
#else
    return -1;
#endif
}



// GARBro ArcAN21
int an21_unpack(unsigned char *input, unsigned char *output, int output_Length, int rle_step) {
    if(rle_step < 0) rle_step = 1;  // ???
    int i;
    int dst = 0;
    for (i = 0; i < rle_step; ++i)
    {
        byte v1 = *input++;
        output[i] = v1;
        dst = i + rle_step;
        while (dst < output_Length)
        {
            byte v2 = *input++;
            output[dst] = v2;
            dst += rle_step;
            if (v2 == v1)
            {
                int count = *input++;
                if (0 != (count & 0x80))
                    count = *input++ + ((count & 0x7F) << 8) + 128;
                while (count --> 0 && dst < output_Length)
                {
                    output[dst] = v2;
                    dst += rle_step;
                }
                if (dst < output_Length)
                {
                    v2 = *input++;
                    output[dst] = v2;
                    dst += rle_step;
                }
            }
            v1 = v2;
        }
    }
    return dst;
}



// GARBro ArcLINK
int arc_link_unpack(unsigned char *input, unsigned char *result, int result_Length, int m_step) {
    if(m_step < 0) m_step = 1;  // ???
    int src = 0;
    int dst = 0;
    int i;
    for (i = 0; i < m_step; ++i)
    {
        byte v1 = input[src++];
        result[i] = v1;
        dst = i + m_step;
        while (dst < result_Length)
        {
            byte v2 = input[src++];
            result[dst] = v2;
            dst += m_step;
            if (v2 == v1)
            {
                int count = input[src++];
                if (0 != (count & 0x80))
                    count = input[src++] + ((count & 0x7F) << 8) + 128;
                while (count --> 0 && dst < result_Length)
                {
                    result[dst] = v2;
                    dst += m_step;
                }
                if (dst < result_Length)
                {
                    v2 = input[src++];
                    result[dst] = v2;
                    dst += m_step;
                }
            }
            v1 = v2;
        }
    }
    return dst;
}



// GARBro ArcBK
int maika_bk_unpack(unsigned char *m_input, unsigned char *m_buffer, int m_length) {
    u8  bitchr=0,bitpos=0;

    byte frame[0x400]="";
    int frame_pos = 1;
    int m_pos = 0;
    for (;;)
    {
        int bit = UNZBITS(m_input, 1);
        if (-1 == bit)
            break;
        if (bit != 0)
        {
            int v = UNZBITS(m_input, 8);
            if (-1 == v)
                break;
            m_buffer[m_pos++] = frame[frame_pos++ & 0x3FF] = (byte)v;
            if (0 == --m_length)
                return m_pos;
        }
        else
        {
            int offset = UNZBITS(m_input, 10);
            if (-1 == offset)
                break;
            int count = UNZBITS(m_input, 5);
            if (-1 == count)
                break;
            count += 2;
            while (count-- > 0)
            {
                byte v = frame[offset++ & 0x3FF];
                m_buffer[m_pos++] = frame[frame_pos++ & 0x3FF] = v;
                if (0 == --m_length)
                    return m_pos;
            }
        }
    }
    return m_pos;
}



// GARBro ArcMK2/ArcGM
int maika_mk2_unpack(unsigned char *m_input, unsigned char *m_buffer, int m_length, int m_rle_code) {
    if(m_rle_code < 0) m_rle_code = 1;  // 1 is the value of Eve ArcGM
    int m_pos = 0;
    for (;;)
    {
        int ctl = *m_input++;
        if (-1 == ctl || 0xFF == ctl)
            break;
        int count = *(u32*)m_input; m_input += sizeof(u32);
        if (m_rle_code == ctl)
        {
            byte b = *m_input++;
            while (count --> 0)
            {
                m_buffer[m_pos++] = b;
                if (0 == --m_length)
                    return m_pos;
            }
        }
        else
        {
            while (count > 0)
            {
                int chunk = MIN (count, m_length);
                memcpy(m_buffer + m_pos, m_input, chunk); m_input += chunk;
                count -= chunk;
                m_pos += chunk;
                m_length -= chunk;
                if (0 == m_length)
                    return m_pos;
            }
        }
    }
    return m_pos;
}



// GARBro ArcMGR
int propeller_mgr_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    int dst = 0;
    while (dst < output_Length)
    {
        int count = *input++;
        if (-1 == count)
            break;
        if (count < 0x20)
        {
            count = MIN (count+1, output_Length-dst);
            int read = count; memcpy(output + dst, input, count); input += count;
            dst += read;
            if (read < count)
                break;
        }
        else
        {
            int offset = ((count & 0x1F) << 8) + 1;
            count >>= 5;
            if (7 == count)
                count += *input++;
            offset += *input++;
            if (offset >= dst) return -1;
            count = MIN (count+2, output_Length-dst);
            Binary_CopyOverlapped (output, dst - offset, dst, count);
            dst += count;
        }
    }
    return dst;
}



// GARBro ArcQLIE
int qlie_unpack(unsigned char *input, int input_Length, unsigned char *output, int output_Length) {
    int is_16bit = 1;
    u8  node[2][256];
    u8  child_node[256];

    if (*(u32*)input == 0xFF435031) { // '1PC\xFF'
        is_16bit = 0 != (input[4] & 1);
        //int output_length = _QUICK_GETi32 (input, 8);
    }

    int src = 12;
    int dst = 0;
    while (src < input_Length)
    {
        int i, k, count, index;

        for (i = 0; i < 256; i++)
            node[0][i] = (byte)i;

        for (i = 0; i < 256; )
        {
            count = input[src++];

            if (count > 127)
            {
                int step = count - 127;
                i += step;
                count = 0;
            }

            if (i > 255)
                break;

            count++;
            for (k = 0; k < count; k++)
            {
                node[0][i] = input[src++];
                if (node[0][i] != i)
                    node[1][i] = input[src++];
                i++;
            }
        }

        if (is_16bit)
        {
            count = *(u16*)(input + src);
            src += 2;
        }
        else
        {
            count = *(u32*)(input + src);
            src += 4;
        }

        k = 0;
        for (;;)
        {
            if (k > 0)
                index = child_node[--k];
            else
            {
                if (0 == count)
                    break;
                count--;
                index = input[src++];
            }

            if (node[0][index] == index)
                output[dst++] = (byte)index;
            else
            {
                child_node[k++] = node[1][index];
                child_node[k++] = node[0][index];
            }
        }
    }

    return dst;
}



// GARBro ArcSEEN
int avg32_seen_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    int dst = 0;
    int bits = 0;
    int mask = 0;
    while (dst < output_Length)
    {
        mask >>= 1;
        if (0 == mask)
        {
            bits = *input++;
            mask = 0x80;
        }
        if (0 != (bits & mask))
        {
            output[dst++] = *input++;
        }
        else
        {
            int offset = *(u16*)input; input += sizeof(u16);
            int count = (offset & 0xF) + 2;
            offset >>= 4;
            Binary_CopyOverlapped (output, dst-offset, dst, count);
            dst += count;
        }
    }
    return dst;
}



// GARBro ArcIAR
int sas5_iar_unpack(unsigned char *m_input, unsigned char *output, int output_Length) {
#if defined(__GNUC__) && !defined(__clang__)
    int m_bits;

    int GetNextBit ()
    {
        if (1 == m_bits)
        {
            m_bits = *(u16*)m_input | 0x10000;  m_input += sizeof(u16);
        }
        int b = m_bits & 1;
        m_bits >>= 1;
        return b;
    }


    m_bits = 1;
    int dst = 0;
    while (dst < output_Length)
    {
        if (1 == GetNextBit())
        {
            output[dst++] = *m_input++;
            continue;
        }
        int offset, count;
        if (1 == GetNextBit())
        {
            int tmp = GetNextBit();
            if (1 == GetNextBit())
                offset = 1;
            else if (1 == GetNextBit())
                offset = 0x201;
            else
            {
                tmp = (tmp << 1) | GetNextBit();
                if (1 == GetNextBit())
                    offset = 0x401;
                else
                {
                    tmp = (tmp << 1) | GetNextBit();
                    if (1 == GetNextBit())
                        offset = 0x801;
                    else
                    {
                        offset = 0x1001;
                        tmp = (tmp << 1) | GetNextBit();
                    }
                }
            }
            offset += (tmp << 8) | *m_input++;
            if (1 == GetNextBit())
                count = 3;
            else if (1 == GetNextBit())
                count = 4;
            else if (1 == GetNextBit())
                count = 5;
            else if (1 == GetNextBit())
                count = 6;
            else if (1 == GetNextBit())
                count = 7 + GetNextBit();
            else if (1 == GetNextBit())
                count = 17 + *m_input++;
            else
            {
                count = GetNextBit() << 2;
                count |= GetNextBit() << 1;
                count |= GetNextBit();
                count += 9;
            }
        }
        else
        {
            count = 2;
            if (1 == GetNextBit())
            {
                offset = GetNextBit() << 10;
                offset |= GetNextBit() << 9;
                offset |= GetNextBit() << 8;
                offset = (offset | *m_input++) + 0x100;
            }
            else
            {
                offset = 1 + *m_input++;
                if (0x100 == offset)
                    break;
            }
        }
        Binary_CopyOverlapped (output, dst - offset, dst, count);
        dst += count;
    }
    return dst;
#else
    return -1;
#endif
}



// GARBro ArcIAR
int seraphim_scn_unpack(unsigned char *input, unsigned char *data, int unpacked_size) {
    int dst = 0;
    while (dst < unpacked_size)
    {
        int ctl = *input++;
        if (-1 == ctl)
            return -1;
        if (0 != (ctl & 0x80))
        {
            byte lo = *input++;
            int offset = ((ctl << 3 | lo >> 5) & 0x3FF) + 1;
            int count = (lo & 0x1F) + 1;
            Binary_CopyOverlapped (data, dst-offset, dst, count);
            dst += count;
        }
        else
        {
            int count = ctl + 1;
            memcpy (data + dst, input, count);
            dst += count;
        }
    }
    return dst;
}



// GARBro ArcDET
int ugos_det_unpack(unsigned char *m_input, unsigned char *m_buffer, int m_length) {
    byte frame[0x100]="";
    int frame_pos = 0;
    const int frame_mask = 0xFF;
    int m_pos = 0;
    for (;;)
    {
        int ctl = *m_input++;
        if (-1 == ctl)
            break;
        if (0xFF != ctl)
        {
            m_buffer[m_pos++] = frame[frame_pos++ & frame_mask] = (byte)ctl;
            if (0 == --m_length)
                return m_pos;
        }
        else
        {
            ctl = *m_input++;
            if (-1 == ctl)
                break;
            if (0xFF == ctl)
            {
                m_buffer[m_pos++] = frame[frame_pos++ & frame_mask] = 0xFF;
                if (0 == --m_length)
                    return m_pos;
            }
            else
            {
                int offset = frame_pos - ((ctl >> 2) + 1);
                int count = (ctl & 3) + 3;
                while (count --> 0)
                {
                    byte v = frame[offset++ & frame_mask];
                    m_buffer[m_pos++] = frame[frame_pos++ & frame_mask] = v;
                    if (0 == --m_length)
                        return m_pos;
                }
            }
        }
    }
    return m_pos;
}



// GARBro ArcFL4
int aaru_fl4_unpack(unsigned char *m_input, unsigned char *m_buffer, int m_length, int Chunks) {
    if(Chunks < 0) Chunks = 1;  // ???
    int i;
    int m_pos = 0;
    for (i = 0; i < Chunks; ++i)
    {
        int ctl = *m_input++;
        if (-1 == ctl)
            break;
        int count;
        if (ctl <= 1)
        {
            if (0 == ctl)
                count = *m_input++;
            else
                count = 0x100;
            while (count > 0)
            {
                int avail = MIN (count, m_length);
                int read = avail; memcpy(m_buffer + m_pos, m_input, avail); m_input += avail;
                if (0 == read)
                    break;
                count -= read;
                m_pos += read;
                m_length -= read;
                if (0 == m_length)
                    return m_pos;
            }
        }
        else
        {
            if (3 == ctl)
                { count = *(u16*)m_input; m_input += sizeof(u16); }
            else
                count = ctl;
            byte v = *m_input++;
            while (count --> 0)
            {
                m_buffer[m_pos++] = v;
                if (0 == --m_length)
                    return m_pos;
            }
        }
    }
    return m_pos;
}



// GARBro ArcIDA
int inspire_ida_unpack(unsigned char *m_input, unsigned char *m_buffer, int m_length) {
    int m_pos = 0;
    int processed = 0;
    while (processed < m_length/*output_size*/)
    {
        int ctl = *m_input++;
        if (-1 == ctl)
            break;
        int count = 0;
        if (0 == (ctl & 0x80))
        {
            count = ctl & 0x3F;
        }
        else if (0 == (ctl & 3))
        {
            count = *m_input++;
        }
        else if (1 == (ctl & 3))
        {
            count = *(u16*)m_input; m_input += sizeof(u16);
        }
        else if (3 == (ctl & 3))
        {
            count = *(u32*)m_input; m_input += sizeof(u32);
        }
        processed += count;
        if (0 != (ctl & 0x40))
        {
            byte v = *m_input++;
            while (count --> 0)
            {
                m_buffer[m_pos++] = v;
                if (0 == --m_length)
                    return m_pos;
            }
        }
        else
        {
            while (count > 0)
            {
                int avail = MIN (count, m_length);
                int read = avail; memcpy(m_buffer + m_pos, m_input, avail); m_input += avail;
                count -= read;
                m_pos += read;
                m_length -= read;
                if (0 == m_length)
                    return m_pos;
            }
        }
    }
    return m_pos;
}



// GARBro ArcMPK
int kurumi_mpk_unpack(unsigned char *m_input, unsigned char *m_output, int m_output_Length) {
#if defined(__GNUC__) && !defined(__clang__)
    int     m_bits_avail;
    int     m_bits;
    bool    m_eof;

    int     word_471CE8;
    int     m_buffer_size;
    ushort  word_471CEC;
    short   word_471D00;
    //short   word_471D4E;
    int     word_471D6C;
    ushort  word_471D58;
    ushort  m_tree_size;
    ushort  word_471D60;

    byte*   m_buffer = NULL;
    ushort* m_lhs_nodes = NULL;
    ushort* m_rhs_nodes = NULL;
    byte*   dword_471CFC = NULL;
    ushort* dword_471D04 = NULL;
    ushort* dword_471D54 = NULL;
    byte*   dword_471D64 = NULL;

    void InitTree (short depth)
    {
        //word_471D4E = depth;
        m_tree_size = 0x1FE;
        word_471D58 = (ushort)(depth + 1);
        m_buffer_size = 1 << depth;
        dword_471D64 = calloc(0x1FE, sizeof(byte));
        dword_471CFC = calloc(MAX (0x13, depth + 1), sizeof(byte));
        m_lhs_nodes = calloc(2 * m_tree_size - 1, sizeof(ushort));
        m_rhs_nodes = calloc(2 * m_tree_size - 1, sizeof(ushort));
    }

    ushort LoadBits (int count)
    {
        word_471D60 <<= count;
        if (count > m_bits_avail)
        {
            do
            {
                count -= m_bits_avail;
                word_471D60 |= (ushort)(m_bits << count);
                int bits = *m_input++;
                if (-1 == bits)
                {
                    bits = 0;
                    m_eof = true;
                }
                m_bits = bits;
                m_bits_avail = 8;
            }
            while (count > 8);
        }
        int v = m_bits_avail - count;
        ushort result = (ushort)(m_bits >> v);
        m_bits_avail = v;
        word_471D60 |= result;
        return result;
    }

    int InitBits ()
    {
        m_bits_avail = 0;
        m_bits = 0;
        word_471D60 = 0;
        word_471D00 = 0;
        return LoadBits (word_471CEC);
    }

    short GetBits (short a1)
    {
        int v1 = word_471D60 >> (word_471CEC - a1);
        LoadBits (a1);
        return (short)v1;
    }

    ushort v33[36];

    int sub_42A540 (int a1, byte* a2, ushort a3, ushort* a4)
    {
        int i;
        for (i = 1; i <= 0x10; ++i)
            v33[i] = 0;
        for (i = 0; i < a1; ++i)
            ++v33[a2[i]];

        v33[19] = 0;
        int v6 = 15;
        int v7;
        for (v7 = 0; v7 < 16; ++v7)
        {
            int v10 = v33[v7 + 1] << v6--;
            v33[v7 + 20] = (ushort)(v33[v7 + 19] + v10);
        }
        if (v33[35] != 0)
            return -1; //throw new InvalidFormatException();
        int v11 = 1;
        int v12 = 16 - a3;
        while (v11 <= a3)
        {
            v33[v11 + 18] = (ushort)(v33[v11 + 18] >> v12);
            v33[v11] = (ushort)(1 << (a3 - v11));
            ++v11;
        }
        if (v11 <= 0x10)
        {
            int v14 = v11;
            int v15 = 17 - v11;
            int v16 = v11; // within v33
            do
            {
                v11 = 1 << (16 - v14++);
                v33[v16++] = (ushort)v11;
                --v15;
            }
            while (v15 > 0);
        }
        v11 = v33[a3 + 19] >> v12;
        if (v11 != 0)
        {
            while (v11 != (1 << a3))
            {
                a4[v11++] = 0;
            }
        }
        int v18 = a1;
        int v19 = 0;
        int result = 0;
        while (v19 < a1)
        {
            int v21 = a2[result];
            if (v21 != 0)
            {
                int v32 = v21 + 18; // within v33
                int v22 = v33[v32];
                int v24 = v22 + v33[v21];
                if (v21 > a3)
                {
                    int v23 = v22;
                    ushort *v26 = &a4[v22 >> (16 - a3)];
                    int count;
                    for (count = v21 - a3; count != 0; --count)
                    {
                        if (0 == *v26)
                        {
                            m_lhs_nodes[v18] = 0;
                            m_rhs_nodes[v18] = 0;
                            *v26 = (ushort)v18;
                            ++v18;
                        }
                        if (0 != ((1 << (15 - a3)) & v23))
                            v26 = &m_rhs_nodes[*v26];
                        else
                            v26 = &m_lhs_nodes[*v26];
                        v23 <<= 1;
                    }
                    *v26 = (ushort)v19;
                }
                else
                {
                    while (v22 < v24)
                    {
                        a4[v22++] = (ushort)v19;
                    }
                }
                v33[v32] = (ushort)v24;
            }
            result = ++v19;
        }
        return result;
    }

    void sub_42A1A0 (int a1, short a2, ushort a3)
    {
        int i, count;
        short v3 = GetBits (a2);
        if (0 == v3)
        {
            short v5 = GetBits (a2);
            for (i = 0; i < a1; ++i)
            {
                dword_471CFC[i] = 0;
            }
            for (i = 0; i < 256; ++i)
            {
                dword_471D54[i] = (ushort)v5;
            }
            return;
        }
        int v10;
        for (v10 = 0; v10 < v3; )
        {
            int v11 = word_471D60 >> (word_471CEC - 3);
            if (v11 == 7)
            {
                int v14 = 1 << (word_471CEC - 4);
                while (0 != (word_471D60 & v14))
                {
                    v14 >>= 1;
                    ++v11;
                }
                LoadBits (v11 - 3);
            }
            else if (v11 < 7)
            {
                LoadBits (3);
            }
            else
                LoadBits (v11 - 3);
            dword_471CFC[v10++] = (byte)v11;
            if (v10 == a3)
            {
                for (count = GetBits (2); count > 0; --count)
                {
                    dword_471CFC[v10++] = 0;
                }
            }
        }
        int n = v3;
        for (count = a1 - v3; count > 0; --count)
        {
            dword_471CFC[n++] = 0;
        }
        sub_42A540 (a1, dword_471CFC, 8, dword_471D54);
    }

    void sub_42A2E0 ()
    {
        int i;
        short v0 = GetBits(9);
        if (v0 != 0)
        {
            int v8 = 0;
            while (v8 < v0)
            {
                int v9 = dword_471D54[word_471D60 >> (word_471CEC - 8)];
                int v10 = 1 << (word_471CEC - 9);
                while (v9 >= 19)
                {
                    if (0 != (word_471D60 & v10))
                        v9 = m_rhs_nodes[v9];
                    else
                        v9 = m_lhs_nodes[v9];
                    v10 >>= 1;
                }
                LoadBits (dword_471CFC[v9]);
                if (v9 <= 2)
                {
                    int count;
                    if (0 == v9)
                        count = 1;
                    else if (1 == v9)
                        count = GetBits (4) + 3;
                    else
                        count = GetBits (9) + 20;
                    while (count --> 0)
                    {
                        dword_471D64[v8++] = 0;
                    }
                }
                else
                {
                    dword_471D64[v8++] = (byte)(v9 - 2);
                }
            }
            while (v8 < m_tree_size)
            {
                dword_471D64[v8++] = 0;
            }
            sub_42A540 (m_tree_size, dword_471D64, 12, dword_471D04);
        }
        else
        {
            short v3 = GetBits (9);
            for (i = 0; i < m_tree_size; ++i)
            {
                dword_471D64[i] = 0;
            }
            for (i = 0; i < 0x1000; ++i)
            {
                dword_471D04[i] = (ushort)v3;
            }
        }
    }

    ushort sub_42A090 ()
    {
        if (0 == word_471D00)
        {
            word_471D00 = GetBits (16);
            sub_42A1A0 (19, 5, 3);
            sub_42A2E0();
            sub_42A1A0 (word_471D58, 4, 0xFFFF);
        }
        --word_471D00;
        ushort v1 = dword_471D04[word_471D60 >> (word_471CEC - 12)];
        int v2 = 1 << (word_471CEC - 13);
        while (v1 >= m_tree_size)
        {
            if (0 != (word_471D60 & v2))
                v1 = m_rhs_nodes[v1];
            else
                v1 = m_lhs_nodes[v1];
            v2 >>= 1;
        }
        LoadBits (dword_471D64[v1]);
        return v1;
    }

    short sub_42A490()
    {
        ushort v0 = dword_471D54[word_471D60 >> (word_471CEC - 8)];
        int v1 = 1 << (word_471CEC - 9);
        while (v0 >= word_471D58)
        {
            if (0 != (word_471D60 & v1))
                v0 = m_rhs_nodes[v0];
            else
                v0 = m_lhs_nodes[v0];
            v1 >>= 1;
        }
        LoadBits (dword_471CFC[v0]);
        if (0 == v0)
            return 0;
        short v2 = GetBits ((short)(v0 - 1));
        return (short)((1 << (v0 - 1)) + v2);
    }

    int sub_429D50 ()
    {
        int dst = 0;
        while (--word_471D6C >= 0)
        {
            m_buffer[dst++ & 0xFFFF] = m_buffer[word_471CE8];
            word_471CE8 = (word_471CE8 + 1) & (m_buffer_size - 1);
            if (dst == m_buffer_size)
                return dst;
        }
        while (!m_eof || word_471D00 != 0)
        {
            ushort v4 = sub_42A090();
            if (v4 > 0xFF)
            {
                short offset = sub_42A490();
                word_471D6C = v4 - 254;
                word_471CE8 = (dst - offset - 1) & (m_buffer_size - 1);
                if (word_471D6C >= 0)
                {
                    do
                    {
                        m_buffer[dst++ & 0xFFFF] = m_buffer[word_471CE8];
                        word_471CE8 = (word_471CE8 + 1) & (m_buffer_size - 1);
                        if (dst == m_buffer_size)
                            return dst;
                    }
                    while (--word_471D6C >= 0);
                }
            }
            else
            {
                m_buffer[dst++ & 0xFFFF] = (byte)v4;
                if (dst == m_buffer_size)
                    break;
            }
        }
        return dst;
    }

    u8 *CreateBuffer ()
    {
        word_471CEC = 16;
        if (NULL == m_buffer)
        {
            dword_471D04 = calloc(0x1000, sizeof(ushort));
            dword_471D54 = calloc(0x100, sizeof(ushort));
            InitTree (15);
            m_buffer = calloc(m_buffer_size, sizeof(byte));
        }
        word_471D6C = 0;
        m_eof = false;
        InitBits();
        return m_buffer;
    }

    u8 *data = CreateBuffer();
    int dst = 0;
    while (dst < m_output_Length)
    {
        int count = sub_429D50();
        if (0 == count)
            break;
        memcpy(m_output + dst, data, count);
        dst += count;
    }
    return dst;
#else
    return -1;
#endif
}



// GARBro ArcRLZ
int dice_rlz_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    byte frame[0x800]="";
    int frame_pos = 0x7EF;
    int dst = 0;
    int ctl_bits = 2;
    while (dst < output_Length)
    {
        ctl_bits >>= 1;
        if (1 == ctl_bits)
        {
            ctl_bits = *input++;
            if (-1 == ctl_bits)
                break;
            ctl_bits |= 0x100;
        }
        if (0 != (ctl_bits & 1))
        {
            byte v = *input++;
            output[dst++] = v;
            frame[frame_pos++ & 0x7FF] = v;
        }
        else
        {
            byte lo = *input++;
            byte hi = *input++;
            int offset = (hi & 0xF0) << 4 | lo;
            int count = (hi & 0xF) + 2;
            int i;
            for (i = 0; i < count; ++i)
            {
                byte v = frame[(offset + i) & 0x7FF];
                output[dst++] = v;
                frame[frame_pos++ & 0x7FF] = v;
            }
        }
    }
    return dst;
}



// GARBro ArcPulltop
int pulltop_unpack(unsigned char *input, unsigned char *output, int unpacked_size) {
    int dst = 0;
    byte frame[0x1000]="";
    int frame_pos = 1;
    while (dst < unpacked_size)
    {
        int ctl = *input++;
        int bit;
        for (bit = 1; dst < unpacked_size && bit != 0x100; bit <<= 1)
        {
            if (0 != (ctl & bit))
            {
                byte b = *input++;
                output[dst++] = frame[frame_pos++ & 0xFFF] = b;
            }
            else
            {
                int hi = *input++;
                int lo = *input++;
                int offset = hi << 4 | lo >> 4;
                int count;
                for (count = 2 + (lo & 0xF); count != 0; --count)
                {
                    byte v = frame[offset++ & 0xFFF];
                    output[dst++] = frame[frame_pos++ & 0xFFF] = v;
                }
            }
        }
    }
    return dst;
}



// GARBro ArcVFS
int vnsystem_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    u8  bitchr=0,bitpos=0;

    u8 dict[0x10]="";
    int dict_pos = 0;
    int dst;
    for (dst = 0; dst < output_Length; ++dst)
    {
        byte cur_byte;
        if (UNZBITS(input, 1) != 0)
        {
            int offset = bitpos;
            int pos = dict_pos - offset;
            if (pos < 0)
                pos += sizeof(dict);
            if (pos < 0 || pos >= sizeof(dict))
                return -1; //throw new InvalidDataException ("Invalid compressed data.");
            cur_byte = dict[pos];
        }
        else
        {
            cur_byte = (byte)UNZBITS(input, 8);
        }
        output[dst] = cur_byte;
        dict[dict_pos++] = cur_byte;
        dict_pos &= 0xF;
    }
    return dst;
}



// GARBro ArcDAT
int QlzUnpack(unsigned char *input, unsigned char *output, int output_Length) {
    int dst = 0;
    uint bits = 1;
    int output_last = output_Length - 11;
    while (dst < output_Length)
    {
        if (1 == bits)
        {
            bits = *(u32*)input; input += sizeof(u32);
        }
        if ((bits & 1) == 1)
        {
            int ctl = *input; //PeekByte();
            int offset, count = 3;
            if ((ctl & 3) == 0)
            {
                offset = *input++ >> 2;
            }
            else if ((ctl & 2) == 0)
            {
                offset = *(u16*)input >> 2; input += sizeof(u16);
            }
            else if ((ctl & 1) == 0)
            {
                offset = *(u16*)input >> 6; input += sizeof(u16);
                count += ((ctl >> 2) & 0xF);
            }
            else if ((ctl & 0x7F) != 3)
            {
                offset = (input[0]|(input[1]<<8)|(input[2]<<16) >> 7) & 0x1FFFF;    input += 3;
                count += ((ctl >> 2) & 0x1F) - 1;
            }
            else
            {
                uint v = *(u32*)input; input += sizeof(u32);
                offset = (int)(v >> 15);
                count += (int)((v >> 7) & 0xFF);
            }
            Binary_CopyOverlapped (output, dst-offset, dst, count);
            dst += count;
        }
        else
        {
            if (dst > output_last)
                break;
            output[dst++] = *input++;
        }
        bits >>= 1;
    }
    while (dst < output_Length)
    {
        if (1 == bits)
        {
            input += 4;
            bits = 0x80000000u;
        }
        output[dst++] = *input++;
        bits >>= 1;
    }
    return dst;
}



// GARBro ArcPK
int umesoft_pk_unpack(unsigned char *input, unsigned char *output, int output_size) {
    int ctl = 0;
    int mask = 0;
    int dst = 0;
    while (dst < output_size)
    {
        mask >>= 1;
        if (0 == mask)
        {
            ctl = *input++;
            if (-1 == ctl)
                break;
            mask = 0x80;
        }
        if (0 == (ctl & mask))
        {
            output[dst++] = (byte)*input++;
        }
        else
        {
            int lo = *input++;
            int hi = *input++;
            if (-1 == lo || -1 == hi)
                break;
            int offset = hi << 4 | lo >> 4;
            if (0 == offset)
                break;
            int count  = (lo & 0xF) + 3;
            Binary_CopyOverlapped (output, dst - offset, dst, count);
            dst += count;
        }
    }
    return dst;
}



// GARBro ArcTCD3
int tomcat_tcd_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    int dst = 0;
    int bits = 2;
    while (dst < output_Length)
    {
        bits >>= 1;
        if (1 == bits)
        {
            bits = *input++;
            if (-1 == bits)
                break;
            bits |= 0x100;
        }
        if (0 == (bits & 1))
        {
            int count = *input++;
            int offset = *input++ << 4 | count >> 4;
            count = MIN ((count & 0xF) + 3, output_Length - dst);
            Binary_CopyOverlapped (output, dst-offset, dst, count);
            dst += count;
        }
        else
        {
            output[dst++] = (byte)*input++;
        }
    }
    return dst;
}



// GARBro ArcCAF
int tail_pren_unpack(unsigned char *input, unsigned char *output, int unpacked_size) {
    byte rle_code = *input++;
    input += 3;
    int dst = 0;
    while (dst < unpacked_size)
    {
        int v = *input++;
        if (-1 == v)
            break;
        if (rle_code == v)
        {
            byte count = *input++;
            byte x = rle_code;
            if (count > 2)
                x = *input++;

            while (count --> 0)
                output[dst++] = x;
        }
        else
        {
            output[dst++] = (byte)v;
        }
    }
    return dst;
}

int tail_crp0_unpack(unsigned char *input, unsigned char *output, int unpacked_size) {
    int dst = 0;
    int i;
    int offset;
    while (dst < unpacked_size)
    {
        int cmd = *input++;
        int count = 0;
        switch (cmd)
        {
        case 0:
            count = *input++;
            memcpy(output + dst, input, count); input += count;
            break;
        case 1:
            count = *(u32*)input; input += sizeof(u32);
            memcpy(output + dst, input, count); input += count;
            break;
        case 2:
            {
                count = *input++;
                byte v = *input++;
                for (i = 0; i < count; ++i)
                    output[dst+i] = v;
                break;
            }
        case 3:
            {
                count = *(u32*)input; input += sizeof(u32);
                byte v = *input++;
                for (i = 0; i < count; ++i)
                    output[dst+i] = v;
                break;
            }
        case 6:
            offset = *(u16*)input; input += sizeof(u16);
            count = *(u16*)input; input += sizeof(u16);
            Binary_CopyOverlapped (output, dst-offset, dst, count);
            break;

        case 15:
        case -1:
            return dst;
        }
        dst += count;
    }
    return dst;
}

int tail_hp_unpack(unsigned char *input, unsigned char *output, int unpacked_size) {
    int root_token = *(u32*)input; input += sizeof(u32);
    int node_count = *(u32*)input; input += sizeof(u32);
    int packed_count = *(u32*)input; input += sizeof(u32);
    int tree_nodes[0x400];
    node_count += root_token - 0xFF;
    while (node_count --> 0)
    {
        int node = 2 * *(u32*)input; input += sizeof(u32);
        tree_nodes[node    ] = *(u32*)input; input += sizeof(u32);
        tree_nodes[node + 1] = *(u32*)input; input += sizeof(u32);
    }
    int dst = 0;
    byte bits = 0;
    byte bit_mask = 0;
    int i;
    for (i = 0; i < packed_count; ++i)
    {
        int symbol = root_token;
        do
        {
            if (0 == bit_mask)
            {
                bits = *input++;
                bit_mask = 128;
            }
            int node = 2 * symbol;
            node += ((bits & bit_mask) != 0) ? 1 : 0;
            symbol = tree_nodes[node];
            bit_mask >>= 1;
        }
        while (tree_nodes[2 * symbol] != -1);
        output[dst++] = (byte)symbol;
    }
    return dst;
}



// GARBro ArcTactics
int tactics_arc_unpack(unsigned char *input, unsigned char *output, int unpacked_size) {
    static ushort LzRefTable[] = {
        0x0001, 0x0804, 0x1001, 0x2001, 0x0002, 0x0805, 0x1002, 0x2002, 
        0x0003, 0x0806, 0x1003, 0x2003, 0x0004, 0x0807, 0x1004, 0x2004, 
        0x0005, 0x0808, 0x1005, 0x2005, 0x0006, 0x0809, 0x1006, 0x2006, 
        0x0007, 0x080A, 0x1007, 0x2007, 0x0008, 0x080B, 0x1008, 0x2008, 
        0x0009, 0x0904, 0x1009, 0x2009, 0x000A, 0x0905, 0x100A, 0x200A, 
        0x000B, 0x0906, 0x100B, 0x200B, 0x000C, 0x0907, 0x100C, 0x200C, 
        0x000D, 0x0908, 0x100D, 0x200D, 0x000E, 0x0909, 0x100E, 0x200E, 
        0x000F, 0x090A, 0x100F, 0x200F, 0x0010, 0x090B, 0x1010, 0x2010, 
        0x0011, 0x0A04, 0x1011, 0x2011, 0x0012, 0x0A05, 0x1012, 0x2012, 
        0x0013, 0x0A06, 0x1013, 0x2013, 0x0014, 0x0A07, 0x1014, 0x2014, 
        0x0015, 0x0A08, 0x1015, 0x2015, 0x0016, 0x0A09, 0x1016, 0x2016, 
        0x0017, 0x0A0A, 0x1017, 0x2017, 0x0018, 0x0A0B, 0x1018, 0x2018, 
        0x0019, 0x0B04, 0x1019, 0x2019, 0x001A, 0x0B05, 0x101A, 0x201A, 
        0x001B, 0x0B06, 0x101B, 0x201B, 0x001C, 0x0B07, 0x101C, 0x201C, 
        0x001D, 0x0B08, 0x101D, 0x201D, 0x001E, 0x0B09, 0x101E, 0x201E, 
        0x001F, 0x0B0A, 0x101F, 0x201F, 0x0020, 0x0B0B, 0x1020, 0x2020, 
        0x0021, 0x0C04, 0x1021, 0x2021, 0x0022, 0x0C05, 0x1022, 0x2022, 
        0x0023, 0x0C06, 0x1023, 0x2023, 0x0024, 0x0C07, 0x1024, 0x2024, 
        0x0025, 0x0C08, 0x1025, 0x2025, 0x0026, 0x0C09, 0x1026, 0x2026, 
        0x0027, 0x0C0A, 0x1027, 0x2027, 0x0028, 0x0C0B, 0x1028, 0x2028, 
        0x0029, 0x0D04, 0x1029, 0x2029, 0x002A, 0x0D05, 0x102A, 0x202A, 
        0x002B, 0x0D06, 0x102B, 0x202B, 0x002C, 0x0D07, 0x102C, 0x202C, 
        0x002D, 0x0D08, 0x102D, 0x202D, 0x002E, 0x0D09, 0x102E, 0x202E, 
        0x002F, 0x0D0A, 0x102F, 0x202F, 0x0030, 0x0D0B, 0x1030, 0x2030, 
        0x0031, 0x0E04, 0x1031, 0x2031, 0x0032, 0x0E05, 0x1032, 0x2032, 
        0x0033, 0x0E06, 0x1033, 0x2033, 0x0034, 0x0E07, 0x1034, 0x2034, 
        0x0035, 0x0E08, 0x1035, 0x2035, 0x0036, 0x0E09, 0x1036, 0x2036, 
        0x0037, 0x0E0A, 0x1037, 0x2037, 0x0038, 0x0E0B, 0x1038, 0x2038, 
        0x0039, 0x0F04, 0x1039, 0x2039, 0x003A, 0x0F05, 0x103A, 0x203A, 
        0x003B, 0x0F06, 0x103B, 0x203B, 0x003C, 0x0F07, 0x103C, 0x203C, 
        0x0801, 0x0F08, 0x103D, 0x203D, 0x1001, 0x0F09, 0x103E, 0x203E, 
        0x1801, 0x0F0A, 0x103F, 0x203F, 0x2001, 0x0F0B, 0x1040, 0x2040, 
    };

    int src = 0;
    int i = 0;
    byte b;
    /*
    int unpacked_size = 0;
    do
    {
        b = input[src++];
        unpacked_size |= (b & 0x7F) << i;
        i += 7;
    }
    while (b >= 0x80);
    if (unpacked_size <= 0)
        throw new InvalidEncryptionScheme();
    */
    int dst = 0;
    while (dst < unpacked_size)
    {
        b = input[src++];
        if (0 != (b & 3))
        {
            int offset_length = (LzRefTable[b] >> 8) & ~7;
            int offset = 0;
            for (i = 0; i < offset_length; i += 8)
                offset |= input[src++] << i;
            offset += LzRefTable[b] & 0x700;

            int count = LzRefTable[b] & 0xFF;
            Binary_CopyOverlapped (output, dst - offset, dst, count);
            dst += count;
        }
        else
        {
            int count = (b >> 2) + 1;
            if (count >= 0x3D)
            {
                int count_length = (count - 0x3C) * 8;
                count = 0;
                for (i = 0; i < count_length; i += 8)
                    count |= input[src++] << i;
                count++;        
            }
            memcpy (output + dst, input + src, count);
            src += count;
            dst += count;
        }
    }
    return dst;
}



// GARBro ArcPKZ
int sviu_pkz_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    byte frame[0x800]="";
    int frame_pos = 0x7E8;
    int ctl = 1;
    int dst = 0;
    while (dst < output_Length)
    {
        if (1 == ctl)
        {
            ctl = *input++;
            if (-1 == ctl)
                break;
            ctl |= 0x100;
        }
        if (0 != (ctl & 1))
        {
            byte b = *input++;
            output[dst++] = b;
            frame[frame_pos++ & 0x7FF] = b;
        }
        else
        {
            byte lo = *input++;
            byte hi = *input++;
            int offset = lo | (hi & 0xE0) << 3;
            int count = (hi & 0x1F) + 2;
            int i;
            for (i = 0; i < count; ++i)
            {
                byte b = frame[(offset + i) & 0x7FF];
                output[dst++] = b;
                frame[frame_pos++ & 0x7FF] = b;
            }
        }
        ctl >>= 1;
    }
    return dst;
}



// GARBro ArcGPC
int nekox_gpc_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    int dst = 0;
    while (dst < output_Length)
    {
        int ctl = *input++;
        if (-1 == ctl)
            break;
        int count, offset;
        if (ctl >= 0x20)
        {
            if (ctl >= 0x80)
            {
                count = (ctl >> 5) & 3;
                offset = (ctl & 0x1F) << 8;
                offset |= *input++;
            }
            else if ((ctl & 0x60) == 0x20)
            {
                offset = (ctl >> 2) & 7;
                count = ctl & 3;
            }
            else if ((ctl & 0x60) == 0x40)
            {
                offset = (ctl & 0x1F) << 8;
                offset |= *input++;
                count = *input++ + 4;
            }
            else
            {
                offset = (ctl & 0x1F) << 8 | *input++;
                count  = *input++ << 24;
                count |= *input++ << 16;
                count |= *input++ << 8;
                count |= *input++;
            }
            count = MIN (count + 3, output_Length-dst);
            Binary_CopyOverlapped (output, dst-offset-1, dst, count);
        }
        else
        {
            if (ctl < 0x1D)
            {
                count = ctl + 1;
            }
            else if (0x1D == ctl)
            {
                count = *input++ + 0x1E;
            }
            else if (0x1E == ctl)
            {
                count  = *input++ << 8;
                count |= *input++;
                count += 286;
            }
            else
            {
                count  = *input++ << 24;
                count |= *input++ << 16;
                count |= *input++ << 8;
                count |= *input++;
            }
            count = MIN (count, output_Length-dst);
            memcpy(output + dst, input, count); input += count;
        }
        dst += count;
    }
    return dst;
}



// GARBro ArcARC
int rec_arc_unpack(unsigned char *bits, unsigned char *output, int output_Length) {
    u8  bitchr=0,bitpos=0;

    int dst = 0;
    while (dst < output_Length)
    {
        int ctl = UNZBITS(bits, 1);
        if (-1 == ctl)
            break;
        if (0 == ctl)
        {
            output[dst++] = (byte)UNZBITS(bits,8);
        }
        else
        {
            int offset = UNZBITS(bits,8);
            int count = UNZBITS(bits,8);
            if (offset <= 0)
                break;
            Binary_CopyOverlapped (output, dst - offset, dst, count);
            dst += count;
        }
    }
    return dst;
}



// GARBro ArcWarcEncryption
int warc_unpack(unsigned char *m_input, int m_input_Length, unsigned char *output) {
#if defined(__GNUC__) && !defined(__clang__)
    int     m_src   = 0;
    uint    m_bits;
    int     m_bits_count;

    void FillBitCache ()
    {
        m_bits = *(u32*)(m_input + m_src);
        m_src += 4;
        m_bits_count = 32;
    }

    int GetBit ()
    {
        uint v = m_bits >> --m_bits_count;
        if (m_bits_count <= 0)
        {
            FillBitCache();
        }
        return (int)(v & 1);
    }

    int dst = 0;
    FillBitCache();
    while (m_src < m_input_Length)
    {
        if (GetBit() != 0)
        {
            output[dst++] = m_input[m_src++];
            continue;
        }
        int count, offset;
        if (GetBit() != 0)
        {
            count = *(u16*)(m_input + m_src);
            m_src += 2;
            offset = count >> 3 | -0x2000;
            count &= 7;
            if (count > 0)
            {
                count += 2;
            }
            else
            {
                count = m_input[m_src++];
                if (0 == count)
                    break;
                count += 9;
            }
        }
        else
        {
            count = GetBit() << 1;
            count |= GetBit();
            count += 2;
            offset = m_input[m_src++] | -0x100;
        }
        Binary_CopyOverlapped (output, dst+offset, dst, count);
        dst += count;
    }
    return dst;
#else
    return -1;
#endif
}



// GARBro ArcWarc / ShiinaRio
int warc10_unpack(unsigned char *m_input, unsigned char *output, int unpacked_size) {
#if defined(__GNUC__) && !defined(__clang__)
    int     m_ctl;
    int     m_bit_count;
    int     m_src;

    int GetCtlBit ()
    {
        int bit = m_ctl & 1;
        m_ctl >>= 1;
        if (--m_bit_count <= 0)
        {
            m_ctl = *(u16*)(m_input + m_src);
            m_src += 2;
            m_bit_count = 16;
        }
        return bit;
    }

    m_src = 0;
    m_bit_count = 0;
    GetCtlBit();
    int dst = 0;
    while (dst < unpacked_size)
    {
        if (GetCtlBit() != 0)
        {
            output[dst++] = m_input[m_src++];
        }
        else
        {
            int offset, count;
            if (GetCtlBit() == 0)
            {
                count  = GetCtlBit() << 1;
                count |= GetCtlBit();
                count += 2;
                offset = m_input[m_src++] | -0x100;
            }
            else
            {
                byte lo = m_input[m_src++];
                byte hi = m_input[m_src++];
                offset = lo | (hi & ~7) << 5 | -0x2000;
                count = hi & 7;
                if (0 == count)
                {
                    count = m_input[m_src++];
                    if (0 == count)
                        break;
                    count += 9;
                }
                else
                {
                    count += 2;
                }
            }
            Binary_CopyOverlapped (output, dst + offset, dst, count);
            dst += count;
        }
    }
    return dst;
#else
    return -1;
#endif
}



// GARBro ArcWARC
int warc_ylz_unpack(unsigned char *m_input, unsigned char *m_output, int m_output_Length) {
#if defined(__GNUC__) && !defined(__clang__)
    int     m_src;
    uint    m_ctl = 0;
    uint    m_mask = 0;

    bool GetCtlBit ()
    {
        bool bit = 0 != (m_ctl & m_mask);
        m_mask >>= 1;
        if (0 == m_mask)
        {
            m_ctl = *(u32*)(m_input + m_src);
            m_src += 4;
            m_mask = 0x80000000;
        }
        return bit;
    }

    int GetBits (int n)
    {
        int v = 0;
        int i;
        for (i = 0; i < n; ++i)
        {
            v <<= 1;
            if (GetCtlBit())
                v |= 1;
        }
        return v;
    }

    GetCtlBit();
    int dst = 0;
    while (dst < m_output_Length)
    {
        if (GetCtlBit())
        {
            m_output[dst++] = m_input[m_src++];
            continue;
        }
        bool next_bit = GetCtlBit();
        int offset = m_input[m_src++] | ~0xffff;
        int ah = 0xff;
        int count = 0;
        if (next_bit) // 5e
        {
            if (GetCtlBit()) // 10d
            {
                ah = (ah << 1) | GetBits (1);
            }
            else if (GetCtlBit()) // 13e
            {
                ah = (ah << 1) | GetBits (1);
                offset -= 0x200;
            }
            else if (GetCtlBit()) // 174
            {
                ah = (ah << 2) | GetBits (2);
                offset -= 0x400;
            }
            else if (GetCtlBit()) // 1c0
            {
                ah = (ah << 3) | GetBits (3);
                offset -= 0x800;
            }
            else
            {
                ah = (ah << 4) | GetBits (4);
                offset -= 0x1000;
            }

            if (GetCtlBit()) // 296
            {
                count = 3;
            }
            else if (GetCtlBit()) // 2a2
            {
                count = 4;
            }
            else if (GetCtlBit()) // 2c2
            {
                count = 5 + GetBits (1);
            }
            else if (GetCtlBit()) // 2f8
            {
                count = 7 + GetBits (2);
            }
            else if (GetCtlBit()) // 33f
            {
                count = 0x0b + GetBits (3);
            }
            else
            {
                count = 0x13 + m_input[m_src++];
            }
        }
        else if (GetCtlBit()) // 94
        {
            ah <<= 3; // b2
            ah |= GetBits (3);
            ah = (ah - 1) & 0xff;
            count = 2;
        }
        else if (0xff == (offset & 0xff))
        {
            return dst;
        }
        else
        {
            count = 2;
        }
        offset += (ah & 0xff) << 8; // 280
        Binary_CopyOverlapped (m_output, dst + offset, dst, count);
        dst += count;
    }
    return dst;
#else
    return -1;
#endif
}



// GARBro ArcWARC
int warc_huff_unpack(unsigned char *m_src, int m_total, unsigned char *m_dst, int m_dst_Length) {
#if defined(__GNUC__) && !defined(__clang__)
    ushort m_tree[2][511] = {{0}};

    int m_input_pos;
    int m_remaining;
    int m_curbits;
    uint m_cache;
    ushort m_curindex;

    uint ReadUInt32 ()
    {
        uint v;
        if (m_remaining >= 4)
        {
            v = *(u32*)(m_src + m_input_pos);
            m_input_pos += 4;
            m_remaining -= 4;
        }
        else if (m_remaining > 0)
        {
            v = m_src[m_input_pos++];
            int shift = 8;
            while (--m_remaining != 0)
            {
                v |= (uint)(m_src[m_input_pos++] << shift);
                shift += 8;
            }
        }
        else
            return 0; //throw new InvalidFormatException ("Unexpected end of file");
        return v;
    }

    uint GetBits (int req_bits)
    {
        uint ret_val = 0;
        if (req_bits > m_curbits)
        {
            req_bits -= m_curbits;
            ret_val |= (m_cache & ((1u << m_curbits) - 1u)) << req_bits;
            m_cache = ReadUInt32();
            m_curbits = 32;
        }
        m_curbits -= req_bits;
        return ret_val | ((1u << req_bits) - 1u) & (m_cache >> m_curbits);
    }

    ushort CreateTree ()
    {
        ushort i;
        if (0 != GetBits (1))
        {
            i = m_curindex++;
            m_tree[0][i] = CreateTree();
            m_tree[1][i] = CreateTree();
        }
        else
            i = (ushort)GetBits (8);
        return i;
    }

    m_input_pos = 0;
    m_remaining = m_total;
    m_curbits = 0;
    m_curindex = 256;
    ushort root = CreateTree();
    int i;
    for (i = 0; i < m_dst_Length; ++i)
    {
        ushort symbol = root;
        while (symbol >= 256)
        {
            symbol = m_tree[GetBits(1)][symbol];
        }
        m_dst[i] = (byte)symbol;
    }
    return i;
#else
    return -1;
#endif
}



// GARBro ArcHXP
int sh_him_unpack(unsigned char *m_input, unsigned char *output, int output_Length) {
    int dst = 0;
    while (dst < output_Length)
    {
        int count;
        int ctl = *m_input++;
        if (ctl < 32)
        {
            switch (ctl)
            {
            case 0x1D:
                count = *m_input++ + 0x1E;
                break;
            case 0x1E:
                count = QUICK_GETb16(m_input) + 0x11E;
                m_input += 2;
                break;
            case 0x1F:
                count = QUICK_GETb32(m_input);
                m_input += 4;
                break;
            default:
                count = ctl + 1;
                break;
            }
            count = MIN (count, output_Length - dst);
            memcpy(output + dst, m_input, count); m_input += count;
        }
        else
        {
            int offset;
            if (0 == (ctl & 0x80))
            {
                if (0x20 == (ctl & 0x60))
                {
                    offset = (ctl >> 2) & 7;
                    count = ctl & 3;
                }
                else
                {
                    offset = *m_input++;
                    if (0x40 == (ctl & 0x60))
                        count = (ctl & 0x1F) + 4;
                    else
                    {
                        offset |= (ctl & 0x1F) << 8;
                        ctl = *m_input++;
                        if (0xFE == ctl)
                            { count = QUICK_GETb16(m_input) + 0x102; m_input += 2; }
                        else if (0xFF == ctl)
                            { count = QUICK_GETb32(m_input); m_input += 4; }
                        else 
                            count = ctl + 4;
                    }
                }
            }
            else
            {
                count = (ctl >> 5) & 3;
                offset = ((ctl & 0x1F) << 8) | *m_input++;
            }
            count += 3;
            offset++;
            count = MIN (count, output_Length-dst);
            Binary_CopyOverlapped (output, dst-offset, dst, count);
        }
        dst += count;
    }
    return dst;
}



// GARBro ArcPBX
int pandora_pbx_unpack(unsigned char *m_input, unsigned char *m_output, int m_output_Length) {
    int dst = 0;
    m_output[dst++] = *m_input++;
    while (dst < m_output_Length)
    {
        int ctl = *m_input++;
        int count;
        if (ctl >= 0x80)
        {
            if (ctl >= 0xC0)
            {
                int offset = *m_input++;
                offset += 0x101 + ((ctl & 0x3F) << 8);
                offset = dst - offset;
                if (offset < 0)
                    return -1; //throw new InvalidFormatException();
                m_output[dst++] = m_output[offset++];
                m_output[dst++] = m_output[offset++];
                m_output[dst++] = m_output[offset++];
            }
            else
            {
                if (ctl >= 0xB0)
                {
                    count = QUICK_GETb16(m_input); m_input += 2;
                    count += 0x813 + ((ctl & 7) << 16);
                    ctl &= 8;
                }
                else if (ctl >= 0xA0)
                {
                    count = *m_input++ + ((ctl & 7) << 8) + 19;
                    ctl &= 8;
                }
                else
                {
                    count = (ctl & 0xF) + 3;
                    ctl &= 0x10;
                }
                int offset;
                if (ctl != 0)
                {
                    offset = QUICK_GETb16(m_input) + 0x101; m_input += 2;
                }
                else
                {
                    offset = *m_input++ + 1;
                }
                Binary_CopyOverlapped (m_output, dst - offset, dst, count);
                dst += count;
            }
        }
        else
        {
            if (ctl >= 0x60)
            {
                count = QUICK_GETb16(m_input); m_input += 2;
                count += 0x2041 + ((ctl & 0x1F) << 16);
            }
            if (ctl >= 0x40)
            {
                count = *m_input++ + ((ctl & 0x1F) << 8) + 0x41;
            }
            else
            {
                count = ctl + 1;
            }
            memcpy(m_output + dst, m_input, count); m_input += count;
            dst += count;
        }
    }
    return dst;
}



// GARBro ArcDAT
int origin_lz_unpack(unsigned char *m_input, unsigned char *output, int output_Length) {
    struct
    {
        int Offset;
        int Length;
    } tree[0x10000] = {{0}};
    int node_count = 0;
    int dst = 0;
    int ctl = 1;
    while (dst < output_Length)
    {
        if (1 == ctl)
            ctl = *m_input++ | 0x100;
        int count = 0;
        if ((ctl & 1) != 0)
        {
            int index;
            if (node_count < 0x100)
                index = *m_input++;
            else
                { index = *(u16*)m_input; m_input += sizeof(u16); }

            count = tree[index].Length;
            mymemmove(output + dst, output + tree[index].Offset, count);
        }
        int symbol = *m_input++;
        if (-1 == symbol)
            break;
        output[dst + count++] = (byte)symbol;
        if (node_count < (sizeof(tree)/sizeof(tree[0])))
        {
            tree[node_count].Offset = dst;
            tree[node_count].Length = count;
            ++node_count;
        }
        dst += count;
        ctl >>= 1;
    }
    return dst;
}

int origin_huffman_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    u8  bitchr=0,bitpos=0;

    u8  m_symbol_table[0x100];
    int i;
    for(i = 0; i < 0x100; i++) m_symbol_table[i] = i;   // method 2 only supported

    struct
    {
        int  Parent;
        byte Value;
    } tree[0x10000] = {{0}};
    u8 buf[0x10000]="";
    int node_count = 0;
    int dst = 0;
    while (dst < output_Length)
    {
        int index = -1;
        int bit = UNZBITS(input,1);
        if (-1 == bit)
            break;

        if (bit != 0)
        {
            int node_bits = node_count == (sizeof(tree)/sizeof(tree[0])) ? node_count - 1 : node_count;
            index = UNZBITS(input,1);
            int shift;
            for (shift = 1; node_bits > 1; ++shift)
            {
                index |= UNZBITS(input,1) << shift;
                node_bits >>= 1;
            }
            int chunk_length = 0;
            int curr_index = index;
            do {
                buf[chunk_length++] = tree[curr_index].Value;
                curr_index          = tree[curr_index].Parent;
            }
            while (curr_index != -1);

            while (chunk_length --> 0)
                output[dst++] = buf[chunk_length];
        }

        int bits_count = 2;
        while (UNZBITS(input,1) > 0)
            bits_count++;

        int c = 0;
        while (bits_count --> 0)
            c |= UNZBITS(input,1) << bits_count;
        if (c < 0)
            break;

        byte symbol = m_symbol_table[c];
        output[dst++] = symbol;

        if (node_count < (sizeof(tree)/sizeof(tree[0])))
        {
            tree[node_count].Parent = index;
            tree[node_count].Value  = symbol;
            node_count++;
        }
    }
    return dst;
}

int origin_rle_unpack(unsigned char *m_input, unsigned char *output, int output_Length) {
    int dst = 0;
    while (dst < output_Length)
    {
        byte symbol = *m_input++;
        int count = MIN (*m_input++, output_Length - dst);
        while (count --> 0)
            output[dst++] = symbol;
    }
    return dst;
}

int origin_alphav2_unpack(unsigned char *m_input, unsigned char *output, int output_Length) {
    int dst = 0;
    u8 table[4];
    memcpy(table + 1, m_input, 3); m_input += 3;
    int ctl = 1;
    byte prev = 0;
    while (dst < output_Length)
    {
        if (1 == ctl)
            ctl = *m_input++ | 0x100;

        int i = ctl & 3;
        byte diff;
        if (0 == i)
            diff = *m_input++;
        else
            diff = table[i];
        prev -= diff;
        output[dst++] = prev;
        ctl >>= 2;
    }
    return dst;
}



int garbro_huffman_unpack(unsigned char *m_input, unsigned char *m_buffer, int m_length) {
#if defined(__GNUC__) && !defined(__clang__)
    u8  bitchr=0,bitpos=0;

    const int TreeSize = 512;

    ushort lhs[TreeSize];
    ushort rhs[TreeSize];
    ushort m_token = 256;

    ushort CreateTree ()
    {
        int bit = UNZBITS(m_input,1);
        if (-1 == bit)
        {
            return 0; //throw new EndOfStreamException ("Unexpected end of the Huffman-compressed stream.");
        }
        else if (bit != 0)
        {
            ushort v = m_token++;
            if (v >= TreeSize)
                return 0; //throw new InvalidFormatException ("Invalid Huffman-compressed stream.");
            lhs[v] = CreateTree();
            rhs[v] = CreateTree();
            return v;
        }
        else
        {
            return (ushort)UNZBITS(m_input,8);
        }
    }

    int m_pos = 0;
    m_token = 256;
    ushort root = CreateTree();
    for (;;)
    {
        ushort symbol = root;
        while (symbol >= 0x100)
        {
            int bit = UNZBITS(m_input,1);
            if (-1 == bit)
                break;
            if (bit != 0)
                symbol = rhs[symbol];
            else
                symbol = lhs[symbol];
        }
        m_buffer[m_pos++] = (byte)symbol;
        if (0 == --m_length)
            return m_pos;
    }
    return m_pos;
#else
    return -1;
#endif
}



int ankh_grp_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    int offsets[4];
    offsets[0] = *(u16*)input; input += sizeof(u16);
    offsets[1] = offsets[0] * 2;
    offsets[2] = offsets[0] * 3;
    offsets[3] = offsets[0] * 4;
    int dst = 0;
    while (dst < output_Length)
    {
        byte ctl = *input++;
        if (0 == ctl)
            break;
        int count;
        if (ctl < 0x40)
        {
            count = MIN (ctl, output_Length - dst);
            memcpy(output + dst, input, count); input += count;
            dst += count;
        }
        else if (ctl <= 0x6F)
        {
            if (0x6F == ctl)
                { count = *(u16*)input; input += sizeof(u16); }
            else
                count = ctl - 0x3D;
            byte v = *input++;
            while (count --> 0)
                output[dst++] = v;
        }
        else if (ctl <= 0x9F)
        {
            if (ctl == 0x9F)
                { count = *(u16*)input; input += sizeof(u16); }
            else 
                count = ctl - 0x6E;
            byte v1 = *input++;
            byte v2 = *input++;
            while (count --> 0)
            {
                output[dst++] = v1;
                output[dst++] = v2;
            }
        }
        else if (ctl <= 0xBF)
        {
            if (ctl == 0xBF)
                { count = *(u16*)input; input += sizeof(u16); }
            else
                count = ctl - 0x9E;
            memcpy(output + dst, input, 3); input += 3;
            if (count > 0)
            {
                count *= 3;
                Binary_CopyOverlapped (output, dst, dst+3, count-3);
                dst += count;
            }
        }
        else
        {
            count = (ctl & 0x3F) + 3;
            int offset = *input++;
            offset = (offset & 0x3F) - offsets[offset >> 6];
            Binary_CopyOverlapped (output, dst+offset, dst, count);
            dst += count;
        }
    }
    return dst;
}



int ankh_hdj_unpack(unsigned char *m_input, unsigned char *output, int output_Length, int method) {
#if defined(__GNUC__) && !defined(__clang__)
    if(method < 0) method = 0; //???
    uint                m_bits;
    int                 m_cached_bits;

    void ResetBits ()
    {
        m_cached_bits = 0;
    }

    int GetBits (int count)
    {
        if (0 == m_cached_bits)
        {
            m_bits = *(u32*)m_input; m_input += sizeof(u32);
            m_cached_bits = 32;
        }
        uint val;
        if (m_cached_bits < count)
        {
            uint next_bits = *(u32*)m_input; m_input += sizeof(u32);
            val = (m_bits | (next_bits >> m_cached_bits)) >> (32 - count);
            m_bits = next_bits << (count - m_cached_bits);
            m_cached_bits = 32 - (count - m_cached_bits);
        }
        else
        {
            val = m_bits >> (32 - count);
            m_bits <<= count;
            m_cached_bits -= count;
        }
        return (int)val;
    }

    int GetNextBit ()
    {
        return GetBits (1);
    }

    ResetBits();
    int word_count = 0;
    int byte_count = 0;
    uint next_byte = 0;
    uint next_word = 0;
    int dst = 0;
    while (dst < output_Length)
    {
        if (GetNextBit() != 0)
        {
            int count = 0;
            bool long_count = false;
            int offset;
            if (GetNextBit() != 0)
            {
                if (0 == word_count)
                {
                    next_word = *(u32*)m_input; m_input += sizeof(u32);
                    word_count = 2;
                }
                count = (int)((next_word >> 13) & 7) + 3;
                offset = (int)(next_word | 0xFFFFE000);
                next_word >>= 16;
                --word_count;
                long_count = 10 == count;
            }
            else
            {
                if (method == 0)
                    count = GetBits (2);
                if (0 == byte_count)
                {
                    next_byte = *(u32*)m_input; m_input += sizeof(u32);
                    byte_count = 4;
                }
                if (method != 0)
                    count = GetBits (2);
                count += 2;
                long_count = 5 == count;
                offset = (int)(next_byte | 0xFFFFFF00);
                next_byte >>= 8;
                --byte_count;
            }
            if (long_count)
            {
                int n = 0;
                while (GetNextBit() != 0)
                    ++n;

                if (n != 0)
                    count += GetBits (n) + 1;
            }
            int src = dst + offset;
            if (src < 0 || src >= dst || dst + count > output_Length)
                return false;
            Binary_CopyOverlapped (output, src, dst, count);
            dst += count;
        }
        else
        {
            if (0 == byte_count)
            {
                next_byte = *(u32*)m_input; m_input += sizeof(u32);
                byte_count = 4;
            }
            output[dst++] = (byte)next_byte;
            next_byte >>= 8;
            --byte_count;
        }
    }
    return dst;
#else
    return -1;
#endif
}



int caramelbox_arc3_unpack(unsigned char *bits, unsigned char *output, int unpacked_size) {
#if defined(__GNUC__) && !defined(__clang__)
    u8  bitchr=0,bitpos=0;

    int LzeGetInteger (u8* bits)
    {
        int length = 0;
        int i;
        for (i = 0; i < 16; ++i)
        {
            if (0 != UNZBITS(bits,1))
                break;
            ++length;
        }
        int v = 1 << length;
        if (length > 0)
            v |= UNZBITS(bits,length);
        return v;
    }

    int dst = 0;
    int output_end = dst + unpacked_size;
    while (dst < output_end)
    {
        int count = LzeGetInteger (bits);
        if (-1 == count)
            break;

        while (--count > 0)
        {
            int data = UNZBITS(bits,8);
            if (-1 == data)
                break;

            if (dst < output_end)
                output[dst++] = (byte)data;
        }
        if (count > 0 || dst >= output_end)
            break;

        int offset = LzeGetInteger (bits);
        if (-1 == offset)
            break;
        count = LzeGetInteger (bits);
        if (-1 == count)
            break;

        Binary_CopyOverlapped (output, dst-offset, dst, count);
        dst += count;
    }
    //if (dst < output_end)
    //    throw new EndOfStreamException ("Premature end of compressed stream");
    return dst;
#else
    return -1;
#endif
}



int caramelbox_arc4_unpack(unsigned char *input, int input_size, unsigned char *output) {
    int src = 0;
    int dst = 0;
    int src_end = src + input_size;
    while (src < src_end)
    {
        int ctl = input[src++];
        if (0 == ctl)
            break;
        if (0 != (ctl & 0x80))
        {
            int offset, count;
            if (0 != (ctl & 0x40))
            {
                if (0 != (ctl & 0x20))
                {
                    ctl = (ctl << 8) | input[src++];
                    ctl = (ctl << 8) | input[src++];

                    count = (ctl & 0x3F) + 4;
                    offset = (ctl >> 6) & 0x7FFF;
                }
                else
                {
                    ctl = (ctl << 8) | input[src++];

                    count = (ctl & 7) + 3;
                    offset = (ctl >> 3) & 0x3FF;
                }
            }
            else
            {
                count = (ctl & 3) + 2;
                offset = (ctl >> 2) & 0xF;
            }
            ++offset;
            Binary_CopyOverlapped (output, dst - offset, dst, count);
            dst += count;
        }
        else
        {
            Buffer_BlockCopy (input, src, output, dst, ctl);
            src += ctl;
            dst += ctl;
        }
    }
    return dst;
}



int circus_V1_unpack(unsigned char *bits, unsigned char *output, int output_Length) {
    int i;
    int dst = 0;
    //intput += 4;
    u8  bitchr=0,bitpos=0;
    {
        while (dst < output_Length)
        {
            int count = UNZBITS(bits,4);
            if (0 != UNZBITS(bits,1))
            {
                for (i = 0; i <= count; ++i)
                    output[dst++] = (byte)UNZBITS(bits,8);
            }
            else
            {
                byte b = (byte)UNZBITS(bits,8);
                for (i = 0; i < count; ++i)
                    output[dst++] = b;
            }
        }
    }
    return dst;
}

int circus_V2_unpack(unsigned char *bits, unsigned char *output, int output_Length) {
    //output[output_Length-1] = input[36];
    //intput += 38;
    u8  bitchr=0,bitpos=0;
    int dst = 0;
    {
        for (dst = 0; dst < output_Length; dst += 2)
        {
            if (0 != UNZBITS(bits,1))
            {
                output[dst+1] = (byte)UNZBITS(bits,8);
                output[dst]   = (byte)UNZBITS(bits,8);
            }
            else
            {
                int j;
                for (j = 0; j < 30; j += 2)
                {
                    if (0 != UNZBITS(bits,1))
                        break;
                }
                output[dst]   = bits/*input*/[4 + j];
                output[dst+1] = bits/*input*/[5 + j];
            }
        }
    }
    return dst;
}

int circus_V3_unpack(unsigned char *bits, unsigned char *output, int output_Length) {
    int dst = 0;
    /*
    int src = 4;
    if (output_Length <= 128)
    {
        Buffer_BlockCopy (input, src, output, dst, output_Length);
        return dst;
    }
    Buffer_BlockCopy (input, src, output, dst, 128);
    src += 128;
    dst += 128;

    intput += src;
    */
    u8  bitchr=0,bitpos=0;
    {
        while (dst < output_Length)
        {
            if (0 != UNZBITS(bits,1))
            {
                int offset = UNZBITS(bits,7) + 1;
                int count  = UNZBITS(bits,4) + 2;
                Binary_CopyOverlapped (output, dst - offset, dst, count);
                dst += count;
            }
            else
            {
                output[dst++] = (byte)UNZBITS(bits,8);
            }
        }
    }
    return dst;
}



int cmvs_cpz_unpack(unsigned char *data, int data_Length, unsigned char *output, int output_Length) {
    byte frame[0x800]="";
    int frame_pos = 0x7DF;
    /*
    int unpacked_size = _QUICK_GETi32 (data, 0x28);
    byte output[0x30+unpacked_size]="";
    Buffer.BlockCopy (data, 0, output, 0, 0x30);
    */
    int src = 0;//0x30;
    int dst = 0;//0x30;
    int ctl = 1;
    while (dst < output_Length && src < data_Length)
    {
        if (1 == ctl)
            ctl = data[src++] | 0x100;
        if (0 != (ctl & 1))
        {
            byte b = data[src++];
            output[dst++] = b;
            frame[frame_pos++] = b;
            frame_pos &= 0x7FF;
        }
        else
        {
            int lo = data[src++];
            int hi = data[src++];
            int offset = lo | (hi & 0xE0) << 3;
            int count = (hi & 0x1F) + 2;
            int i;
            for (i = 0; i < count; ++i)
            {
                byte b = frame[(offset + i) & 0x7FF];
                output[dst++] = b;
                frame[frame_pos++] = b;
                frame_pos &= 0x7FF;
            }
        }
        ctl >>= 1;
    }
    return dst;
}



int daisystem_pac_unpack(unsigned char *input, unsigned char *output, int output_Length, int ctl_bytes) {
    if(ctl_bytes < 0) return -1;
    int ctl = 0;
    int src = ctl + ctl_bytes;
    int dst = 0;
    int bits = 2;
    while (dst < output_Length)
    {
        bits >>= 1;
        if (1 == bits)
        {
            bits = input[ctl++] | 0x100;
        }
        if (0 == (bits & 1))
        {
            output[dst++] = input[src++];
        }
        else
        {
            int offset = input[src++];
            int count  = input[src++];
            Binary_CopyOverlapped (output, dst-offset, dst, count);
            dst += count;
        }
    }
    return dst;
}



int ethornell_bgi_unpack(unsigned char *m_input, unsigned char *m_output, int m_dec_count) {
    u8  bitchr=0,bitpos=0;

    //uint      m_dec_count;

    typedef struct
    {
        ushort Code;
        ushort Depth;
    } HuffmanCode;

    typedef struct
    {
        bool IsParent;
        int  Code;
        int  LeftChildIndex;
        int  RightChildIndex;
    } HuffmanNode;

    int i,j,x;

    //Input.Position = 0x20;
    HuffmanCode hcodes[512]  = {{0}};
    HuffmanNode hnodes[1023] = {{0}};

    int leaf_node_count = 0;
    for (i = 0; i < 512; i++)
    {
        int src = *m_input++;
        if (-1 == src)
            return -1; //throw new EndOfStreamException ("Incomplete compressed stream");
        byte depth = (byte)(src /*- UpdateKey()*/);
        if (0 != depth)
        {
            hcodes[leaf_node_count].Depth = depth;
            hcodes[leaf_node_count].Code = i;
            leaf_node_count++;
        }
    }

    /*
        int CompareTo (HuffmanCode my, HuffmanCode other)
        {
            int cmp = (int)my.Depth - (int)other.Depth;
            if (0 == cmp)
                cmp = (int)my.Code - (int)other.Code;
            return cmp;
        }
    */

    //Array.Sort (hcodes, 0, leaf_node_count);
    for(i = 0; i < (leaf_node_count-1); i++) {
        x = i;
        for(j=i+1; j < leaf_node_count; j++) {
            //if(CompareTo(hcodes[i],hcodes[j]) > 0)

            int cmp = (int)hcodes[i].Depth - (int)hcodes[j].Depth;
            if (0 == cmp)
                cmp = (int)hcodes[i].Code - (int)hcodes[j].Code;
            if(cmp > 0)
            {
                x = j;
            }
        }
        if(x != i) {
            HuffmanCode tmp;
            memcpy(&tmp,       &hcodes[i], sizeof(tmp));
            memcpy(&hcodes[i], &hcodes[j], sizeof(tmp));
            memcpy(&hcodes[i], &tmp,       sizeof(tmp));
        }
    }

    //CreateHuffmanTree (hnodes, hcodes, leaf_node_count);
    //return HuffmanDecompress (hnodes, m_dec_count);

    //void CreateHuffmanTree (HuffmanNode* hnodes, HuffmanCode* hcode, int node_count)
    //{
        HuffmanCode* hcode = hcodes;
        int node_count = leaf_node_count;

        int nodes_index[2][512];
        int next_node_index = 1;
        int depth_nodes = 1;
        int depth = 0;
        int child_index = 0;
        nodes_index[0][0] = 0;
        int n/*,i*/;
        for (n = 0; n < node_count; )
        {
            int huffman_nodes_index = child_index;
            child_index ^= 1;

            int depth_existed_nodes = 0;
            while (n < node_count/*hcode.Length*/ && hcode[n].Depth == depth)
            {
                HuffmanNode node = { 0, hcode[n++].Code };
                hnodes[nodes_index[huffman_nodes_index][depth_existed_nodes]] = node;
                depth_existed_nodes++;
            }
            int depth_nodes_to_create = depth_nodes - depth_existed_nodes;
            for (i = 0; i < depth_nodes_to_create; i++)
            {
                HuffmanNode node = { 1 };
                nodes_index[child_index][i * 2]     = node.LeftChildIndex = next_node_index++;
                nodes_index[child_index][i * 2 + 1] = node.RightChildIndex = next_node_index++;
                hnodes[nodes_index[huffman_nodes_index][depth_existed_nodes+i]] = node;
            }
            depth++;
            depth_nodes = depth_nodes_to_create * 2;
        }
    //}

    //int HuffmanDecompress (HuffmanNode* hnodes, uint dec_count)
    //{
        int dst_ptr = 0;
        uint    k;
        for (k = 0; k < m_dec_count; k++)
        {
            int node_index = 0;
            do
            {
                int bit = UNZBITS(m_input,1);
                if (-1 == bit)
                    return -1;//throw new EndOfStreamException();
                if (0 == bit)
                    node_index = hnodes[node_index].LeftChildIndex;
                else
                    node_index = hnodes[node_index].RightChildIndex;
            }
            while (hnodes[node_index].IsParent);

            int code = hnodes[node_index].Code;
            if (code >= 256)
            {
                int offset = UNZBITS(m_input,12);
                if (-1 == offset)
                    break;
                int count = (code & 0xff) + 2;
                offset += 2;			
                Binary_CopyOverlapped (m_output, dst_ptr - offset, dst_ptr, count);
                dst_ptr += count;
            } else
                m_output[dst_ptr++] = (byte)code;
        }
        return dst_ptr;
    //}
}



int fc01_mrg_unpack(unsigned char *m_input, int m_size, unsigned char *m_output, int m_output_Length) {
    int dst = 0;
    byte frame[0x1000]="";
    int frame_pos = 0xfee;
    int frame_mask = 0xfff;
    int remaining = m_size;
    while (remaining > 0)
    {
        int ctl = *m_input++;
        --remaining;
        int bit;
        for (bit = 1; remaining > 0 && bit != 0x100; bit <<= 1)
        {
            if (dst >= m_output_Length)
                return dst;
            if (0 != (ctl & bit))
            {
                byte b = *(u16*)m_input; m_input += sizeof(u16);
                --remaining;
                frame[frame_pos++] = b;
                frame_pos &= frame_mask;
                m_output[dst++] = b;
            }
            else
            {
                if (remaining < 2)
                    return dst;
                int offset = *(u16*)m_input; m_input += sizeof(u16);
                remaining -= 2;
                int count = (offset >> 12) + 3;
                for ( ; count != 0; --count)
                {
                    if (dst >= m_output_Length)
                        break;
                    offset &= frame_mask;
                    byte v = frame[offset++];
                    frame[frame_pos++] = v;
                    frame_pos &= frame_mask;
                    m_output[dst++] = v;
                }
            }
        }
    }
    return dst;
}



int fc01_mrg_quant_unpack(unsigned char *m_input, int m_input_Length, unsigned char *m_output, int m_output_Length) {
    ushort word_10036650[0x200]={0};
    byte byte_table[0xff00]="";
    byte Key = 0;
    int m_src = 0;

    uint d;
    uint quant;// = InitTable();
    //ushort InitTable () // sub_10026E30
    //{
        d = 0;
        int t = 0;
        byte key = Key;
        int i,j;
        for (i = 0; i < 0x100; i++)
        {
            byte c = m_input[m_src++];
            if (0 != Key)
            {
                c = (byte)(/*Binary.RotByteL (c, 1)*/((c<<1|c>>7)&0xff) ^ key);
                key -= (byte)i;
            }
            word_10036650[i*2] = d;
            word_10036650[i*2+1] = c;
            d += c;
            for (j = 0; j < c; ++j)
                byte_table[t++] = (byte)i;
        }
        quant = d; //return d;
    //}

    if (0 == quant || quant > 0x10000)
        return -1; //throw new InvalidFormatException();

    uint mask;// = GetMask (quant);
    //uint GetMask (uint d) // sub_10026DC0
    //{
        d = quant;
        d--;
        d >>= 8;
        uint result = 0xff;
        while (d > 0)
        {
            d >>= 1;
            result = (result << 1) | 1;
        }
        mask = result; //return result;
    //}

    uint scale = 0x10000 / quant;
    uint b = 0;
    uint c = 0xffffffff;
    int dst = 0;
    uint a = _QUICK_GETb32(m_input, m_src); m_input += sizeof(u32);
    m_src += 4;
    while (dst < m_output_Length)
    {
        c = ((c >> 8) * scale) >> 8;
        uint v = (a - b) / c;
        if (v > quant)
            return -1; //throw new InvalidFormatException();
        v = byte_table[v];
        m_output[dst++] = (byte)v;
        b += word_10036650[v*2] * c;
        c *= word_10036650[v*2+1];
        while (0 == (((c + b) ^ b) & 0xFF000000))
        {
            if (m_src >= m_input_Length)
                return dst;
            a <<= 8;
            b <<= 8;
            c <<= 8;
            a |= m_input[m_src++];
        }
        while (c <= mask)
        {
            if (m_src >= m_input_Length)
                return dst;
            c = (~b & mask) << 8;
            a <<= 8;
            b <<= 8;
            a |= m_input[m_src++];
        }
    }
    return dst;
}



int fc01_pak_lz_unpack(unsigned char *m_input, unsigned char *m_buffer, int m_unpacked_size) {
    int m_length = m_unpacked_size;
    u8  bitchr=0,bitpos=0;
    byte frame[0x1000]="";
    int m_pos=0,dst = 0;
    int frame_pos = 1;
    while (dst < m_unpacked_size)
    {
        int bit = UNZBITS(m_input,1);
        if (bit != 0)
        {
            if (-1 == bit)
                break;
            int v = UNZBITS(m_input,8);
            if (-1 == v)
                break;
            frame[frame_pos++ & 0xFFF] = m_buffer[m_pos++] = (byte)v;
            dst++;
            if (0 == --m_length)
                return m_pos;
        }
        else
        {
            int offset = UNZBITS(m_input,12);
            if (-1 == offset)
                break;
            int count = UNZBITS(m_input,4);
            if (-1 == count)
                break;
            count += 2;
            dst += count;
            while (count --> 0)
            {
                byte v = frame[offset++ & 0xFFF];
                frame[frame_pos++ & 0xFFF] = v;
                m_buffer[m_pos++] = v;
                if (0 == --m_length)
                    return m_pos;
            }
        }
    }
    return m_pos;
}



int favorite_lzw_unpack(unsigned char *m_input, unsigned char *m_output, int m_output_Length) {
    u8  bitchr=0,bitpos=0;
    int dst = 0;
    int lzw_dict[0x8900]={0};
    int token_width = 9;
    int dict_pos = 0;
    while (dst < m_output_Length)
    {
        int token = UNZBITS(m_input,token_width);
        if (-1 == token)
            return -1;//throw new EndOfStreamException ("Invalid compressed stream");
        else if (0x100 == token) // end of input
            break;
        else if (0x101 == token) // increase token width
        {
            ++token_width;
            if (token_width > 24)
                return -1;//throw new InvalidFormatException ("Invalid comressed stream");
        }
        else if (0x102 == token) // reset dictionary
        {
            token_width = 9;
            dict_pos = 0;
        }
        else
        {
            if (dict_pos >= (sizeof(lzw_dict)/sizeof(lzw_dict[0])))
                return -1;//throw new InvalidFormatException ("Invalid comressed stream");
            lzw_dict[dict_pos++] = dst;
            if (token < 0x100)
            {
                m_output[dst++] = (byte)token;
            }
            else
            {
                token -= 0x103;
                if (token >= dict_pos)
                    return -1;//throw new InvalidFormatException ("Invalid comressed stream");
                int src = lzw_dict[token];
                int count = MIN (m_output_Length-dst, lzw_dict[token+1] - src + 1);
                if (count < 0)
                    return -1;//throw new InvalidFormatException ("Invalid comressed stream");
                Binary_CopyOverlapped (m_output, src, dst, count);
                dst += count;
            }
        }
    }
    return dst;
}



int frontwing_rle_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    int src = 0;
    int dst = 0;
    while (dst < output_Length)
    {
        byte rle = input[src++];
        int count = MIN (rle & 0x7F, output_Length - dst);
        if (0 != (rle & 0x80))
        {
            byte v = input[src++];
            while (count --> 0)
                output[dst++] = v;
        }
        else
        {
            Buffer_BlockCopy (input, src, output, dst, count);
            src += count;
            dst += count;
        }
    }
    return dst;
}



int frontwing_huffman_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    typedef struct
    {
        int      Weight;
        ushort   LChild;
        ushort   RChild;
    } HuffmanNode;

    HuffmanNode tree[0x201];
    //ushort root = BuildHuffmanTree (tree, input);

    //ushort BuildHuffmanTree (HuffmanNode* tree, u8* input)
    ushort root = 0x100;
    //{
        int i;
        for (i = 0; i < 0x100; ++i)
        {
            tree[i].Weight = *input++ ^ 0x55;
        }
        //ushort root = 0x100;
        tree[root].Weight = 1;
        ushort lhs = 0x200;
        ushort rhs = 0x200;
        for (;;)
        {
            int lmin = 0x10000;
            int rmin = 0x10000;
            for (i = 0; i < 0x201; ++i)
            {
                int w = tree[i].Weight;
                if (w != 0 && w < rmin)
                {
                    rmin = lmin;
                    lmin = w;
                    rhs = lhs;
                    lhs = i;
                }
            }
            if (rmin == 0x10000 || lmin == 0x10000 || lmin == 0 || rmin == 0)
                break;
            ++root;
            tree[root].LChild = lhs;
            tree[root].RChild = rhs;
            tree[root].Weight = rmin + lmin;
            tree[lhs].Weight = 0;
            tree[rhs].Weight = 0;
        }
        //return root;
    //}

    u8  bitchr=0,bitpos=0;
        int dst = 0;
        for (;;)
        {
            ushort symbol = root;
            while (symbol > 0x100)
            {
                int bit = UNZBITS(input,1);
                if (-1 == bit)
                    return dst;
                if (bit != 0)
                    symbol = tree[symbol].RChild;
                else
                    symbol = tree[symbol].LChild;
            }
            if (0x100 == symbol)
                return dst;
            output[dst++] = (byte)symbol;
        }

    return dst;
}



int g2_gcex_unpack(unsigned char *m_input, unsigned char *m_output, int segment_length) {
#if defined(__GNUC__) && !defined(__clang__)
    int m_control_len = *(u32*)m_input; m_input += sizeof(u32);
    if(m_control_len < 0) return -1;
    u8  *m_control = m_input;
    m_input += m_control_len;
    int m_control_pos = 0;
    int m_bit_pos = 0;

    int m_frame[0x10000]={0};

    int GetBit ()
    {
        if (0 == m_bit_pos--)
        {
            ++m_control_pos;
            m_bit_pos = 7;
            --m_control_len;
            if (0 == m_control_len)
                return -1; //throw new EndOfStreamException();
        }
        return 1 & (m_control[m_control_pos] >> m_bit_pos);
    }

    int GetLength ()
    {
        int v = 0;
        if (0 == GetBit())
        {
            int digits = 0;
            while (0 == GetBit())
                ++digits;
            v = 1 << digits;
            while (digits --> 0)
                v |= GetBit() << digits;
        }
        return v;
    }

    int m_dst = 0;
    int frame_pos = 0;
    int dst_end = m_dst + segment_length;
    while (m_dst < dst_end)
    {
        int n = GetLength();
        while (n --> 0)
        {
            m_frame[frame_pos] = m_dst;
            byte b = *m_input++;
            frame_pos = ((frame_pos << 8) | b) & 0xFFFF;
            m_output[m_dst++] = b;
        }
        if (m_dst >= dst_end)
            break;
        n = GetLength() + 1;
        int src = m_frame[frame_pos];
        while (n --> 0)
        {
            m_frame[frame_pos] = m_dst;
            frame_pos = ((frame_pos << 8) | m_output[src]) & 0xFFFF;
            m_output[m_dst++] = m_output[src++];
        }
    }
    return m_dst;
#else
    return -1;
#endif
}



int gss_arc_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    int dst = 0;
    while (dst < output_Length)
    {
        int ctl = *input++;
        if (-1 == ctl)
            break;
        int count;
        if ((ctl & 0xC0) == 0xC0)
        {
            count = ctl & 0xF;
            ctl &= 0xF0;
        }
        else
        {
            count = ctl & 0x3F;
            ctl &= 0xC0;
        }
        switch (ctl)
        {
        case 0xF0: return dst;
        case 0x40:
            memcpy(output + dst, input, count); input += count;
            dst += count;
            break;

        case 0xD0:
            count = count << 8 | *input++;
            memcpy(output + dst, input, count); input += count;
            dst += count;
            break;

        case 0x80:
            {
                byte v = *input++;
                while (count --> 0)
                    output[dst++] = v;
                break;
            }

        case 0xE0:
            {
                count = count << 8 | *input++;
                byte v = *input++;
                while (count --> 0)
                    output[dst++] = v;
                break;
            }

        case 0x00:
            dst += count;
            break;

        case 0xC0:
            count = count << 8 | *input++;
            dst += count;
            break;
        }
    }
    return dst;
}



int hypatia_mariel_unpack(unsigned char *input, unsigned char *dest, int dest_size) {
    int out_pos = 0;
    uint bits = 0;
    while (dest_size > 0)
    {
        bool carry = 0 != (bits & 0x80000000);
        bits <<= 1;
        if (0 == bits)
        {
            bits = *(u32*)input; input += sizeof(u32);
            carry = 0 != (bits & 0x80000000);
            bits = (bits << 1) | 1u;
        }
        int b = *input++;
        if (-1 == b)
            break;
        if (!carry)
        {
            dest[out_pos++] = (byte)b;
            dest_size--;
            continue;
        }
        int offset = (b & 0x0f) + 1;
        int count = ((b >> 4) & 0x0f) + 1;
        if (0x0f == count)
        {
            b = *input++;
            if (-1 == b)
                break;
            count = (byte)b;
        }
        else if (count > 0x0f)
        {
            count = *(u16*)input; input += sizeof(u16);
        }
        if (offset >= 0x0b)
        {
            offset -= 0x0b;
            offset <<= 8;
            offset |= *input++;
        }
        if (count > dest_size)
            count = dest_size;
        int src = out_pos - offset;
        if (src < 0 || src >= out_pos)
            break;
        Binary_CopyOverlapped (dest, src, out_pos, count);
        out_pos += count;
        dest_size -= count;
    }
    return out_pos;
}



int interheart_fpk_unpack(unsigned char *m_input, int m_size, unsigned char *m_output, int m_output_Length) {
    int remaining = m_size;
    int dst = 0;
    while (remaining > 0 && dst < m_output_Length)
    {
        int ctl = *m_input++;
        remaining--;
        int mask;
        for (mask = 0x80; mask != 0 && remaining > 0 && dst < m_output_Length; mask >>= 1)
        {
            if (0 != (ctl & mask))
            {
                if (remaining < 2)
                    return dst;

                int offset = *m_input++;
                int count  = *m_input++;
                remaining -= 2;
                offset |= (count & 0xF0) << 4;
                count   = (count & 0x0F) + 3;

                if (0 == offset)
                    offset = 4096;
                if (dst + count > m_output_Length)
                    count = m_output_Length - dst;

                Binary_CopyOverlapped (m_output, dst-offset, dst, count);
                dst += count;
            }
            else
            {
                m_output[dst++] = *m_input++;
                remaining--;
            }
        }
    }
    return dst;
}



int kaguya_ari_unpack(unsigned char *m_input, unsigned char *m_output, int m_output_Length) {
    u8  bitchr=0,bitpos=0;
    int dst = 0;
    int frame_pos = 1;
    byte frame[4096];
    int frame_mask = sizeof(frame) - 1;

    while (dst < m_output_Length)
    {
        int bit = UNZBITS(m_input,1);
        if (-1 == bit)
            break;
        if (0 != bit)
        {
            int data = UNZBITS(m_input,8);
            m_output[dst++] = (byte)data;
            frame[frame_pos++] = (byte)data;
            frame_pos &= frame_mask;
        }
        else
        {
            int win_offset = UNZBITS(m_input,12);
            if (-1 == win_offset || 0 == win_offset)
                break;

            int count = UNZBITS(m_input,4) + 2;
            int i;
            for (i = 0; i < count; i++)
            {
                byte data = frame[(win_offset + i) & frame_mask];
                m_output[dst++] = data;
                frame[frame_pos++] = data;
                frame_pos &= frame_mask;
            }
        }
    }
    return dst;
}



int kaguya_lin2_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    byte frame[0x100]="";
    int frame_pos = 0xEF;
    int dst = 0;
    int ctl = 0;
    int bit = 0;
    int prev_count = -1;
    while (dst < output_Length)
    {
        bit >>= 1;
        if (0 == bit)
        {
            ctl = *input++;
            if (-1 == ctl)
                break;
            bit = 0x80;
        }
        if (0 != (ctl & bit))
        {
            byte v = *input++;
            frame[frame_pos++ & 0xFF] = v;
            output[dst++] = v;
        }
        else
        {
            int offset = *input++;
            int count;
            if (-1 == prev_count)
            {
                prev_count = *input++;
                count = prev_count & 0xF;
            }
            else
            {
                count = prev_count >> 4;
                prev_count = -1;
            }
            count += 2;
            while (count --> 0 && dst < output_Length)
            {
                byte v = frame[offset++ & 0xFF];
                frame[frame_pos++ & 0xFF] = v;
                output[dst++] = v;
            }
        }
    }
    return dst;
}



int kaguya_link_unpack(unsigned char *m_input, unsigned char *m_output, int m_output_Length, int m_step) {
#if defined(__GNUC__) && !defined(__clang__)
    if(m_step < 0) m_step = 1; //???
    int             m_final_size = m_output_Length;
    int             m_key   = 0;

    /*
    int             m_step;
    public BmrDecoder (IBinaryStream input)
    {
        input.Position = 3;
        m_step = *input++;
        m_final_size = *(u32*)input; input += sizeof(u32);
        m_key = *(u32*)input; input += sizeof(u32);
        int unpacked_size = *(u32*)input; input += sizeof(u32);
        m_output = new byte[unpacked_size];
        m_input = new MsbBitStream (input.AsStream, true);
    }
    */

    byte* DecompressRLE (byte* input, int *ret)
    {
        int result_Length = m_final_size;
        byte *result = malloc(m_final_size);
        int src = 0;
        int i;
        int dst = 0;
        for (i = 0; i < m_step; ++i)
        {
            byte v1 = input[src++];
            result[i] = v1;
            dst = i + m_step;
            while (dst < result_Length)
            {
                byte v2 = input[src++];
                result[dst] = v2;
                dst += m_step;
                if (v2 == v1)
                {
                    int count = input[src++];
                    if (0 != (count & 0x80))
                        count = input[src++] + ((count & 0x7F) << 8) + 128;
                    while (count --> 0 && dst < result_Length)
                    {
                        result[dst] = v2;
                        dst += m_step;
                    }
                    if (dst < result_Length)
                    {
                        v2 = input[src++];
                        result[dst] = v2;
                        dst += m_step;
                    }
                }
                v1 = v2;
            }
        }
        if(ret) *ret = dst;
        return result;
    }


    void UndoMoveToFront ()
    {
        byte dict[256];
        int i, j;
        for (i = 0; i < 256; ++i)
            dict[i] = (byte)i;
        for (i = 0; i < m_output_Length; ++i)
        {
            byte v = m_output[i];
            m_output[i] = dict[v];
            for (j = v; j > 0; --j)
            {
                dict[j] = dict[j-1];
            }
            dict[0] = m_output[i];
        }
    }

    byte* Decode (byte* input, int input_Length, int key)
    {
        int freq_table[256];
        int i;
        for (i = 0; i < input_Length; ++i)
        {
            ++freq_table[input[i]];
        }
        for (i = 1; i < 256; ++i)
        {
            freq_table[i] += freq_table[i-1];
        }
        int *distrib_table = calloc(input_Length, sizeof(int));
        for (i = input_Length-1; i >= 0; --i)
        {
            int v = input[i];
            int freq = --freq_table[v];
            distrib_table[freq] = i;
        }
        int pos = key;
        byte *copy_out = malloc(input_Length);
        for (i = 0; i < input_Length; ++i)
        {
            pos = distrib_table[pos];
            copy_out[i] = input[pos];
        }
        return copy_out;
    }

    ushort      m_token;
    ushort      m_tree[2][256];

    u8  bitchr=0,bitpos=0;

    ushort CreateHuffmanTree ()
    {
        if (0 != UNZBITS(m_input,1))
        {
            ushort v = m_token++;
            m_tree[0][v-256] = CreateHuffmanTree();
            m_tree[1][v-256] = CreateHuffmanTree();
            return v;
        }
        else
        {
            return (ushort)UNZBITS(m_input,8);
        }
    }

    int UnpackHuffman ()
    {
        m_token = 256;
        ushort root = CreateHuffmanTree();
        int dst = 0;
        while (dst < m_output_Length)
        {
            ushort symbol = root;
            while (symbol >= 0x100)
            {
                int bit = UNZBITS(m_input,1);
                if (-1 == bit)
                    return -1;//throw new EndOfStreamException();
                symbol = m_tree[bit][symbol-256];
            }
            m_output[dst++] = (byte)symbol;
        }
        return dst;
    }

    int dst;
    u8  *out;
    //m_input.Input.Position = 0x14;
    dst = UnpackHuffman();
    if(dst < 0) return -1;
    UndoMoveToFront();
    out = Decode (m_output, dst, m_key);
    memcpy(m_output, out, dst);
    FREE(out)

    if (m_step != 0) {
        out = DecompressRLE (m_output, &dst);
        memcpy(m_output, out, dst);
        FREE(out)
    }

    return dst;
#else
    return -1;
#endif
}



int kaguya_uf_unpack(unsigned char *bits, unsigned char *output, int output_Length) {
    u8  bitchr=0,bitpos=0;
    byte frame[0x1000]="";
    int frame_pos = 1;
    int dst = 0;
    {
        while (dst < output_Length)
        {
            if (0 != UNZBITS(bits,1))
            {
                byte b = (byte)UNZBITS(bits,8);
                output[dst++] = b;
                frame[frame_pos++ & 0xFFF] = b;
            }
            else
            {
                int offset = UNZBITS(bits,12);
                int count = UNZBITS(bits,4) + 2;
                int i;
                for (i = 0; i < count; ++i)
                {
                    byte b = frame[(offset + i) & 0xFFF];
                    output[dst++] = b;
                    frame[frame_pos++ & 0xFFF] = b;
                }
            }
        }
    }
    return dst;
}



int kid_dat_unpack(unsigned char *input, unsigned char *output, int unpacked_size) {
    int dst = 0;
    while (dst < unpacked_size)
    {
        int ctl = *input++;
        if (-1 == ctl)
            break;
        if ((ctl & 0x80) != 0)
        {
            if ((ctl & 0x40) != 0)
            {
                int count = (ctl & 0x1F) + 2;
                if ((ctl & 0x20) != 0)
                    count += *input++ << 5;
                count = MIN (count, unpacked_size - dst);
                byte v = *input++;
                int i;
                for (i = 0; i < count; ++i)
                    output[dst++] = v;
            }
            else
            {
                int count = ((ctl >> 2) & 0xF) + 2;
                int offset = ((ctl & 3) << 8) + *input++ + 1;
                count = MIN (count, unpacked_size - dst);
                Binary_CopyOverlapped (output, dst - offset, dst, count);
                dst += count;
            }
        }
        else if ((ctl & 0x40) != 0)
        {
            int length = MIN ((ctl & 0x3F) + 2, unpacked_size - dst);
            int count = *input++;
            memcpy(output + dst, input, length); input += length;
            dst += length;
            count = MIN (count * length, unpacked_size - dst);
            if (count > 0)
            {
                Binary_CopyOverlapped (output, dst - length, dst, count);
                dst += count;
            }
        }
        else
        {
            int count = (ctl & 0x1F) + 1;
            if ((ctl & 0x20) != 0)
                count += *input++ << 5;
            count = MIN (count, unpacked_size - dst);
            memcpy(output + dst, input, count); input += count;
            dst += count;
        }
    }
    return dst;
}



int lambda_lax_unpack(unsigned char *BaseStream, unsigned char *m_buffer, int unpacked_size) {
    byte m_frame[0x1000]="";
    int i;

    for (i = 0; i < sizeof(m_frame); ++i)
        m_frame[i] = 0;
    int frame_pos = 0xFEE;
    int bits = 2;
    int dst = 0;
    while (dst < unpacked_size)
    {
        bits >>= 1;
        if (1 == bits)
        {
            bits = *BaseStream++;
            if (-1 == bits)
                break;
            bits |= 0x100;
        }
        int lo = *BaseStream++;
        if (-1 == lo)
            break;
        if (0 != (bits & 1))
        {
            m_buffer[dst++] = m_frame[frame_pos++ & 0xFFF] = (byte)lo;
        }
        else
        {
            int hi = *BaseStream++;
            if (-1 == hi)
                break;
            int offset = (hi & 0xF0) << 4 | lo;
            int count = MIN (3 + (hi & 0xF), unpacked_size - dst);
            while (count --> 0)
            {
                byte v = m_frame[offset++ & 0xFFF];
                m_buffer[dst++] = m_frame[frame_pos++ & 0xFFF] = v;
            }
        }
    }
    return dst;
}



int microvision_arc_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    byte frame[0x1000]="";
    int frame_pos = 1;
    const int frame_mask = 0xFFF;
    int dst = 0;
    int bit = 0;
    int ctl = 0;
    while (dst < output_Length)
    {
        if (0 == bit)
        {
            ctl = *input++;
            if (-1 == ctl)
                break;
            bit = 0x80;
        }
        if (0 != (ctl & bit))
        {
            byte b = (byte)*input++;
            frame[frame_pos++ & frame_mask] = b;
            output[dst++] = b;
        }
        else
        {
            int lo = *input++;
            int hi = *input++;
            if (-1 == lo || -1 == hi)
                break;
            int offset = hi >> 4 | lo << 4;
            int count;
            for (count = 2 + (hi & 0xF); count > 0 && dst < output_Length; --count)
            {
                byte v = frame[offset++ & frame_mask];
                frame[frame_pos++ & frame_mask] = v;
                output[dst++] = v;
            }
        }
        bit >>= 1;
    }
    return dst;
}



int moonhir_fpk_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    int exctl;
    int dst = 0;
    int ctl = 1;
    while (dst < output_Length)
    {
        if (1 == ctl)
        {
            ctl = *input++;
            if (-1 == ctl)
                break;
            ctl |= 0x100;
        }
        int count, offset;
        switch (ctl & 3)
        {
        case 0:
            output[dst++] = (byte)*input++;
            break;
        case 1:
            count = *input++;
            if (-1 == count)
                return dst;
            count = MIN (count + 2, output_Length - dst);
            memcpy(output + dst, input, count); input += count;
            dst += count;
            break;
        case 2:
            offset  = *input++ << 8;
            offset |= *input++;
            if (-1 == offset)
                return dst;
            count = MIN ((offset & 0x1F) + 4, output_Length - dst);
            offset >>= 5;
            Binary_CopyOverlapped (output, dst - offset - 1, dst, count);
            dst += count;
            break;
        case 3:
            exctl = *input++;
            if (-1 == exctl)
                return dst;
            count = exctl & 0x3F;
            switch (exctl >> 6)
            {
            case 0:
                count = count << 8 | *input++;
                if (-1 == count)
                    return dst;
                count = MIN (count + 0x102, output_Length - dst);
                memcpy(output + dst, input, count); input += count;
                dst += count;
                break;
            case 1:
                offset  = *input++ << 8;
                offset |= *input++;
                count = count << 5 | offset & 0x1F;
                count = MIN (count + 0x24, output_Length - dst);
                offset >>= 5;
                Binary_CopyOverlapped (output, dst - offset - 1, dst, count);
                dst += count;
                break;
            case 3:
                input += count;
                ctl = 1 << 2;
                break;
            default:
                break;
            }
            break;
        }
        ctl >>= 2;
    }
    return dst;
}



int spack_unpack(unsigned char *m_input, int m_packed_size, unsigned char *m_output, int m_output_Length) {
    int dst = 0;
    uint src = 0;
    uint ctl = 0;
    uint mask = 0;

    while (dst < m_output_Length && src < m_packed_size)
    {
        if (0 == mask)
        {
            ctl = *(u32*)m_input; m_input += sizeof(u32);
            src += 4;
            mask = 0x80000000;
        }
        if (0 != (ctl & mask))
        {
            int copy_count, offset;

            offset = *m_input++;
            src++;
            copy_count = offset >> 4;
            offset &= 0x0f;
            if (15 == copy_count)
            {
                copy_count = *(u16*)m_input; m_input += sizeof(u16);
                src += 2;
            }
            else if (14 == copy_count)
            {
                copy_count = *m_input++;
                src++;
            }
            else
                copy_count++;

            if (offset < 10)
                offset++;
            else
            {
                offset = ((offset - 10) << 8) | *m_input++;
                src++;
            }

            if (dst + copy_count > m_output_Length)
                copy_count = m_output_Length - dst;
            Binary_CopyOverlapped (m_output, dst-offset, dst, copy_count);
            dst += copy_count;
        }
        else
        {
            m_output[dst++] = *m_input++;
            src++;
        }
        mask >>= 1;
    }
    return dst;
}



int azsys_unpack(unsigned char *m_input, unsigned char *m_output, int m_output_Length) {
    int m_control_len = _QUICK_GETi32 (m_input, 4);
    int m_compr1_len = _QUICK_GETi32 (m_input, 8);
    int m_compr2_len = _QUICK_GETi32 (m_input, 12);
    int m_output_len = _QUICK_GETi32 (m_input, 0x10);

    int control = 0x14;
    int compr1 = control + m_control_len;
    int compr2 = compr1 + m_compr1_len;
    int dst = 0;
    byte mask = 0x80;
    int copy_count;
    while (dst < m_output_Length)
    {
        if (0 != (m_input[control] & mask))
        {
            int offset = _QUICK_GETi16(m_input, compr1);
            compr1 += 2;
            copy_count = (offset >> 13) + 3;
            offset &= 0x1fff;
            offset++;
            Binary_CopyOverlapped (m_output, dst-offset, dst, copy_count);
            dst += copy_count;
        }
        else
        {
            copy_count = m_input[compr2++] + 1;
            Buffer_BlockCopy (m_input, compr2, m_output, dst, copy_count);
            compr2 += copy_count;
            dst += copy_count;
        }
        mask >>= 1;
        if (0 == mask)
        {
            ++control;
            mask = 0x80;
        }
    }
    return dst;
}



int dxlib_unpack(unsigned char *input, int remaining, unsigned char *output) {  // unwolf?
    byte control_code = *input++;
    int dst = 0;
    while (remaining > 0)
    {
        byte b = *input++;
        --remaining;
        if (b != control_code)
        {
            output[dst++] = b;
            continue;
        }
        b = *input++;
        --remaining;
        if (b == control_code)
        {
            output[dst++] = b;
            continue;
        }
        if (b > control_code)
            --b;
        int count = b >> 3;
        if (0 != (b & 4))
        {
            count |= *input++ << 5;
            --remaining;
        }
        count += 4;
        int offset;
        switch (b & 3)
        {
        case 0:
            offset = *input++;
            --remaining;
            break;

        case 1:
            offset = *(u16*)input; input += sizeof(u16);
            remaining -= 2;
            break;

        case 2:
            offset = *(u16*)input; input += sizeof(u16);
            offset |= *input++ << 16;
            remaining -= 3;
            break;

        default:
            return -1; //throw new InvalidFormatException ("DX decompression failed");
        }
        ++offset;
        Binary_CopyOverlapped (output, dst - offset, dst, count);
        dst += count;
    }
    return dst;
}



int glibg_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    const int frame_mask = 0xFFF;
    byte frame[0x1000]="";
    int frame_pos = 0xFEE;
    int dst = 0;
    int ctl = 2;
    while (dst < output_Length)
    {
        ctl >>= 1;
        if (1 == ctl)
        {
            ctl = *input++;
            if (-1 == ctl)
                break;
            ctl |= 0x100;
        }
        if (0 != (ctl & 1))
        {
            int b = *input++;
            if (-1 == b)
                break;
            output[dst++] = frame[frame_pos++ & frame_mask] = (byte)b;
        }
        else
        {
            int lo = *input++;
            if (-1 == lo)
                break;
            int hi = *input++;
            if (-1 == hi)
                break;
            int offset = (hi & 0xf0) << 4 | lo;
            int count = MIN ((~hi & 0xF) + 3, output_Length-dst);
            while (count --> 0)
            {
                byte v = frame[offset++ & frame_mask];
                frame[frame_pos++ & frame_mask] = v;
                output[dst++] = v;
            }
        }
    }
    return dst;
}



int gamesystem_cmp_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    int dst = 0;
    while (dst < output_Length)
    {
        byte ctl = *input++;
        if (0 != (ctl & 0x80))
        {
            int num = *input++ + (ctl << 8);
            int offset = num & 0x7FF;
            int count = MIN (((num >> 10) & 0x1E) + 2, output_Length - dst);
            Binary_CopyOverlapped (output, dst-offset-1, dst, count);
            dst += count;
        }
        else
        {
            int count = MIN (ctl + 1, output_Length - dst);
            memcpy(output + dst, input, count); input += count;
            dst += count;
        }
    }
    return dst;
}



int puremail_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    byte frame[0x1000]="";
    int dst = 0;
    int bits = 0;
    int mask = 0;
    int frame_pos = 0xFEE;
    while (dst < output_Length)
    {
        mask >>= 1;
        if (0 == mask)
        {
            bits = *input++;
            if (-1 == bits)
                break;
            mask = 0x80;
        }
        if (0 == (bits & mask))
        {
            int b = *input++;
            if (-1 == b)
                break;
            output[dst++] = (byte)b;
            frame[frame_pos++ & 0xFFF] = (byte)b;
        }
        else
        {
            int offset = *(u16*)input; input += sizeof(u16);
            int count = (offset & 0xF) + 3;
            offset >>= 4;
            while (count --> 0 && dst < output_Length)
            {
                byte v = frame[offset++ & 0xFFF];
                frame[frame_pos++ & 0xFFF] = v;
                output[dst++] = v;
            }
        }
    }
    return dst;
}



int groover_pcg_unpack(unsigned char *m_input, int m_packed_size, unsigned char *output, int m_unpacked_size) {
    int dst = 0;
    int src;
    for (src = 0; src < m_packed_size && dst < m_unpacked_size; ++src)
    {
        memcpy(output + dst, m_input, 3); m_input += 3;
        int count = *m_input++;
        if (count > 0)
        {
            if (count > 1)
                Binary_CopyOverlapped (output, dst, dst+3, (count-1) * 3);
            dst += count * 3;
        }
    }
    return dst;
}



int mnp_mma_unpack(unsigned char *input, unsigned char *output, int output_Length) {
    byte id = *input++;
    if (id != 0xC0)
    {
        if (id != 0)
            return -1; //throw new InvalidFormatException();
        memcpy(output, input, output_Length);
        return output_Length;
    }
    int ctl = 0;
    int mask = 0;
    int dst = 0;
    while (dst < output_Length)
    {
        if (0 == mask)
        {
            ctl = *input++;
            if (-1 == ctl)
                break;
            mask = 0x80;
        }
        if ((ctl & mask) != 0)
        {
            int offset = *input++ << 8;
            offset |= *input++;
            int count = (offset & 0x1F) + 3;
            offset = (offset >> 5) + 1;
            Binary_CopyOverlapped (output, dst - offset, dst, count);
            dst += count;
        }
        else
        {
            u8  t = *input++;
            output[dst++] = (t<<5)|(t>>3);
        }
        mask >>= 1;
    }
    return dst;
}



int strikes_pck_unpack(unsigned char *input, int in_length, unsigned char *output, int output_Length) {
    byte frame[0x1000]="";
    int frame_pos = 0xFEE;
    int src = 0;
    int dst = 0;
    while (src < in_length)
    {
        int ctl = input[src++];
        int bit;
        for (bit = 1; bit != 0x100; bit <<= 1)
        {
            if (0 != (ctl & bit))
            {
                if (src >= in_length)
                    return dst;
                byte b = input[src++];
                frame[frame_pos++ & 0xFFF] = b;
                output[dst++] = b;
            }
            else
            {
                if (src + 2 > in_length)
                    return dst;
                int lo = input[src++];
                int hi = input[src++];
                int offset = (hi & 0xF0) << 4 | lo;
                int count = MIN (3 + (hi & 0xF), output_Length - dst);
                while (count --> 0)
                {
                    byte b = frame[offset++ & 0xFFF];
                    frame[frame_pos++ & 0xFFF] = b;
                    output[dst++] = b;
                }
            }
        }
    }
    return dst;
}

