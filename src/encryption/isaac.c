// modified by Luigi Auriemma

/*************************************************
* ISAAC Source File                              *
* (C) 1999-2005 The Botan Project                *
*************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "isaac.h"



static void ISAAC__xor_buf(uint8_t *out,
  const uint8_t *in,
  const uint8_t *in2,
  size_t length)
  {
    size_t i;
  for(i = 0; i != length; ++i)
  {
  out[i] = in[i] ^ in2[i];
  }
}

static uint32_t ISAAC__make_uint32_t(uint8_t i0, uint8_t i1, uint8_t i2, uint8_t i3)
  {
  return (((uint32_t)(i0) << 24) |
  ((uint32_t)(i1) << 16) |
  ((uint32_t)(i2) << 8) |
  ((uint32_t)(i3)));
}

static uint8_t ISAAC__get_byte(size_t byte_num, uint32_t input)
{
  return (
  input >> (((~byte_num)&(sizeof(input)-1)) << 3)
  );
}



/*************************************************
* Clear memory of sensitive data                 *
*************************************************/
static void ISAAC__clear(ISAAC_ctx_t *ISAAC_ctx)
   {
   memset(ISAAC_ctx->state, 0, sizeof(ISAAC_ctx->state));
   memset(ISAAC_ctx->buffer, 0, sizeof(ISAAC_ctx->buffer));
   ISAAC_ctx->A = ISAAC_ctx->B = ISAAC_ctx->C = ISAAC_ctx->position = 0;
   }

/*************************************************
* Generate ISAAC Cipher Stream                   *
*************************************************/
static void ISAAC__generate(ISAAC_ctx_t *ISAAC_ctx)
   {
   ISAAC_ctx->C++; ISAAC_ctx->B += ISAAC_ctx->C;
   uint32_t X, Y;

   uint32_t j;
   for(j = 0; j != 256; j += 4)
      {
      ISAAC_ctx->A ^= (ISAAC_ctx->A << 13);
      X = ISAAC_ctx->state[j];
      ISAAC_ctx->A += ISAAC_ctx->state[(j+128) % 256];
      ISAAC_ctx->state[j] = Y = ISAAC_ctx->state[(X >> 2) & 0xFF] + ISAAC_ctx->A + ISAAC_ctx->B;
      X = ISAAC_ctx->B = ISAAC_ctx->state[(Y >> 10) & 0xFF] + X;
      ISAAC_ctx->buffer[4*j+ 0] = ISAAC__get_byte(0, X); ISAAC_ctx->buffer[4*j+ 1] = ISAAC__get_byte(1, X);
      ISAAC_ctx->buffer[4*j+ 2] = ISAAC__get_byte(2, X); ISAAC_ctx->buffer[4*j+ 3] = ISAAC__get_byte(3, X);

      ISAAC_ctx->A ^= (ISAAC_ctx->A >> 6);
      X = ISAAC_ctx->state[j+1];
      ISAAC_ctx->A += ISAAC_ctx->state[(j+129) % 256];
      ISAAC_ctx->state[j+1] = Y = ISAAC_ctx->state[(X >> 2) & 0xFF] + ISAAC_ctx->A + ISAAC_ctx->B;
      X = ISAAC_ctx->B = ISAAC_ctx->state[(Y >> 10) & 0xFF] + X;
      ISAAC_ctx->buffer[4*j+ 4] = ISAAC__get_byte(0, X); ISAAC_ctx->buffer[4*j+ 5] = ISAAC__get_byte(1, X);
      ISAAC_ctx->buffer[4*j+ 6] = ISAAC__get_byte(2, X); ISAAC_ctx->buffer[4*j+ 7] = ISAAC__get_byte(3, X);

      ISAAC_ctx->A ^= (ISAAC_ctx->A << 2);
      X = ISAAC_ctx->state[j+2];
      ISAAC_ctx->A += ISAAC_ctx->state[(j+130) % 256];
      ISAAC_ctx->state[j+2] = Y = ISAAC_ctx->state[(X >> 2) & 0xFF] + ISAAC_ctx->A + ISAAC_ctx->B;
      X = ISAAC_ctx->B = ISAAC_ctx->state[(Y >> 10) & 0xFF] + X;
      ISAAC_ctx->buffer[4*j+ 8] = ISAAC__get_byte(0, X); ISAAC_ctx->buffer[4*j+ 9] = ISAAC__get_byte(1, X);
      ISAAC_ctx->buffer[4*j+10] = ISAAC__get_byte(2, X); ISAAC_ctx->buffer[4*j+11] = ISAAC__get_byte(3, X);

      ISAAC_ctx->A ^= (ISAAC_ctx->A >> 16);
      X = ISAAC_ctx->state[j+3];
      ISAAC_ctx->A += ISAAC_ctx->state[(j+131) % 256];
      ISAAC_ctx->state[j+3] = Y = ISAAC_ctx->state[(X >> 2) & 0xFF] + ISAAC_ctx->A + ISAAC_ctx->B;
      X = ISAAC_ctx->B = ISAAC_ctx->state[(Y >> 10) & 0xFF] + X;
      ISAAC_ctx->buffer[4*j+12] = ISAAC__get_byte(0, X); ISAAC_ctx->buffer[4*j+13] = ISAAC__get_byte(1, X);
      ISAAC_ctx->buffer[4*j+14] = ISAAC__get_byte(2, X); ISAAC_ctx->buffer[4*j+15] = ISAAC__get_byte(3, X);
      }
   ISAAC_ctx->position = 0;
   }

