// converted to C by Luigi Auriemma
// http://code.google.com/p/dsdecmp/source/browse/trunk/CSharp/GoldenSunDD/GoldenSunDD.cs

int goldensun(unsigned char *instream, int inLength, unsigned char *outstream, int decompressedSize) {
    //#region format specification
    // no NDSTEK-like specification for this one; I seem to not be able to get those right.
    /*
     * byte tag; // 0x40
     * byte[3] decompressedSize;
     * the rest is the data;
     * 
     * for each chunk:
     *      - first byte determines which blocks are compressed
     *           multiply by -1 to get the proper flags (1->compressed, 0->raw)
     *      - then come 8 blocks:
     *          - a non-compressed block is simply one single byte
     *          - a compressed block can have 3 sizes:
     *              - A0 CD EF
     *                  -> Length = EF + 0x10, Disp = CDA
     *              - A1 CD EF GH
     *                  -> Length = GHEF + 0x110, Disp = CDA
     *              - AB CD  (B > 1)
     *                  -> Length = B, Disp = CDA
     *              Copy <Length> bytes from Dest-<Disp> to Dest (with <Dest> similar to the NDSTEK specs)
     */
    //#endregion

    int i,
        o = 0;
    int readBytes = 0;

    /*
    unsigned char type = instream[0]; readBytes++;
    if (type != 0x40) return(-1);
    unsigned char sizeBytes[4];
    memcpy(sizeBytes, instream + readBytes, 3); readBytes += 3;
    int decompressedSize = sizeBytes[0] | (sizeBytes[1] << 8) | (sizeBytes[2] << 16);
    if (decompressedSize == 0)
    {
        memcpy(sizeBytes, instream + readBytes, 4);
        decompressedSize = sizeBytes[0] | (sizeBytes[1] << 8) | (sizeBytes[2] << 16) | (sizeBytes[3] << 24);
        readBytes += 4;
    }
    */

    // the maximum 'DISP' is 0xFFF.
    int bufferLength = 0x1000;
    unsigned char buffer[bufferLength];
    int bufferOffset = 0;

    int currentOutSize = 0;
    int currentBlock = 0;
    // the expended flag byte
    unsigned char expandedFlags[8];
    while (currentOutSize < decompressedSize)
    {
        // (throws when requested new flags byte is not available)
        //#region Update the mask. If all flag bits have been read, get a new set.
        // the current mask is the mask used in the previous run. So if it masks the
        // last flag bit, get a new flags byte.
        if (currentBlock == 8)
        {
            if (readBytes >= inLength)
                return(-1);
            int flags = instream[readBytes++];
            if (flags < 0)
                return(-1);

            // determine which blocks are compressed
            int b = 0;
            // flags = -flags
            while (flags > 0)
            {
                int bit = (flags & 0x80) > 0;
                flags = (flags & 0x7F) << 1;
                expandedFlags[b++] = (flags == 0) || !bit;
            }

            currentBlock = 0;
        }
        else
        {
            currentBlock++;
        }
        //#endregion

        // bit = 1 <=> compressed.
        if (expandedFlags[currentBlock])
        {
            // (throws when < 2, 3 or 4 bytes are available)
            //#region Get length and displacement('disp') values from next 2, 3 or 4 bytes

            // there are < 2 bytes available when the end is at most 1 byte away
            if (readBytes + 1 >= inLength)
            {
                // make sure the stream is at the end
                //if (readBytes < inLength)
                //{
                    //instream[readBytes++];
                //}
                return(-1);
            }
            int byte1 = instream[readBytes++];
            int byte2 = instream[readBytes++];
            if (byte2 < 0) return(-1);

            int disp, length;
            disp = (byte1 >> 4) + (byte2 << 4);
            if (disp > currentOutSize)
                return(-1);

            switch (byte1 & 0x0F)
            {
                case 0:
                    {
                        if (readBytes >= inLength)
                            return(-1);
                        int byte3 = instream[readBytes++];
                        if (byte3 < 0)
                            return(-1);
                        length = byte3 + 0x10;
                        break;
                    }
                case 1:
                    {
                        if (readBytes + 1 >= inLength)
                            return(-1);
                        int byte3 = instream[readBytes++];
                        int byte4 = instream[readBytes++];
                        if (byte4 < 0)
                            return(-1);
                        length = ((byte3 << 8) + byte4) + 0x110;
                        break;
                    }
                default:
                    {
                        length = byte1 & 0x0F;
                        break;
                    }
            }
            //#endregion

            int bufIdx = bufferOffset + bufferLength - disp;
            for (i = 0; i < length; i++)
            {
                unsigned char next = buffer[bufIdx % bufferLength];
                bufIdx++;
                outstream[o++] = next;
                buffer[bufferOffset] = next;
                bufferOffset = (bufferOffset + 1) % bufferLength;
            }
            currentOutSize += length;
        }
        else
        {
            if (readBytes >= inLength)
                return(-1);
            int next = instream[readBytes++];
            if (next < 0)
                return(-1);

            currentOutSize++;
            outstream[o++] = next;
            buffer[bufferOffset] = (unsigned char)next;
            bufferOffset = (bufferOffset + 1) % bufferLength;
        }
        //outstream.Flush();
    }

    if (readBytes < inLength)
    {
        // the input may be 4-byte aligned.
        if ((readBytes ^ (readBytes & 3)) + 4 < inLength)
            return(-1);
    }

    return(o);
    //return decompressedSize;
}

