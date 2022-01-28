// modified by Luigi Auriemma

// http://www.rockraidersunited.com/topic/6675-is-there-a-way-i-could-rip-files-of-lego-city-undercovers-disc/#comment-120442

int rfpk_decompress(unsigned char *fileData, int fileData_size, unsigned char *resultData, int resultData_size) {
	typedef struct {
		int toCopy;
		int thenCopy;
		int offset;
	} BlockHeaderTriple;

    int Count = 0;
    int iPos = 0;
    while (iPos < fileData_size)
    {
        //BlockHeaderTriple blockHeaderTriple = this.ReadBlockHeader();
        BlockHeaderTriple blockHeaderTriple = {0,0,0};
        unsigned char b = fileData[iPos];
        iPos++;
        if ((b & 128) == 0)
        {
            unsigned char b2 = fileData[iPos];
            iPos++;
            blockHeaderTriple.toCopy = (b & 12) >> 2;
            blockHeaderTriple.thenCopy = ((b & 112) >> 4) + 3;
            blockHeaderTriple.offset = (int)(b & 3) * 256 + (int)b2 + 1;
        }
        else if ((b & 192) == 128)
        {
            unsigned char b2 = fileData[iPos];
            iPos++;
            unsigned char b3 = fileData[iPos];
            iPos++;
            blockHeaderTriple.toCopy = (b2 & 192) >> 6;
            blockHeaderTriple.thenCopy = (int)((b & 63) + 4);
            blockHeaderTriple.offset = (int)(b2 & 63) * 256 + (int)b3 + 1;
        }
        else if ((b & 224) == 192)
        {
            unsigned char b2 = fileData[iPos];
            iPos++;
            unsigned char b3 = fileData[iPos];
            iPos++;
            unsigned char b4 = fileData[iPos];
            iPos++;
            blockHeaderTriple.toCopy = (b & 24) >> 3;
            int num = (int)(b & 7);
            blockHeaderTriple.thenCopy = (num << 7) + (int)b4 + 5;
            blockHeaderTriple.offset = (int)b2 * 256 + (int)b3 + 1;
        }
        else if (b == 252)
        {
            blockHeaderTriple.toCopy = 0;
            blockHeaderTriple.thenCopy = 0;
            blockHeaderTriple.offset = 0;
        }
        else if (b == 253)
        {
            blockHeaderTriple.toCopy = 1;
            blockHeaderTriple.thenCopy = 0;
            blockHeaderTriple.offset = 0;
        }
        else if (b == 254)
        {
            blockHeaderTriple.toCopy = 2;
            blockHeaderTriple.thenCopy = 0;
            blockHeaderTriple.offset = 0;
        }
        else if (b == 255)
        {
            blockHeaderTriple.toCopy = 3;
            blockHeaderTriple.thenCopy = 0;
            blockHeaderTriple.offset = 0;
        }
        else
        {
            if ((b & 224) != 224)
            {
                //throw new NotSupportedException(string.Format("{0:x2} @ {1:x8}", b, iPos));
            }
            blockHeaderTriple.toCopy = (int)(((b & 31) + 1) * 4);
            blockHeaderTriple.thenCopy = 0;
            blockHeaderTriple.offset = 0;
        }



        int i;
        for (i = 0; i < blockHeaderTriple.toCopy; i++)
        {
            if(Count >= resultData_size) break;
            resultData[Count] = (fileData[iPos]);
            Count++;
            iPos++;
        }
        for (i = 0; i < blockHeaderTriple.thenCopy; i++)
        {
            if(Count >= resultData_size) break;
            resultData[Count] = (resultData[Count - blockHeaderTriple.offset]);
            Count++;
        }
    }
    return Count;
}

