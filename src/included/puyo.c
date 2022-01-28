// original code from https://github.com/nickworonekin/puyotools
// original code from http://code.google.com/p/puyotools/ (BSD License) written by not.nmn and nickwor
// ported to C by Luigi Auriemma

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//typedef unsigned short  ushort;
//typedef unsigned int    uint;
//typedef unsigned char   byte;



// the compressed data must start from offset 0x10!
//                uint compressedSize   = data.ReadUInt(0x8).SwapEndian() + 16; // Compressed Size
//                uint decompressedSize = data.ReadUInt(0xC).SwapEndian();      // Decompressed Size
//                uint Cpointer = 0x10; // Compressed Pointer
int puyo_cnx_unpack(byte *compressedData, int compressedSize, byte *decompressedData, int decompressedSize) {
    int     i,
            j;

                uint Cpointer = 0x0;  // Compressed Pointer // was 0x10
                uint Dpointer = 0x0;  // Decompressed Pointer
                while (Cpointer < compressedSize && Dpointer < decompressedSize)
                {
                    byte Cflag = compressedData[Cpointer];
                    Cpointer++;

                    for (i = 0; i < 4; i++)
                    {
                        /* Check for the mode */
                        switch ((Cflag >> (i * 2)) & 3)
                        {
                            /* Padding Mode
					         * All CNX archives seem to be packed in 0x800 chunks. when nearing
					         * a 0x800 cutoff, there usually is a padding command at the end to skip
					         * a few bytes (to the next 0x800 chunk, i.e. 0x4800, 0x7000, etc.) */
                            case 0: {
                                byte temp_byte = compressedData[Cpointer];
                                Cpointer      += (uint)(temp_byte & 0xFF) + 1;

                                i = 3;
                                break;
                            }
                            /* Single Byte Copy Mode */
                            case 1: {
                                decompressedData[Dpointer] = compressedData[Cpointer];
                                Cpointer++;
                                Dpointer++;
                                break;
                            }
                            /* Copy from destination buffer to current position */
                            case 2: {
                                //uint temp_word = BitConverter.ToUInt16(compressedData, (int)Cpointer).SwapEndian();
                                uint temp_word = (compressedData[Cpointer] << 8) | compressedData[Cpointer+1];

                                uint off = (temp_word >> 5)   + 1;
                                uint len = (temp_word & 0x1F) + 4;

                                Cpointer += 2;

                                for (j = 0; j < len; j++)
                                {
                                    decompressedData[Dpointer] = decompressedData[Dpointer - off];
                                    Dpointer++;
                                }

                                break;
                            }
                            /* Direct Block Copy (first byte signifies length of copy) */
                            case 3: {
                                byte blockLength = compressedData[Cpointer];
                                Cpointer++;

                                for (j = 0; j < blockLength; j++)
                                {
                                    decompressedData[Dpointer] = compressedData[Cpointer];
                                    Cpointer++;
                                    Dpointer++;
                                }

                                break;
                            }
                        }
                    }
                }
    return(Dpointer);
}



// the compressed data must start from offset 0x8!
//                uint CompressedSize   = (uint)data.Length;
//                uint DecompressedSize = data.ReadUInt(0x4) >> 8;
//                uint SourcePointer = 0x8;
int puyo_cxlz_unpack(byte *CompressedData, int CompressedSize, byte *DecompressedData, int DecompressedSize) {
    int     i,
            j;
                uint SourcePointer = 0x0;   // was 0x8
                uint DestPointer   = 0x0;

                // Start Decompression
                while (SourcePointer < CompressedSize && DestPointer < DecompressedSize)
                {
                    byte Flag = CompressedData[SourcePointer]; // Compression Flag
                    SourcePointer++;

                    for (i = 7; i >= 0; i--)
                    {
                        if ((Flag & (1 << i)) == 0) // Data is not compressed
                        {
                            DecompressedData[DestPointer] = CompressedData[SourcePointer];
                            SourcePointer++;
                            DestPointer++;
                        }
                        else // Data is compressed
                        {
                            int Distance = (((CompressedData[SourcePointer] & 0xF) << 8) | CompressedData[SourcePointer + 1]) + 1;
                            int Amount   = (CompressedData[SourcePointer] >> 4) + 3;
                            SourcePointer += 2;

                            // Copy the data
                            for (j = 0; j < Amount; j++)
                                DecompressedData[DestPointer + j] = DecompressedData[DestPointer - Distance + j];
                            DestPointer += (uint)Amount;
                        }

                        // Check for out of range
                        if (SourcePointer >= CompressedSize || DestPointer >= DecompressedSize)
                            break;
                    }
                }
    return(DestPointer);
}



        uint puyo_GetNewMagicValue(uint xValue)
        {
            uint x;

            x = (((((((xValue << 1) + xValue) << 5) - xValue) << 5) + xValue) << 7) - xValue;
            x = (x << 6) - x;
            x = (x << 4) - x;

            return ((x << 2) - x) + 12345;
        }
        byte puyo_DecryptByte(byte value, uint xValue)
        {
            uint t0 = ((uint)xValue >> 16) & 0x7fff;
            return (byte)(value ^ ((uint)(((t0 << 8) - t0) >> 15)));
        }
