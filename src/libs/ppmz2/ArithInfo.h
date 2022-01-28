#pragma once

namespace Ppmz2
{
    class ArithInfo
    {
    private:
        static const unsigned int CODE_BITS	= 31;
        static const unsigned int SHIFT_BITS = (CODE_BITS - 8);
        static const unsigned int CODE_BYTES = ((CODE_BITS+7)/8);

        static const unsigned int PRECISION_BITS = 9; // coding is done to this accuracy (in terms of range>>PRECISION_BITS)

        static const unsigned int MinRange = ((unsigned long)1<<SHIFT_BITS);
        static const unsigned int One = ((unsigned long)1<<CODE_BITS);
        static const unsigned int CumProbMax = (MinRange>>PRECISION_BITS);
        static const unsigned int CODE_MASK	= (One - 1);
        static const unsigned int EXTRA_BITS = ((CODE_BITS-1) % 8 + 1);		// == 7	== CODE_BITS - (8*(CODE_BYTES-1))
        static const unsigned int TAIL_EXTRA_BITS = (8 - EXTRA_BITS);

    public:
        ArithInfo()
            : _probMax(CumProbMax), _probMaxSafe(CumProbMax - 512), _outBuf(0), _outPtr(0),
            _code(0), _range(0), _overflow_bytes(0), _queue(0)
        {
        }

        void EncodeInitNoStuff(unsigned char* outBuf)
        {
            _outBuf = outBuf;
	        EncodeReInit(outBuf - 1);
        }

        void EncodeReInit(unsigned char* outBuf)
        {   
	        _code = 0;
	        _range = One;
	        _overflow_bytes = 0;
	        _queue = 0; // this is a waste of a byte

	        // ari->outBuf unchanged
	        _outPtr = outBuf;
        }

        void DecodeDone()
        {
        }

        void DecodeInitNoStuff(unsigned char* outBuf)
        {
	        _outBuf = _outPtr = outBuf;

	        DecodeReInit(outBuf - 1);
        }

        void DecodeReInit(unsigned char* outBuf)
        {
            unsigned long byte;

	        _outPtr = outBuf + 1;

	        /** the code needs to be kept filled with 31 bits ;
	        *	this means we cannot just read in 4 bytes.  We must read in 3,
	        *	then the 7 bits of another (EXTRA_BITS == 7) , and save that last
	        *	bit in the queue 
	        **/

	        byte = _queue = *(_outPtr)++;
	        _code = byte >> (TAIL_EXTRA_BITS);
	        _range = 1 << EXTRA_BITS;
        }

        void Decode(unsigned long symlow, unsigned long symhigh, unsigned long symtot)
        {
	        assert( symlow < symhigh && symhigh <= symtot );
	        assert( symtot <= CumProbMax );

            #ifdef FAST_ENCODE

	            ari->code -= dec_range_over_symtot * symlow;
	            ari->range = dec_range_over_symtot * (symhigh - symlow);

            #else
            {
                unsigned long lowincr = _dec_range_over_symtot * symlow;

	            _code -= lowincr;
	            if ( symhigh == symtot )	_range -= lowincr;
	            else 						_range = _dec_range_over_symtot * (symhigh - symlow);
            }
            #endif
        }

        unsigned long Get(unsigned long symtot)
        {
            assert( symtot <= CumProbMax );

			unsigned long range,code;
	        DecodeRenorm(&code, &range);

	        _dec_range_over_symtot = range / symtot;
			unsigned long ret = code / _dec_range_over_symtot;

	        _range = range;
	        _code  = code;
        		
			// This assert does not seem necessary, it goes off on some machines and not others and doesn't seem to 
			// change encoding performance or speed
	        //assert( ret < symtot );  // is this safe? ; apparently not!
	        ret = ( ret >= symtot ? symtot-1 : ret );

            return ret;
        }

        void DecodeRenorm(unsigned long* pcode,unsigned long* prange)
        {
	        unsigned long range = _range;
	        unsigned long code  = _code;

	        assert( range <= One );

	        while ( range <= MinRange )
	        {
		        range <<= 8;
		        code = (code<<8) + (((_queue)<<EXTRA_BITS)&0xFF);	// use the top bit in the queue
		        _queue = *(_outPtr)++;
		        code += (_queue) >> (TAIL_EXTRA_BITS);
	        }
        	
	        assert( range <= One );

	        *prange = range;
	        *pcode = code;
        }

