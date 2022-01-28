// converted to C by Luigi Auriemma
// http://code.google.com/p/dsdecmp/source/browse/trunk/CSharp/LuminousArc/LuminousArc.cs

int luminousarc(unsigned char *instream, int inLength, unsigned char *outstream, int decompressedSize) {
    int i,
        o = 0;
    int readBytes = 0;

    /*
    byte[] magic = new byte[2];
    instream.Read(magic, 0, 2);
    if (magic[0] != 'L' || magic[1] != 'e')
        return(-1);
    unsigned char sizeBytes[4];
    memcpy(sizeBytes, instream + readBytes, 4);
    unsigned decompressedSize = sizeBytes[0] | (sizeBytes[1] << 8) | (sizeBytes[2] << 16) | (sizeBytes[3] << 24);
    readBytes += 4;
    */

    // the maximum 'DISP-5' is 0xFFF.
    int bufferLength = 0xFFF + 5;
    unsigned char buffer[bufferLength];
    int bufferOffset = 0;


    int currentOutSize = 0;
    int flags = 0, mask = 3;
    while (currentOutSize < decompressedSize)
    {
        // (throws when requested new flags byte is not available)
        //#region Update the mask. If all flag bits have been read, get a new set.
        // the current mask is the mask used in the previous run. So if it masks the
        // last flag bit, get a new flags byte.
        if (mask == 3)
        {
            if (readBytes >= inLength)
                return(-1);
            flags = instream[readBytes++];
            if (flags < 0)
                return(-1);
            mask = 0xC0;
        }
        else
        {
            mask >>= 2;
            flags >>= 2;
        }
        //#endregion

        switch (flags & 0x3)
        {
            case 0:
                //#region 0 -> LZ10-like format
                {
                    //#region Get length and displacement('disp') values from next 2 bytes
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
                    if (byte2 < 0)
                        return(-1);

                    // the number of bytes to copy
                    int length = byte2 >> 4;
                    length += 3;

                    // from where the bytes should be copied (relatively)
                    int disp = ((byte2 & 0x0F) << 8) | byte1;
                    disp += 5;

                    if (disp > currentOutSize)
                        return(-1);
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

                    break;
                }
                //#endregion
            case 1:
                //#region 1 -> compact LZ10/RLE-like format
                {
                    //#region Get length and displacement('disp') values from next byte
                    // there are < 2 bytes available when the end is at most 1 byte away
                    if (readBytes >= inLength)
                    {
                        return(-1);
                    }
                    int b = instream[readBytes++];
                    if (b < 0)
                        return(-1);

                    // the number of bytes to copy
                    int length = b >> 2;
                    length += 2;

                    // from where the bytes should be copied (relatively)
                    int disp = (b & 0x03);
                    disp += 1;

                    if (disp > currentOutSize)
                        return(-1);
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
                    break;
                }
                //#endregion
            case 2:
                //#region 2 -> copy 1 byte
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
                    break;
                }
                //#endregion
            case 3:
                //#region 3 -> copy 3 bytes
                {
                    for (i = 0; i < 3; i++)
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
                    break;
                }
            //#endregion
            default:
                return(-1);
        }

        //outstream.Flush();
    }

    if (readBytes < inLength)
    {
        // the input may be 4-byte aligned.
        if ((readBytes ^ (readBytes & 3)) + 4 < inLength)
            return(-1);
        // (this happens rather often for Le files?)
    }

    return(o);
    //return decompressedSize;
}

