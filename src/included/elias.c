// modified by Luigi Auriemma

// original pseudo code from wikipedia:
// http://en.wikipedia.org/wiki/Elias_delta_coding
// http://en.wikipedia.org/wiki/Elias_gamma_coding
// http://en.wikipedia.org/wiki/Elias_omega_coding



int elias_getbit(unsigned char **in) {
    static int  bits = 0;
    int     ret;

    if(!in || !*in) {   // init
        bits = 0;
        return(0);
    }
    if(bits >= 8) {
        bits = 0;
        (*in)++;
    }
    //ret = ((*in)[0] >> bits) & 1;
    ret = ((*in)[0] >> (7 - bits)) & 1;
    bits++;
    return(ret);
}



int eliasDeltaDecode(unsigned char* in, int insz, unsigned char* dest) {
    unsigned char    *inl = in + insz;
    elias_getbit(NULL);
    u32     *o = (u32 *)dest;
    int     i;
    while (in < inl)
    {
        int num = 1;
        int len = 1;
        int lengthOfLen = 0;
        while (!elias_getbit(&in) && (in < inl))
            lengthOfLen++;
        for (i = 0; i < lengthOfLen; i++)
        {
            len <<= 1;
            if (elias_getbit(&in))
                len |= 1;
        }
        for (i = 0; i < len-1; i++)
        {
            num <<= 1;
            if (elias_getbit(&in))
                num |= 1;
        }
        *o++ = num;
    }
    return((void *)o - (void *)dest);
}



int eliasGammaDecode(unsigned char* in, int insz, unsigned char* dest) {
    unsigned char    *inl = in + insz;
    elias_getbit(NULL);
    u32     *o = (u32 *)dest;
    int     a;
    while (in < inl)
    {
        int numberBits = 0;
        while (!elias_getbit(&in) && (in < inl))
            numberBits++; //keep on reading until we fetch a one...
        int current = 0;
        for (a=numberBits-1; a >= 0; a--) //Read numberBits bits
        {
            if (elias_getbit(&in))
                current |= 1 << a;
        }
        current |= 1 << numberBits; //last bit isn't encoded!
         *o++ = current;
    }
    return((void *)o - (void *)dest);
}



int eliasOmegaDecode(unsigned char* in, int insz, unsigned char* dest) {
    unsigned char    *inl = in + insz;
    elias_getbit(NULL);
    u32     *o = (u32 *)dest;
    int     i;
    while (in < inl)
    {
        int num = 1;
        while (elias_getbit(&in) && (in < inl))
        {
            int len = num;
            num = 1;
            for (i = 0; i < len; ++i)
            {
                num <<= 1;
                if (elias_getbit(&in))
                    num |= 1;
            }
        }
        *o++ = num;
    }
    return((void *)o - (void *)dest);
}