// the compressed data must start from offset 0x34!
//                uint CompressedSize   = data.ReadUInt(0x4);
//                uint DecompressedSize = data.ReadUInt(0x30);
//                uint MagicValue       = data.ReadUInt(0x34);
//                uint SourcePointer = 0x40;
int puyo_lz00_unpack(byte *CompressedData, int CompressedSize, byte *DecompressedData, int DecompressedSize) {
    int     i,
            j;
                byte *DestBuffer = calloc(0x1000, 1);

                uint MagicValue    = *(uint *)CompressedData;
                uint SourcePointer = 0x40-0x34;   // was 0x40;
                uint DestPointer   = 0x0;
                uint BufferPointer = 0xFEE;

                // Start Decompression
                while (SourcePointer < CompressedSize && DestPointer < DecompressedSize)
                {
                    MagicValue = puyo_GetNewMagicValue(MagicValue);
                    byte Flag = puyo_DecryptByte(CompressedData[SourcePointer], MagicValue); // Compression Flag
                    SourcePointer++;

                    for (i = 0; i < 8; i++)
                    {
                        if ((Flag & (1 << i)) > 0) // Data is not compressed
                        {
                            MagicValue = puyo_GetNewMagicValue(MagicValue);
                            DecompressedData[DestPointer] = puyo_DecryptByte(CompressedData[SourcePointer], MagicValue);
                            DestBuffer[BufferPointer]     = DecompressedData[DestPointer];
                            SourcePointer++;
                            DestPointer++;
                            BufferPointer = (BufferPointer + 1) & 0xFFF;
                        }
                        else // Data is compressed
                        {
                            MagicValue      = puyo_GetNewMagicValue(MagicValue);
                            byte PairFirst  = puyo_DecryptByte(CompressedData[SourcePointer], MagicValue);
                            MagicValue      = puyo_GetNewMagicValue(MagicValue);
                            byte PairSecond = puyo_DecryptByte(CompressedData[SourcePointer + 1], MagicValue);

                            int Offset = ((((PairSecond >> 4) & 0xF) << 8) | PairFirst);
                            int Amount = (PairSecond & 0xF) + 3;
                            SourcePointer += 2;

                            for (j = 0; j < Amount; j++)
                            {
                                DecompressedData[DestPointer + j] = DestBuffer[(Offset + j) & 0xFFF];
                                DestBuffer[BufferPointer]         = DecompressedData[DestPointer + j];
                                BufferPointer = (BufferPointer + 1) & 0xFFF;
                            }
                            DestPointer += (uint)Amount;
                        }

                        // Check for out of range
                        if (SourcePointer >= CompressedSize || DestPointer >= DecompressedSize)
                            break;
                    }
                }
    free(DestBuffer);
    return(DestPointer);
}



