// http://en.wikipedia.org/wiki/Jenkins_hash_function
uint32_t jenkins_one_at_a_time_hash(unsigned char *key, size_t len)
{
    uint32_t hash, i;
    for(hash = i = 0; i < len; ++i)
    {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

    // http://en.wikipedia.org/wiki/Pearson_hashing
   uint64_t xPear16(unsigned char *x, size_t len) {
      size_t i, j;
      unsigned char hh[8];
      static const unsigned char T[256] = {
      // 256 values 0-255 in any (random) order suffices
       98,  6, 85,150, 36, 23,112,164,135,207,169,  5, 26, 64,165,219, //  1
       61, 20, 68, 89,130, 63, 52,102, 24,229,132,245, 80,216,195,115, //  2
       90,168,156,203,177,120,  2,190,188,  7,100,185,174,243,162, 10, //  3
      237, 18,253,225,  8,208,172,244,255,126,101, 79,145,235,228,121, //  4
      123,251, 67,250,161,  0,107, 97,241,111,181, 82,249, 33, 69, 55, //  5
       59,153, 29,  9,213,167, 84, 93, 30, 46, 94, 75,151,114, 73,222, //  6
      197, 96,210, 45, 16,227,248,202, 51,152,252,125, 81,206,215,186, //  7
       39,158,178,187,131,136,  1, 49, 50, 17,141, 91, 47,129, 60, 99, //  8
      154, 35, 86,171,105, 34, 38,200,147, 58, 77,118,173,246, 76,254, //  9
      133,232,196,144,198,124, 53,  4,108, 74,223,234,134,230,157,139, // 10
      189,205,199,128,176, 19,211,236,127,192,231, 70,233, 88,146, 44, // 11
      183,201, 22, 83, 13,214,116,109,159, 32, 95,226,140,220, 57, 12, // 12
      221, 31,209,182,143, 92,149,184,148, 62,113, 65, 37, 27,106,166, // 13
        3, 14,204, 72, 21, 41, 56, 66, 28,193, 40,217, 25, 54,179,117, // 14
      238, 87,240,155,180,170,242,212,191,163, 78,218,137,194,175,110, // 15
       43,119,224, 71,122,142, 42,160,104, 48,247,103, 15, 11,138,239  // 16
      };     
      for (j = 0; j < 8; j++) {
         unsigned char h = T[(x[0] + j) % 256];
         for (i = 1; i < len; i++) {
            h = T[h ^ x[i]];
         }
         hh[j] = h;
      }
      uint64_t ret = 0;
        for(i = 0; i < 8; i++) {
            ret <<= 8;
            ret |= hh[i];
        }
      return ret;
   }



// https://raw.githubusercontent.com/ladislav-zezula/StormLib/master/src/SBaseCommon.cpp

#define MPQ_HASH_TABLE_INDEX    0x000
#define MPQ_HASH_NAME_A         0x100
#define MPQ_HASH_NAME_B         0x200
#define MPQ_HASH_FILE_KEY       0x300
#define MPQ_HASH_KEY2_MIX       0x400

#define STORM_BUFFER_SIZE       0x500

//#define HASH_INDEX_MASK(ha) (ha->pHeader->dwHashTableSize ? (ha->pHeader->dwHashTableSize - 1) : 0)

static DWORD StormBuffer[STORM_BUFFER_SIZE];    // Buffer for the decryption engine

void InitializeMpqCryptography()
{
    static int  bMpqCryptographyInitialized = 0;
    DWORD dwSeed = 0x00100001;
    DWORD index1 = 0;
    DWORD index2 = 0;
    int   i;

    // Initialize the decryption buffer.
    // Do nothing if already done.
    if(bMpqCryptographyInitialized == 0)
    {
        for(index1 = 0; index1 < 0x100; index1++)
        {
            for(index2 = index1, i = 0; i < 5; i++, index2 += 0x100)
            {
                DWORD temp1, temp2;

                dwSeed = (dwSeed * 125 + 3) % 0x2AAAAB;
                temp1  = (dwSeed & 0xFFFF) << 0x10;

                dwSeed = (dwSeed * 125 + 3) % 0x2AAAAB;
                temp2  = (dwSeed & 0xFFFF);

                StormBuffer[index2] = (temp1 | temp2);
            }
        }

        // Also register both MD5 and SHA1 hash algorithms
        //register_hash(&md5_desc);
        //register_hash(&sha1_desc);

        // Use LibTomMath as support math library for LibTomCrypt
        //ltc_mp = ltm_desc;    

        // Don't do that again
        bMpqCryptographyInitialized = 1;
    }
}

DWORD StormHash(unsigned char * pbKey, int len, DWORD dwHashType)
{
    InitializeMpqCryptography();

    DWORD  dwSeed1 = 0x7FED7FED;
    DWORD  dwSeed2 = 0xEEEEEEEE;
    DWORD  ch;

    while(len--)
    {
        ch = *pbKey++;

        dwSeed1 = StormBuffer[dwHashType + ch] ^ (dwSeed1 + dwSeed2);
        dwSeed2 = ch + dwSeed1 + dwSeed2 + (dwSeed2 << 5) + 3;
    }

    return dwSeed1;
}

void EncryptMpqBlock(void * pvDataBlock, DWORD dwLength, DWORD dwKey1)
{
    InitializeMpqCryptography();

    DWORD* DataBlock = (DWORD*)pvDataBlock;
    DWORD dwValue32;
    DWORD dwKey2 = 0xEEEEEEEE;

    // Round to DWORDs
    dwLength >>= 2;

    // Encrypt the data block at array of DWORDs
    DWORD i;
    for(i = 0; i < dwLength; i++)
    {
        // Modify the second key
        dwKey2 += StormBuffer[MPQ_HASH_KEY2_MIX + (dwKey1 & 0xFF)];

        dwValue32 = DataBlock[i];
        DataBlock[i] = DataBlock[i] ^ (dwKey1 + dwKey2);

        dwKey1 = ((~dwKey1 << 0x15) + 0x11111111) | (dwKey1 >> 0x0B);
        dwKey2 = dwValue32 + dwKey2 + (dwKey2 << 5) + 3;
    }
}

void DecryptMpqBlock(void * pvDataBlock, DWORD dwLength, DWORD dwKey1)
{
    InitializeMpqCryptography();

    DWORD* DataBlock = (DWORD*)pvDataBlock;
    DWORD dwValue32;
    DWORD dwKey2 = 0xEEEEEEEE;

    // Round to DWORDs
    dwLength >>= 2;

    // Decrypt the data block at array of DWORDs
    DWORD i;
    for(i = 0; i < dwLength; i++)
    {
        // Modify the second key
        dwKey2 += StormBuffer[MPQ_HASH_KEY2_MIX + (dwKey1 & 0xFF)];
        
        DataBlock[i] = DataBlock[i] ^ (dwKey1 + dwKey2);
        dwValue32 = DataBlock[i];

        dwKey1 = ((~dwKey1 << 0x15) + 0x11111111) | (dwKey1 >> 0x0B);
        dwKey2 = dwValue32 + dwKey2 + (dwKey2 << 5) + 3;
    }
}




uint32_t jenkins_hashlittle( const void *key, size_t length, uint32_t initval)
{
#define jenkins_rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))
#define jenkins_mix(a,b,c) \
{ \
  a -= c;  a ^= jenkins_rot(c, 4);  c += b; \
  b -= a;  b ^= jenkins_rot(a, 6);  a += c; \
  c -= b;  c ^= jenkins_rot(b, 8);  b += a; \
  a -= c;  a ^= jenkins_rot(c,16);  c += b; \
  b -= a;  b ^= jenkins_rot(a,19);  a += c; \
  c -= b;  c ^= jenkins_rot(b, 4);  b += a; \
}
#define jenkins_final(a,b,c) \
{ \
  c ^= b; c -= jenkins_rot(b,14); \
  a ^= c; a -= jenkins_rot(c,11); \
  b ^= a; b -= jenkins_rot(a,25); \
  c ^= b; c -= jenkins_rot(b,16); \
  a ^= c; a -= jenkins_rot(c,4);  \
  b ^= a; b -= jenkins_rot(a,14); \
  c ^= b; c -= jenkins_rot(b,24); \
}

  uint32_t a,b,c;                                          /* internal state */

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)length) + initval;

    const uint8_t *k = (const uint8_t *)key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      a += ((uint32_t)k[1])<<8;
      a += ((uint32_t)k[2])<<16;
      a += ((uint32_t)k[3])<<24;
      b += k[4];
      b += ((uint32_t)k[5])<<8;
      b += ((uint32_t)k[6])<<16;
      b += ((uint32_t)k[7])<<24;
      c += k[8];
      c += ((uint32_t)k[9])<<8;
      c += ((uint32_t)k[10])<<16;
      c += ((uint32_t)k[11])<<24;
      jenkins_mix(a,b,c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=((uint32_t)k[11])<<24;
    case 11: c+=((uint32_t)k[10])<<16;
    case 10: c+=((uint32_t)k[9])<<8;
    case 9 : c+=k[8];
    case 8 : b+=((uint32_t)k[7])<<24;
    case 7 : b+=((uint32_t)k[6])<<16;
    case 6 : b+=((uint32_t)k[5])<<8;
    case 5 : b+=k[4];
    case 4 : a+=((uint32_t)k[3])<<24;
    case 3 : a+=((uint32_t)k[2])<<16;
    case 2 : a+=((uint32_t)k[1])<<8;
    case 1 : a+=k[0];
             break;
    case 0 : return c;
    }

  jenkins_final(a,b,c);
  return c;
}

