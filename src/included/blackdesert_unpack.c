// modified by Luigi Auriemma

// dumped by Ekey
// http://forum.xentax.com/viewtopic.php?p=89364#p89364
// http://pastebin.com/0YDTYsLb
// http://pastebin.com/jhDSfpKL

/*
a1  in
a2  out
a3  size
a4  out base, the function is meant for temporary/chunked decompression
a1_size allows the raw decompression if different than 0
*/
u32 blackdesert_unpack_core(u8 *a1, u8 *a2, u32 a3, u8 *a4, u32 a1_size)
{
    static const u8 blackdesert_unpack_dwTable[] = {
        0x4, 0x0, 0x1, 0x0, 0x2, 0x0, 0x1, 0x0, 0x3, 0x0, 0x1, 0x0, 0x2, 0x0, 0x1, 0x0
    };

  u8 *v4; // ebp@1
  u8 *v5; // edi@1
  signed int v6; // edx@1
  u8 *v7; // esi@1
  u32 v8; // ebx@1
  int v9; // eax@2
  u32 v10; // edx@4
  u8 *v11; // eax@4
  u32 v12; // edx@9
  u32 v13; // ecx@11
  int v14; // edx@11
  u32 v15; // ecx@17
  u8 *v16; // ebx@19
  u32 v17; // edi@22
  u8 *v18; // ecx@22
  int v19; // ebx@22
  int v20; // ecx@26
  u8 *v22; // eax@29
  u32 v23; // [sp+10h] [bp-8h]@10
  u8 *v24; // [sp+14h] [bp-4h]@4
  u8 *v25; // [sp+1Ch] [bp+4h]@1

  v4 = a2;
  v5 = a1;
  v8 = 1;
  v25 = a3 + a2 - 1;

// simple way to allow raw decompression: specify the input size
if(!a1_size) {
  v6 = (char)((*(u8 *)a1 & 2) - 2) != 0 ? 1 : 4;
  v7 = a1 + 2 * v6 + 1;
  if ( ((char)((*(u8 *)a1 & 2) - 2) != 0 ? 0 : 3) > 3u )
    v9 = 0;
  else {
    if(v6 == 1) v9 = *(u8 *)(a1 + 1);
    else v9 = *(u32 *)(a1 + 1);
  }
} else {
    v6 = 4;
    v7 = a1;
    v9 = a1_size;
}
  v10 = v9 & (0xFFFFFFFFu >> 8 * (4 - v6));
  v11 = v10 + v5 - 1;
  v24 = v10 + v5 - 1;
  while ( 1 )
  {
    while ( 1 )
    {
      if ( v8 == 1 )
      {
        if ( v7 + 3 > v11 )
          return -1;
        v8 = *(u32 *)v7;
        v7 += 4;
      }
      if ( v7 + 3 > v11 )
        return -2;
      v12 = *(u32 *)v7;
      if ( !(v8 & 1) )
        break;
      v23 = v8 >> 1;
      if ( v12 & 3 )
      {
        if ( v12 & 2 )
        {
          if ( v12 & 1 )
          {
            if ( (v12 & 0x7F) == 3 )
            {
              v13 = v12 >> 15;
              v14 = ((v12 >> 7) & 0xFF) + 3;
              v7 += 4;
            }
            else
            {
              v15 = v12 >> 7;
              v13 = v15 & 0x1FFFF;
              v14 = ((v12 >> 2) & 0x1F) + 2;
              v7 += 3;
            }
          }
          else
          {
            v13 = (u16)v12 >> 6;
            v14 = ((v12 >> 2) & 0xF) + 3;
            v7 += 2;
          }
        }
        else
        {
          v13 = (u16)v12 >> 2;
          v14 = 3;
          v7 += 2;
        }
      }
      else
      {
        v13 = (u8)v12 >> 2;
        v14 = 3;
        ++v7;
      }
      v16 = v4 - v13;
      if ( v4 - v13 < a4 || v16 > v4 - 3 || v14 > v25 - v4 - 3 )
        return -3;
      v17 = 0;
      v18 = v4;
      v19 = v16 - v4;
      do
      {
        *(u32 *)v18 = *(u32 *)(v19 + v18);
        v17 += 3;
        v18 += 3;
      }
      while ( v17 < v14 );
      v8 = v23;
      v11 = v24;
      v4 += v14;
    }
    if ( v4 >= v25 - 10 )
      break;
    v20 = blackdesert_unpack_dwTable[v8 & 0xF];
    *(u32 *)v4 = v12;
    v8 >>= v20;
    v4 += v20;
    v7 += v20;
  }
  if ( v4 <= v25 )
  {
    v22 = v11 + 1;
    while ( 1 )
    {
      if ( v8 == 1 )
      {
        v7 += 4;
        v8 = 0x80000000;
      }
      if ( v7 >= v22 )
        break;
      *(u8 *)v4++ = *(u8 *)v7++;
      v8 >>= 1;
      if ( v4 > v25 )
        return v4 - a2;
    }
    return -4;
  }
  return v4 - a2;
}



u32 __cdecl blackdesert_unpack(u8 *a1, void *a2 /*, u8 *a3*/)
{

  signed int v3; // edx@1
  char v4; // bl@1
  int v5; // edi@2
  int v6; // esi@4
  //int v7; // edi@7
  int result; // eax@11
  //unsigned int v9; // edi@12
  int v10; // [sp+10h] [bp-4h]@5
    char t;

  t = (char)((*(u8 *)a1 & 2) - 2);
  v4 = *(u8 *)a1;
  v3 = t != 0 ? 1 : 4;
  if ( (t != 0 ? 0 : 3) > 3u )
    v5 = 0;
  else {
    if(v3 == 1) v5 = *(u8 *)(a1 + v3 + 1);
    else v5 = *(u32 *)(a1 + v3 + 1);
  }
  //v6 = v5 & (0xFFFFFFFFu >> -8 * (t != 0 ? -3 : 0));
  v6 = v5 & (0xFFFFFFFFu >> 8 * (4 - v3));

  if ( (t != 0 ? 0 : 3) > 3u )
    v10 = 0;
  else {
    if(v3 == 1) v10 = *(u8 *)(a1 + v3 + 1);
    else v10 = *(u32 *)(a1 + v3 + 1);
  }
  result = v10; // ?

/*
  v7 = *(u32 *)(a3 + 1000000);
  if ( (v10 & (0xFFFFFFFFu >> 8 * (4 - v3))) - 1 + v7 < 0xF4240 )
  {
    v9 = a3 + v7;
    if ( v4 & 1 )
      v6 = blackdesert_unpack_core(a1, v9, v6, a3, 0);
    else
      memcpy(v9, a1 + 2 * v3 + 1, v6);
    memcpy(a2, v9, v6);
    *(u32 *)(a3 + 1000000) += v6;
    result = v6;
  }
  else
*/
  {
    if ( v4 & 1 )
      v6 = blackdesert_unpack_core(a1, a2, v6, a2, 0);
    else
      memcpy(a2, a1 + 2 * v3 + 1, v6);
    result = v6;
    //*(u32 *)(a3 + 1000000) = 0;
  }
  return result;
}