// the compressed data must start from offset 0x10!
//                uint CompressedSize   = data.ReadUInt(0x4);
//                uint DecompressedSize = data.ReadUInt(0x8);
//                uint SourcePointer = 0x10;
int puyo_lz01_unpack(byte *CompressedData, int CompressedSize, byte *DecompressedData, int DecompressedSize) {
    int     i,
            j;
                byte *DestBuffer = calloc(0x1000, 1);

                uint SourcePointer = 0x0; // was 0x10;
                uint DestPointer   = 0x0;
                uint BufferPointer = 0xFEE;

                // Start Decompression
                while (SourcePointer < CompressedSize && DestPointer < DecompressedSize)
                {
                    byte Flag = CompressedData[SourcePointer]; // Compression Flag
                    SourcePointer++;

                    for (i = 0; i < 8; i++)
                    {
                        if ((Flag & (1 << i)) > 0) // Data is not compressed
                        {
                            DecompressedData[DestPointer] = CompressedData[SourcePointer];
                            DestBuffer[BufferPointer]     = DecompressedData[DestPointer];
                            SourcePointer++;
                            DestPointer++;
                            BufferPointer = (BufferPointer + 1) & 0xFFF;
                        }
                        else // Data is compressed
                        {
                            int Offset = ((((CompressedData[SourcePointer + 1] >> 4) & 0xF) << 8) | CompressedData[SourcePointer]);
                            int Amount = (CompressedData[SourcePointer + 1] & 0xF) + 3;
                            SourcePointer += 2;

                            for (j = 0; j < Amount; j++)
                            {
                                DecompressedData[DestPointer + j] = DestBuffer[(Offset + j) & 0xFFF];
                                DestBuffer[BufferPointer]         = DecompressedData[DestPointer + j];
                                BufferPointer = (BufferPointer + 1) & 0xFFF;
                            }
                            DestPointer += (uint)Amount;
                        }

                        // Check for out of range
                        if (SourcePointer >= CompressedSize || DestPointer >= DecompressedSize)
                            break;
                    }
                }
    free(DestBuffer);
    return(DestPointer);
}



// the compressed data must start from offset 0x4!
//                uint CompressedSize   = (uint)data.Length;
//                uint DecompressedSize = data.ReadUInt(0x0) >> 8;
//                uint SourcePointer = 0x4;
int puyo_lzss_unpack(byte *CompressedData, int CompressedSize, byte *DecompressedData, int DecompressedSize) {
    int     i,
            j;
                uint SourcePointer = 0x0; // was 0x4;
                uint DestPointer   = 0x0;

                // Start Decompression
                while (SourcePointer < CompressedSize && DestPointer < DecompressedSize)
                {
                    byte Flag = CompressedData[SourcePointer]; // Compression Flag
                    SourcePointer++;

                    for (i = 7; i >= 0; i--)
                    {
                        if ((Flag & (1 << i)) == 0) // Data is not compressed
                        {
                            DecompressedData[DestPointer] = CompressedData[SourcePointer];
                            SourcePointer++;
                            DestPointer++;
                        }
                        else // Data is compressed
                        {
                            int Distance = (((CompressedData[SourcePointer] & 0xF) << 8) | CompressedData[SourcePointer + 1]) + 1;
                            int Amount   = (CompressedData[SourcePointer] >> 4) + 3;
                            SourcePointer += 2;

                            // Copy the data
                            for (j = 0; j < Amount; j++)
                                DecompressedData[DestPointer + j] = DecompressedData[DestPointer - Distance + j];
                            DestPointer += (uint)Amount;
                        }

                        // Check for out of range
                        if (SourcePointer >= CompressedSize || DestPointer >= DecompressedSize)
                            break;
                    }
                }
    return(DestPointer);
}



// the compressed data must start from offset 0x4!
//                uint CompressedSize   = (uint)data.Length;
//                uint DecompressedSize = data.ReadUInt(0x0) >> 8;
//                uint SourcePointer = 0x4;
int puyo_onz_unpack(byte *CompressedData, int CompressedSize, byte *DecompressedData, int DecompressedSize) {
    int     i,
            j;
                uint SourcePointer = 0x0; // was 0x4;
                uint DestPointer   = 0x0;

                // not handled here because this is a memory2memory decompression
                //if (DecompressedSize == 0) // Next 4 bytes are the decompressed size
                //{
                    //DecompressedSize = data.ReadUInt(0x4);
                    //SourcePointer += 0x4;
                //}

                // Start Decompression
                while (SourcePointer < CompressedSize && DestPointer < DecompressedSize)
                {
                    byte Flag = CompressedData[SourcePointer]; // Compression Flag
                    SourcePointer++;

                    for (i = 7; i >= 0; i--)
                    {
                        if ((Flag & (1 << i)) == 0) // Data is not compressed
                        {
                            DecompressedData[DestPointer] = CompressedData[SourcePointer];
                            SourcePointer++;
                            DestPointer++;
                        }
                        else // Data is compressed
                        {
                            int Distance;
                            int Amount;

                            // Let's determine how many bytes the distance & length pair take up
                            switch (CompressedData[SourcePointer] >> 4)
                            {
                                case 0: { // 3 bytes
                                    Distance = (((CompressedData[SourcePointer + 1] & 0xF) << 8) | CompressedData[SourcePointer + 2]) + 1;
                                    Amount   = (((CompressedData[SourcePointer] & 0xF) << 4) | (CompressedData[SourcePointer + 1] >> 4)) + 17;
                                    SourcePointer += 3;
                                    break;
                                }
                                case 1: { // 4 bytes
                                    Distance = (((CompressedData[SourcePointer + 2] & 0xF) << 8) | CompressedData[SourcePointer + 3]) + 1;
                                    Amount   = (((CompressedData[SourcePointer] & 0xF) << 12) | (CompressedData[SourcePointer + 1] << 4) | (CompressedData[SourcePointer + 2] >> 4)) + 273;
                                    SourcePointer += 4;
                                    break;
                                }
                                default: { // 2 bytes
                                    Distance = (((CompressedData[SourcePointer] & 0xF) << 8) | CompressedData[SourcePointer + 1]) + 1;
                                    Amount   = (CompressedData[SourcePointer] >> 4) + 1;
                                    SourcePointer += 2;
                                    break;
                                }
                            }

                            // Copy the data
                            for (j = 0; j < Amount; j++)
                                DecompressedData[DestPointer + j] = DecompressedData[DestPointer - Distance + j];
                            DestPointer += (uint)Amount;
                        }

                        // Check for out of range
                        if (SourcePointer >= CompressedSize || DestPointer >= DecompressedSize)
                            break;
                    }
                }
    return(DestPointer);
}