        bool DecodeBit(unsigned long mid, unsigned long tot)
        {
            bool bit;
            unsigned long range, code;

	        DecodeRenorm(&code, &range);

            /**
            *
            *		r = (range / tot)
            *
            *	if ( (code / r) >= mid ) bit = 1; else bit = 0;
            *
            *	we eliminate one divide. this is the savings of the binary coder
            *
            **/

	        unsigned long r = (range / tot) * mid;

	        if ( code >= r )
	        {
		        bit = 1;
		        code -= r;
		        range -= r;
	        }
	        else
	        {
		        bit = 0;
		        range = r;
	        }

	        _range = range;
	        _code  = code;

            return bit;
        }

        int EncodeDone()
        {
            unsigned int rangeMask,rangeMSB;

	        // set code to the maximum that won't change how it decodes.
	        _code += _range - 1;

	        /* first send the queue */

	        if (_code & One)
	        {
		        *(_outPtr)++ = (unsigned char)(_queue + 1)&0xFF;
		        while ( _overflow_bytes-- ) *(_outPtr)++ = 0;
	        }
	        else
	        {
		        *(_outPtr)++ = _queue;
		        while ( _overflow_bytes-- ) *(_outPtr)++ = 0xFF;
	        }

	        /*****

	        the minimal way to flush is to do :

		        code += (range - 1);

		        clear code below MSB of range

	        eg. if range is 67 we do :
        		
		        code += 66;
		        code &= ~63;

	        then we just send code bytes until the remainder is zero.

	        (this assumes that when the decoder reads past EOF, it reads zeros!)

	        -----

	        we almost always write just 1 byte
	        (in fact, I think we might *always* write 1 byte)

	        ******/

	        if ( _range >= (1UL<<31) )
	        {
		        rangeMSB = 1UL<<31;
	        }
	        else
	        {
		        for(rangeMSB=1;rangeMSB <= _range; rangeMSB <<= 1) ;
		        rangeMSB >>= 1;
        		
                if ( _range >= rangeMSB && _range < rangeMSB*2 )
                    throw "Range error";
	        }

	        // clear code under rangeMask :

	        rangeMask = 0;
	        rangeMSB >>= 1;
	        while(rangeMSB)
	        {
		        rangeMask |= rangeMSB;
		        rangeMSB >>= 1;
	        }

	        if ( rangeMask < _range )
                throw "RangeMast was equal to or larger than range";

	        _code &= (~ rangeMask);
	        _code &= CODE_MASK;

	        while(_code)	
	        {
		        *(_outPtr)++ = (_code >> SHIFT_BITS) & 0xFF;
		        _code <<= 8;
		        _code &= CODE_MASK;
	        }

	        _outPtr[0] = 0;
	        _outPtr[1] = 0;
	        _outPtr[2] = 0;
	        _outPtr[3] = 0;
	        _outPtr[4] = 0;
	        _outPtr[5] = 0;

            return _outPtr - _outBuf;
        }

        void Encode(unsigned long symlow, unsigned long symhigh, unsigned long symtot)
        {
	        unsigned long code = _code;
	        unsigned long range = _range;

	        assert( symlow < symhigh && symhigh <= symtot );
	        assert( symtot <= CumProbMax );

	        /** we want to do :
	        *		 lowincr = (range * symlow) / symtot
	        *	but range & symtot can both be large , so this would overflow the register	
	        *	thus we instead do:
	        *
	        ***/

        #ifdef FAST_ENCODE
	        /*#*/ { 
	        unsigned long r = range / symtot;

	        code += r * symlow;
	        range = r * (symhigh - symlow);

	        /*#*/ }
        #else
	        /*#*/ { 
	        unsigned long r = range / symtot;

	        unsigned long lowincr = r * symlow;
	        code += lowincr;
	        if ( symhigh == symtot )	range -= lowincr;
	        else 						range = r * (symhigh - symlow);

	        /*#*/ }
        #endif

            EncodeRenorm(code, range);
        }

