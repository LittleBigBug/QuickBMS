// various code from DSDecmp: http://code.google.com/p/dsdecmp/
// original code of unlz77wii_raw10 from "Hector Martin <marcan@marcansoft.com>" http://wiibrew.org/wiki/Wii.py
// ported to C by Luigi Auriemma

#define WII_NONE_TAG    0x00
#define WII_LZ77_TAG    0x10
#define WII_LZSS_TAG    0x11
#define WII_HUFF_TAG    0x20
#define WII_RLE_TAG     0x30
#define WII_040_TAG     0x40



int unlz77wii_raw10(unsigned char *in, int insz, unsigned char *out, int outsz) {
    int     i,
            j,
            flags,
            info,
            num;
    u8      *inl,
            *outl,
            *o,
            *p;

    o    = out;
    outl = out + outsz;
    inl  = in + insz;

    for(;;) {
        if(in >= inl) break;
        flags = *in++;
        for(i = 0; i < 8; i++) {
            if(o >= outl) break;    // needed
            if(flags & 0x80) {
                if((in + 2) > inl) break;
                info = (in[0] << 8) | in[1];
                in += 2;
                num = 3 + ((info >> 12) & 0xF);
                p = (o - (info & 0xfff)) - 1;
                if(p < out) return(-1);
                if((o + num) > outl) return(-1);
                for(j = 0; j < num; j++) {
                    *o++ = *p++;
                }
            } else {
                if(in >= inl) break;
                if(o >= outl) return(-1);
                *o++ = *in++;
            }
            flags <<= 1;
        }
    }
    return(o - out);
}



int unlz77wii_raw11(unsigned char *in, int insz, unsigned char *outdata, int decomp_size) {
    int     curr_size;
    u8      *inl;

            int i, j, disp=0, len=0, flag, cdest;
            unsigned char b1, bt, b2, b3, flags;
            int threshold = 1;

    inl  = in + insz;
    curr_size = 0;

            while (curr_size < decomp_size)
            {
                if(in >= inl) break;
                flags = *in++;

                for (i = 0; i < 8 && curr_size < decomp_size; i++)
                {
                    flag = (flags & (0x80 >> i)) > 0;
                    if (flag)
                    {
                        if(in >= inl) break;
                        b1 = *in++;

                        switch (b1 >> 4)
                        {
                            //#region case 0
                            case 0: {
                                // ab cd ef
                                // =>
                                // len = abc + 0x11 = bc + 0x11
                                // disp = def

                                len = b1 << 4;
                                if(in >= inl) break;
                                bt = *in++;
                                len |= bt >> 4;
                                len += 0x11;

                                disp = (bt & 0x0F) << 8;
                                if(in >= inl) break;
                                b2 = *in++;
                                disp |= b2;
                                break; }
                            //#endregion

                            //#region case 1
                            case 1: {
                                // ab cd ef gh
                                // => 
                                // len = bcde + 0x111
                                // disp = fgh
                                // 10 04 92 3F => disp = 0x23F, len = 0x149 + 0x11 = 0x15A

                                if((in + 3) > inl) break;
                                bt = *in++;
                                b2 = *in++;
                                b3 = *in++;

                                len = (b1 & 0xF) << 12; // len = b000
                                len |= bt << 4; // len = bcd0
                                len |= (b2 >> 4); // len = bcde
                                len += 0x111; // len = bcde + 0x111
                                disp = (b2 & 0x0F) << 8; // disp = f
                                disp |= b3; // disp = fgh
                                break; }
                            //#endregion

                            //#region other
                            default: {
                                // ab cd
                                // =>
                                // len = a + threshold = a + 1
                                // disp = bcd

                                len = (b1 >> 4) + threshold;

                                disp = (b1 & 0x0F) << 8;
                                if(in >= inl) break;
                                b2 = *in++;
                                disp |= b2;
                                break; }
                            //#endregion
                        }

                        if (disp > curr_size)
                            return(-1);

                        cdest = curr_size;

                        for (j = 0; j < len && curr_size < decomp_size; j++)
                            outdata[curr_size++] = outdata[cdest - disp - 1 + j];

                        if (curr_size > decomp_size)
                        {
                            //throw new Exception(String.Format("File {0:s} is not a valid LZ77 file; actual output size > output size in header", filein));
                            //Console.WriteLine(String.Format("File {0:s} is not a valid LZ77 file; actual output size > output size in header; {1:x} > {2:x}.", filein, curr_size, decomp_size));
                            break;
                        }
                    }
                    else
                    {
                        if(in >= inl) break;
                        outdata[curr_size++] = *in++;

                        if (curr_size > decomp_size)
                        {
                            //throw new Exception(String.Format("File {0:s} is not a valid LZ77 file; actual output size > output size in header", filein));
                            //Console.WriteLine(String.Format("File {0:s} is not a valid LZ77 file; actual output size > output size in header; {1:x} > {2:x}", filein, curr_size, decomp_size));
                            break;
                        }
                    }
                }
            }
    return(curr_size);
}