// no problems with the starting point of compressed data here
int puyo_prs_unpack(byte *compressedData, int compressedSize, byte *decompressedData, int decompressedSize) {
    int     i,
            j;
                uint Cpointer = 0x0; // Compressed Pointer
                uint Dpointer = 0x0; // Decompressed Pointer

                while (Cpointer < compressedSize && (Dpointer < decompressedSize || decompressedSize == 0))
                {
                    byte Cflag = compressedData[Cpointer];
                    Cpointer++;

                    for (i = 0; i < 8; i++)
                    {
                        /* Is the data compressed? */
                        if ((Cflag & (1 << i)) > 0)
                        {
                            /* No */
                            decompressedData[Dpointer] = compressedData[Cpointer];
                            Cpointer++;
                            Dpointer++;
                        }
                        else
                        {
                            /* Yes */
                            i++;
                            if (i >= 8)
                            {
                                i = 0;
                                Cflag = compressedData[Cpointer];
                                Cpointer++;
                            }

                            /* How will we copy this? */
                            uint offset, amountToCopy;
                            if ((Cflag & (1 << i)) > 0)
                            {
                                byte first  = compressedData[Cpointer];
                                byte second = compressedData[Cpointer + 1];
                                Cpointer += 2;

                                /* Make sure we are not out of range */
                                if (Cpointer >= compressedSize)
                                    break;

                                offset = (uint)((second << 8 | first) >> 3) | 0xFFFFE000;
                                amountToCopy = first & (uint)0x7;

                                if (amountToCopy == 0)
                                {
                                    amountToCopy = (uint)compressedData[Cpointer] + 1;
                                    Cpointer++;
                                }
                                else
                                    amountToCopy += 2;
                            }
                            else
                            {
                                amountToCopy = 0;
                                for (j = 0; j < 2; j++)
                                {
                                    i++;
                                    if (i >= 8)
                                    {
                                        i = 0;
                                        Cflag = compressedData[Cpointer];
                                        Cpointer++;
                                    }
                                    offset = (amountToCopy << 1);
                                    amountToCopy = offset | (uint)((Cflag & (1 << i)) > 0 ? 0x1 : 0x0);
                                }
                                offset = (compressedData[Cpointer] | 0xFFFFFF00);
                                amountToCopy += 2;
                                Cpointer++;
                            }

                            /* Now copy the data */
                            for (j = 0; j < amountToCopy; j++)
                                decompressedData[Dpointer] = decompressedData[(int)(Dpointer + offset + j)];

                            Dpointer += amountToCopy;
                        }

                        /* Make sure we are not out of range */
                        if (Cpointer >= compressedSize || (Dpointer >= decompressedSize && decompressedSize != 0))
                            break;
                    }
                }
    return(Dpointer);
}