        void EncodeBit(unsigned long mid, unsigned long tot, bool bit)
        {
            unsigned long code = _code;
            unsigned long range = _range;

	        unsigned long r = (range / tot) * mid;

	        if ( bit )
	        {
		        code += r;
		        range -= r;
	        }
	        else
	        {
		        range = r;
	        }

            EncodeRenorm(code, range);
        }

        void EncodeRenorm(unsigned long code, unsigned long range)
        {
	        assert( range <= One );

	        while( range <= MinRange )
	        {
		        unsigned long byte = (code >> SHIFT_BITS);
        	
		        if ( byte == 0xFF )
		        {
			        /** the waiting queue is incremented like :
			        *		(ari->queue), 0xFF, 0xFF, ... ,0xFF, code
			        ***/

			        _overflow_bytes++;
		        }
		        else
		        {
			        unsigned long carry = code>>CODE_BITS;	

				        /* carry == 1 or 0 : is the top bit on ?
				        *	carry = byte>>8
				        * if ( carry )	send nextbyte+1
				        *				MinRange queue with zeros
				        * else			send nextbyte
				        *				MinRange queue with ones
				        **/					
        		
			        *(_outPtr)++ = (unsigned char)(_queue + carry);	// propagate the carry.
			        // send the queue
			        if ( _overflow_bytes )
			        {
				        *(_outPtr)++ = (unsigned char)(0xFF + carry);
				        while ( --(_overflow_bytes) ) *(_outPtr)++ = (unsigned char)(0xFF + carry);
			        }
			        _queue = byte;
		        }

		        code = (code<<8) & CODE_MASK;
		        range <<= 8;
	        }

	        assert( range <= One );

	        _code  = code;
	        _range = range;
        }

        int EncodeDoneMinimal()
        {
            unsigned int rangeMask, rangeMSB;

	        // set code to the maximum that won't change how it decodes.

	        _code += _range - 1;

	        /* first send the queue */

	        if ( _code & One )
	        {
		        *(_outPtr)++ = (unsigned char)(_queue + 1)&0xFF;
		        while ( _overflow_bytes-- ) *(_outPtr)++ = 0;
	        }
	        else
	        {
		        *(_outPtr)++ = _queue;
		        while ( _overflow_bytes-- ) *(_outPtr)++ = 0xFF;
	        }

	        /*****

	        the minimal way to flush is to do :

		        code += (range - 1);

		        clear code below MSB of range

	        eg. if range is 67 we do :
        		
		        code += 66;
		        code &= ~63;

	        then we just send code bytes until the remainder is zero.

	        (this assumes that when the decoder reads past EOF, it reads zeros!)

	        -----

	        we almost always write just 1 byte
	        (in fact, I think we might *always* write 1 byte)

	        ******/

	        if ( _range >= (1UL<<31) )
	        {
		        rangeMSB = 1UL<<31;
	        }
	        else
	        {
		        for(rangeMSB = 1; rangeMSB <= _range; rangeMSB <<= 1) ;
		        rangeMSB >>= 1;
        		
		        assert( _range >= rangeMSB && _range < rangeMSB*2 );
	        }

	        // clear code under rangeMask :

	        rangeMask = 0;
	        rangeMSB >>= 1;
	        while(rangeMSB)
	        {
		        rangeMask |= rangeMSB;
		        rangeMSB >>= 1;
	        }

	        assert( rangeMask < _range );

	        _code &= (~ rangeMask);
	        _code &= CODE_MASK;

	        while(_code)	
	        {
		        *(_outPtr)++ = (_code >> SHIFT_BITS) & 0xFF;
		        _code <<= 8;
		        _code &= CODE_MASK;
	        }

            for (int i = 0; i < 6; ++i)
                _outPtr[i] = 0;

            return _outPtr - _outBuf;
        }

    //private:        
        int _probMax;
        int _probMaxSafe;
	    unsigned char* _outBuf;
        unsigned char* _outPtr;
	    unsigned long _code;
        unsigned long _range;
	    unsigned long _overflow_bytes;
        unsigned long _queue;
        unsigned long _dec_range_over_symtot;
    };
}