int unlz77wii_raw30(unsigned char *in, int insz, unsigned char *outdata, int decomp_size) {
    int     curr_size;
    u8      *inl;

            int i, rl;
            unsigned char flag, b;
            int compressed;

    inl  = in + insz;
    curr_size = 0;

            while (1)
            {
                // get tag
                if(in >= inl) break;
                flag = *in++;
                compressed = (flag & 0x80) > 0;
                rl = flag & 0x7F;
                if (compressed)
                    rl += 3;
                else
                    rl += 1;
                //curr_size += rl;
                if (compressed)
                {
                    if(in >= inl) break;
                    b = *in++;
                    for (i = 0; i < rl; i++)
                        outdata[curr_size++] = b;
                }
                else
                    for (i = 0; i < rl; i++) {
                        if(in >= inl) break;
                        outdata[curr_size++] = *in++;
                    }

                if (curr_size > decomp_size)
                {
                    //Console.WriteLine("curr_size > decomp_size; {0:x}>{1:x}", curr_size, decomp_size);
                    break;// throw new Exception(String.Format("File {0:s} is not a valid LZSS file; actual output size > output size in header", filein));
                }
                if (curr_size == decomp_size)
                    break;
            }
    return(curr_size);
}



int unlz77wii_raw20(unsigned char *in, int zsize, unsigned char *out, int size, int bits) {
    u32 header = 0;
    if(bits) {
        header = (WII_HUFF_TAG | bits) | (size << 8);
    } else {
        // remember that in must contain the 32bit header!
    }
    nintendo_ds_set_inout(in, zsize, out, size); HUF_Decode("", header); size = nintendo_ds_set_inout(NULL, 0, NULL, 0);
    return size;
}



int unlz77wii_raw00(unsigned char *in, int insz, unsigned char *out, int outsz) {
    if(insz > outsz) return(-1);
    memcpy(out, in, outsz);
    return(outsz);
}



int unlz77wii(unsigned char *in, int insz, u8 **ret_out, int *full_outsz) {
    int         outsz,
                tag;
    unsigned char   *out;

    if(insz < 4) return(-1);
    if(!memcmp(in, "LZ77", 4) | !memcmp(in, "CMPR", 4)) {
        in   += 4;
        insz -= 4;
    }
    tag = in[0];
    outsz = in[1] | (in[2] << 8) | (in[3] << 16);
    in   += 4;
    insz -= 4;
    out = *ret_out;
    myalloc(&out, outsz, full_outsz);
    *ret_out = out;

    ntcompress_init();
    switch(tag >> 4) {
        case (WII_NONE_TAG >> 4):  outsz = unlz77wii_raw00(in, insz, out, outsz);   break;
        case (WII_LZ77_TAG >> 4): {
                 if(tag == WII_LZ77_TAG) outsz = unlz77wii_raw10(in, insz, out, outsz);
            else if(tag == WII_LZSS_TAG) outsz = unlz77wii_raw11(in, insz, out, outsz);
            else                         outsz = -1;
            break;
        }
        case (WII_HUFF_TAG >> 4):  outsz = unlz77wii_raw20(in-4, insz+4, out, outsz, 0);    break;
        case (WII_RLE_TAG  >> 4):  outsz = unlz77wii_raw30(in, insz, out, outsz);   break;
        case (WII_040_TAG  >> 4):  outsz = ntcompress_40(in,   insz, out);          break;
        default: outsz = -1;
        // modify COMP_NTCOMPRESS too
    }
    return(outsz);
}



int unlz77wii_level5(unsigned char *in, int insz, u8 **ret_out, int *full_outsz) {
    int         outsz,
                tag;
    unsigned char   *out;

    if(insz < 4) return(-1);
    tag = in[0] & ((1<<3)-1);
    outsz = (in[0] | (in[1] << 8) | (in[2] << 16) | (in[3] << 24)) >> 3;
    in   += 4;
    insz -= 4;
    out = *ret_out;
    myalloc(&out, outsz, full_outsz);
    *ret_out = out;

    switch(tag) {
        case 0: outsz = unlz77wii_raw00(in, insz, out, outsz);      break;
        case 1: outsz = puyo_lzss_unpack(in, insz, out, outsz);     break;
        case 2: outsz = unlz77wii_raw20(in, insz, out, outsz, 4);   break;
        case 3: outsz = unlz77wii_raw20(in, insz, out, outsz, 8);   break;
        case 4: outsz = unlz77wii_raw30(in, insz, out, outsz);      break;
        default: outsz = -1;
    }
    return(outsz);
}