/*
// the compressed data must start from offset 0x4!
// uint decompressedSize = data.ReadUInt(0x0);
int puyo_pvz_unpack(byte *compressedData, int compressedSize, byte *decompressedData, int decompressedSize) {
    int     i,
            j;
                uint Cpointer = 0x0; // Compressed Pointer  // was 0x4
                uint Dpointer = 0x0; // Decompressed Pointer

                // This file heavily relies on VrSharp
                // Check to see if this is a valid PVR file
                Images images = new Images(data.Copy(0x4, (int)data.Length - 4), null);
                if (images.Format != GraphicFormat.PVR)
                    return(-1); //throw new Exception();

                // Get correct file offset
                int FileOffset = (data.ReadString(0x4, 4) == "GBIX" ? 0x10 : 0x0);

                PvrPixelCodec PaletteCodec = PvrCodecs.GetPixelCodec(data.ReadByte(FileOffset + 0xC));
                PvrDataCodec DataCodec     = PvrCodecs.GetDataCodec(data.ReadByte(FileOffset + 0xD));
                if (PaletteCodec == null || DataCodec == null)
                    throw new Exception();

                DataCodec.Decode.Initialize(0, 0, PaletteCodec.Decode);
                int ChunkSize = (DataCodec.Decode.GetChunkBpp() / 8);

                // Copy the first 16/32 bytes
                for (int i = 0; i < FileOffset + 0x10; i++)
                {
                    decompressedData[Dpointer] = compressedData[Cpointer];
                    Dpointer++;
                    Cpointer++;
                }

                // Ok, let's decompress the data
                while (Cpointer < compressedSize && Dpointer < decompressedSize)
                {
                    int copyAmount = compressedData[Cpointer + ChunkSize] + 1;

                    for (int i = 0; i < copyAmount; i++)
                        Array.Copy(compressedData, Cpointer, decompressedData, Dpointer + (i * ChunkSize), ChunkSize);

                    Cpointer += (uint)(ChunkSize + 1);
                    Dpointer += (uint)(copyAmount * ChunkSize);
                }
    return(Dpointer);
}
*/



// latest version of PuyoTools



int puyo_cnx(u8 *source, int sourceLength, u8 *destination, int destinationLength) {
    u8  *bck = destination, *bcks = source;
    int j;
    if(!memcmp(source, "CNX\x02", 4)) {
        source += 8;
        source += 4;
        source += 4;
    }
    int sourcePointer      = source - bcks;
    /*
            source += 8;

            // Get the source length and destination length
            int sourceLength      = ReadInt32BE(source) + 16;
            int destinationLength = ReadInt32BE(source);

            // Set the source, destination, and buffer pointers
            int sourcePointer      = 0x10;
    */
            int destinationPointer = 0x0;
            int bufferPointer      = 0x0;

            // Initalize the buffer
            u8 buffer[0x800];
            memset(buffer, 0, sizeof(buffer));  // necessary!

            // Start decompression
            while (sourcePointer < sourceLength)
            {
                byte flag = *source++;
                sourcePointer++;

                int i;
                for ( i = 0; i < 4; i++)
                {
                    byte value;
                    ushort matchPair;
                    int matchDistance, matchLength;

                    switch (flag & 0x3)
                    {
                        // Jump to the next 0x800 boundary
                        case 0:
                            value = *source++;

                            sourcePointer += value + 1;
                            source += value;

                            i = 3;
                            break;

                        // Not compressed, single byte
                        case 1:
                            value = *source++;
                            sourcePointer++;

                            *destination++ = (value);
                            destinationPointer++;

                            buffer[bufferPointer] = value;
                            bufferPointer = (bufferPointer + 1) & 0x7FF;
                            break;

                        // Compressed
                        case 2:
                            matchPair = (source[0] << 8) | source[1]; source += 2; //ReadUInt16BE(source);
                            sourcePointer += 2;

                            matchDistance = (matchPair >> 5) + 1;
                            matchLength = (matchPair & 0x1F) + 4;

                            for ( j = 0; j < matchLength; j++)
                            {
                                *destination++ = (buffer[(bufferPointer - matchDistance) & 0x7FF]);
                                destinationPointer++;

                                buffer[bufferPointer] = buffer[(bufferPointer - matchDistance) & 0x7FF];
                                bufferPointer = (bufferPointer + 1) & 0x7FF;
                            }
                            break;

                        // Not compressed, multiple bytes
                        case 3:
                            matchLength = *source++;
                            sourcePointer++;

                            for ( j = 0; j < matchLength; j++)
                            {
                                value = *source++;
                                sourcePointer++;

                                *destination++ = (value);
                                destinationPointer++;

                                buffer[bufferPointer] = value;
                                bufferPointer = (bufferPointer + 1) & 0x7FF;
                            }
                            break;
                    }

                    // Check to see if we reached the end of the source
                    if (sourcePointer >= sourceLength)
                    {
                        break;
                    }

                    // Check to see if we wrote too much data to the destination
                    if (destinationPointer > destinationLength)
                    {
                        return -1; //throw new Exception("Too much data written to the destination.");
                    }

                    flag >>= 2;
                }
            }
    return destination - bck;
}



        byte puyo_lz00_ReadByte(byte value, uint key)
        {
            // Generate a new key
            uint x = (((((((key << 1) + key) << 5) - key) << 5) + key) << 7) - key;
            x = (x << 6) - x;
            x = (x << 4) - x;

            key = ((x << 2) - x) + 12345;

            // Now return the value since we have the key
            uint t = (key >> 16) & 0x7FFF;
            return (byte)(value ^ ((((t << 8) - t) >> 15)));
        }