/*************************************************
* Combine Cipher Stream with Message             *
*************************************************/
void ISAAC__cipher(ISAAC_ctx_t *ISAAC_ctx, const uint8_t *in, uint8_t *out, uint32_t length)
   {
   while(length >= sizeof(ISAAC_ctx->buffer) - ISAAC_ctx->position)
      {
      ISAAC__xor_buf(out, in, ISAAC_ctx->buffer + ISAAC_ctx->position, sizeof(ISAAC_ctx->buffer) - ISAAC_ctx->position);
      length -= (sizeof(ISAAC_ctx->buffer) - ISAAC_ctx->position);
      in += (sizeof(ISAAC_ctx->buffer) - ISAAC_ctx->position);
      out += (sizeof(ISAAC_ctx->buffer) - ISAAC_ctx->position);
      ISAAC__generate(ISAAC_ctx);
      }
   ISAAC__xor_buf(out, in, ISAAC_ctx->buffer + ISAAC_ctx->position, length);
   ISAAC_ctx->position += length;
   }

/*************************************************
* ISAAC Key Schedule                             *
*************************************************/
void ISAAC__key(ISAAC_ctx_t *ISAAC_ctx, const uint8_t *key, uint32_t length)
   {
   uint32_t *state = ISAAC_ctx->state;
   ISAAC__clear(ISAAC_ctx);

   uint32_t j;
   for(j = 0; j != 256; j++) {
      state[j] = ISAAC__make_uint32_t(key[(4*j  )%length], key[(4*j+1)%length],
                             key[(4*j+2)%length], key[(4*j+3)%length]);
   }

   uint32_t A = 0x1367DF5A, B = 0x95D90059, C = 0xC3163E4B, D = 0x0F421AD8,
          E = 0xD92A4A78, F = 0xA51A3C49, G = 0xC4EFEA1B, H = 0x30609119;

   for(j = 0; j != 2; j++)
      {
      uint32_t l;
      for(l = 0; l != 256; l += 8)
         {
         A += state[l  ]; B += state[l+1]; C += state[l+2]; D += state[l+3];
         E += state[l+4]; F += state[l+5]; G += state[l+6]; H += state[l+7];
         A ^= (B << 11); D += A; B += C; B ^= (C >>  2); E += B; C += D;
         C ^= (D <<  8); F += C; D += E; D ^= (E >> 16); G += D; E += F;
         E ^= (F << 10); H += E; F += G; F ^= (G >>  4); A += F; G += H;
         G ^= (H <<  8); B += G; H += A; H ^= (A >>  9); C += H; A += B;
         state[l  ] = A; state[l+1] = B; state[l+2] = C; state[l+3] = D;
         state[l+4] = E; state[l+5] = F; state[l+6] = G; state[l+7] = H;
         }
      }
   ISAAC__generate(ISAAC_ctx);
   ISAAC__generate(ISAAC_ctx);
   }