int puyo_lz00(u8 *source, int sourceLength, u8 *destination, int destinationLength) {
    u8  *bck = destination, *bcks = source;

    if(!memcmp(source, "LZ00", 4)) {
        source += 4;
        source += 4;
        source += 40;
        source += 4;
    }
    u32 key = source[0] | source[1] << 8 | source[2] << 16 | source[3] << 24;   source += 4;
    source += 8;
    int sourcePointer      = source - bcks;

    /*
            source += 4;

            // Get the source length, destination length, and encryption key
            int sourceLength = ReadInt32(source);

            source += 40;

            int destinationLength = ReadInt32(source);
            uint key = ReadUInt32(source);

            source += 8;

            // Set the source, destination, and buffer pointers
            int sourcePointer      = 0x40;
    */
            int destinationPointer = 0x0;
            int bufferPointer      = 0xFEE;

            // Initalize the buffer
            u8 buffer[0x1000];
            memset(buffer, 0, sizeof(buffer));  // necessary!

            // Start decompression
            while (sourcePointer < sourceLength)
            {
                byte flag = puyo_lz00_ReadByte(*source++, key);
                sourcePointer++;

                int i;
                for ( i = 0; i < 8; i++)
                {
                    if ((flag & 0x1) != 0) // Not compressed
                    {
                        byte value = puyo_lz00_ReadByte(*source++, key);
                        sourcePointer++;

                        *destination++ = (value);
                        destinationPointer++;

                        buffer[bufferPointer] = value;
                        bufferPointer = (bufferPointer + 1) & 0xFFF;
                    }
                    else // Compressed
                    {
                        byte b1 = puyo_lz00_ReadByte(*source++, key), b2 = puyo_lz00_ReadByte(*source++, key);
                        sourcePointer += 2;

                        int matchOffset = (((b2 >> 4) & 0xF) << 8) | b1;
                        int matchLength = (b2 & 0xF) + 3;

                        int j;
                        for ( j = 0; j < matchLength; j++)
                        {
                            *destination++ = (buffer[(matchOffset + j) & 0xFFF]);
                            destinationPointer++;

                            buffer[bufferPointer] = buffer[(matchOffset + j) & 0xFFF];
                            bufferPointer = (bufferPointer + 1) & 0xFFF;
                        }
                    }

                    // Check to see if we reached the end of the source
                    if (sourcePointer >= sourceLength)
                    {
                        break;
                    }

                    // Check to see if we wrote too much data to the destination
                    if (destinationPointer > destinationLength)
                    {
                        return -1; //throw new Exception("Too much data written to the destination.");
                    }

                    flag >>= 1;
                }
            }
    return destination - bck;
}



int puyo_lz01(u8 *source, int sourceLength, u8 *destination, int destinationLength) {
    u8  *bck = destination, *bcks = source;

    if(!memcmp(source, "LZ01", 4)) {
        source += 4;
        source += 4;
        source += 4;
        source += 4;
    }
    int sourcePointer      = source - bcks;
    /*

            source += 4;

            // Get the source length and the destination length
            int sourceLength      = ReadInt32(source);
            int destinationLength = ReadInt32(source);

            source += 4;

            // Set the source, destination, and buffer pointers
            int sourcePointer      = 0x10;
    */
            int destinationPointer = 0x0;
            int bufferPointer      = 0xFEE;

            // Initalize the buffer
            u8 buffer[0x1000];
            memset(buffer, 0, sizeof(buffer));  // necessary!

            // Start decompression
            while (sourcePointer < sourceLength)
            {
                byte flag = *source++;
                sourcePointer++;

                int i;
                for ( i = 0; i < 8; i++)
                {
                    if ((flag & 0x1) != 0) // Not compressed
                    {
                        byte value = *source++;
                        sourcePointer++;

                        *destination++ = (value);
                        destinationPointer++;

                        buffer[bufferPointer] = value;
                        bufferPointer = (bufferPointer + 1) & 0xFFF;
                    }
                    else // Compressed
                    {
                        byte b1 = *source++, b2 = *source++;
                        sourcePointer += 2;

                        int matchOffset = (((b2 >> 4) & 0xF) << 8) | b1;
                        int matchLength = (b2 & 0xF) + 3;

                        int j;
                        for ( j = 0; j < matchLength; j++)
                        {
                            *destination++ = (buffer[(matchOffset + j) & 0xFFF]);
                            destinationPointer++;

                            buffer[bufferPointer] = buffer[(matchOffset + j) & 0xFFF];
                            bufferPointer = (bufferPointer + 1) & 0xFFF;
                        }
                    }

                    // Check to see if we reached the end of the source
                    if (sourcePointer >= sourceLength)
                    {
                        break;
                    }

                    // Check to see if we wrote too much data to the destination
                    if (destinationPointer > destinationLength)
                    {
                        return -1; //throw new Exception("Too much data written to the destination.");
                    }

                    flag >>= 1;
                }
            }
    return destination - bck;
}



int puyo_lz10(u8 *source, int sourceLength, u8 *destination, int destinationLength) {
    u8  *bck = destination;

    int sourcePointer      = 0;
    /*
            // Get the source and destination length
            int sourceLength      = (int)(source.Length - source);
            int destinationLength = ReadInt32(source) >> 8;

            // Set the source, destination, and buffer pointers
            int sourcePointer      = 0x4;
    */
            int destinationPointer = 0x0;
            int bufferPointer      = 0x0;

            // Initalize the buffer
            u8 buffer[0x1000];
            memset(buffer, 0, sizeof(buffer));  // necessary!

            // Start decompression
            while (sourcePointer < sourceLength)
            {
                byte flag = *source++;
                sourcePointer++;

                int i;
                for ( i = 0; i < 8; i++)
                {
                    if ((flag & 0x80) == 0) // Not compressed
                    {
                        byte value = *source++;
                        sourcePointer++;

                        *destination++ = (value);
                        destinationPointer++;

                        buffer[bufferPointer] = value;
                        bufferPointer = (bufferPointer + 1) & 0xFFF;
                    }
                    else // Compressed
                    {
                        byte b1 = *source++, b2 = *source++;
                        sourcePointer += 2;

                        int matchDistance = (((b1 & 0xF) << 8) | b2) + 1;
                        int matchLength   = (b1 >> 4) + 3;

                        int j;
                        for ( j = 0; j < matchLength; j++)
                        {
                            *destination++ = (buffer[(bufferPointer - matchDistance) & 0xFFF]);
                            destinationPointer++;

                            buffer[bufferPointer] = buffer[(bufferPointer - matchDistance) & 0xFFF];
                            bufferPointer = (bufferPointer + 1) & 0xFFF;
                        }
                    }

                    // Check to see if we reached the end of the source
                    if (sourcePointer >= sourceLength)
                    {
                        break;
                    }

                    // Check to see if we wrote too much data to the destination
                    if (destinationPointer > destinationLength)
                    {
                        return -1; //throw new Exception("Too much data written to the destination.");
                    }

                    flag <<= 1;
                }
            }

    return destination - bck;
}



int puyo_lz11(u8 *source, int sourceLength, u8 *destination, int destinationLength) {
    u8  *bck = destination;

            // Set the source, destination, and buffer pointers
            int sourcePointer      = 0; //0x4;
            int destinationPointer = 0x0;
            int bufferPointer      = 0x0;

    /*
            // Get the source and destination length
            int sourceLength      = (int)(source.Length - source);
            int destinationLength = ReadInt32(source) >> 8;
            if (destinationLength == 0)
            {
                // If the destination length is larger than 0xFFFFFF, then the next 4 bytes is the destination length
                destinationLength = ReadInt32(source);
                sourcePointer += 4;
            }
    */

            // Initalize the buffer
            u8 buffer[0x1000];
            memset(buffer, 0, sizeof(buffer));  // necessary!

            // Start decompression
            while (sourcePointer < sourceLength)
            {
                byte flag = *source++;
                sourcePointer++;

                int i;
                for ( i = 0; i < 8; i++)
                {
                    if ((flag & 0x80) == 0) // Not compressed
                    {
                        byte value = *source++;
                        sourcePointer++;

                        *destination++ = (value);
                        destinationPointer++;

                        buffer[bufferPointer] = value;
                        bufferPointer = (bufferPointer + 1) & 0xFFF;
                    }
                    else // Data is compressed
                    {
                        int matchDistance, matchLength;

                        // Read in the first 2 bytes (since they are used for all of these)
                        byte b1 = *source++, b2 = *source++, b3, b4;
                        sourcePointer += 2;

                        // Let's determine how many bytes the distance & length pair take up
                        switch (b1 >> 4)
                        {
                            case 0: // 3 bytes
                                b3 = *source++;
                                sourcePointer++;

                                matchDistance = (((b2 & 0xF) << 8) | b3) + 1;
                                matchLength = (((b1 & 0xF) << 4) | (b2 >> 4)) + 17;
                                break;

                            case 1: // 4 bytes
                                b3 = *source++;
                                b4 = *source++;
                                sourcePointer += 2;

                                matchDistance = (((b3 & 0xF) << 8) | b4) + 1;
                                matchLength = (((b1 & 0xF) << 12) | (b2 << 4) | (b3 >> 4)) + 273;
                                break;

                            default: // 2 bytes
                                matchDistance = (((b1 & 0xF) << 8) | b2) + 1;
                                matchLength = (b1 >> 4) + 1;
                                break;
                        }

                        int j;
                        for ( j = 0; j < matchLength; j++)
                        {
                            *destination++ = (buffer[(bufferPointer - matchDistance) & 0xFFF]);
                            destinationPointer++;

                            buffer[bufferPointer] = buffer[(bufferPointer - matchDistance) & 0xFFF];
                            bufferPointer = (bufferPointer + 1) & 0xFFF;
                        }
                    }

                    // Check to see if we reached the end of the source
                    if (sourcePointer >= sourceLength)
                    {
                        break;
                    }

                    // Check to see if we wrote too much data to the destination
                    if (destinationPointer > destinationLength)
                    {
                        return -1; //throw new Exception("Too much data written to the destination.");
                    }

                    flag <<= 1;
                }
            }

    return destination - bck;
}




        int puyo_prs_GetControlBit(int *bitPos, u8 *currentByte, u8 **source)
        {
            (*bitPos)--;
            if (*bitPos == 0)
            {
                *currentByte = (**source)++;
                *bitPos = 8;
            }

            int flag = *currentByte & 1;
            *currentByte >>= 1;
            return flag;
        }

int puyo_prs(u8 *source, int sourceLength, u8 *destination, int destinationLength) {
    u8  *bck = destination;

            int bitPos = 9;
            byte currentByte;
            int lookBehindOffset, lookBehindLength;

            currentByte = *source++;
            for (; ; )
            {
                if (puyo_prs_GetControlBit(&bitPos, &currentByte, &source) != 0)
                {
                    // Direct byte
                    *destination++ = (*source++);
                    continue;
                }

                if (puyo_prs_GetControlBit(&bitPos, &currentByte, &source) != 0)
                {
                    lookBehindOffset = *source++;
                    lookBehindOffset |= *source++ << 8;
                    if (lookBehindOffset == 0)
                    {
                        // End of the compressed data
                        break;
                    }

                    lookBehindLength = lookBehindOffset & 7;
                    lookBehindOffset = (lookBehindOffset >> 3) | -0x2000;
                    if (lookBehindLength == 0)
                    {
                        lookBehindLength = *source++ + 1;
                    }
                    else
                    {
                        lookBehindLength += 2;
                    }
                }
                else
                {
                    lookBehindLength = 0;
                    lookBehindLength = (lookBehindLength << 1) | puyo_prs_GetControlBit(&bitPos, &currentByte, &source);
                    lookBehindLength = (lookBehindLength << 1) | puyo_prs_GetControlBit(&bitPos, &currentByte, &source);
                    lookBehindOffset = *source++ | -0x100;
                    lookBehindLength += 2;
                }

                int i;
                for ( i = 0; i < lookBehindLength; i++)
                {
                    long writePosition = destination - bck;
                    destination = destination + (writePosition + lookBehindOffset);
                    byte b = *destination++;
                    destination = destination + (writePosition);
                    *destination++ = (b);
                }
            }
    return destination - bck;
}

