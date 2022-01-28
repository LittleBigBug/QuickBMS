#include "pomaranch.h"

#include <string.h>

void ECRYPT_init(void)
{
	u16 i, j;

	/* calculate binary weight of all 8-bit vectors */
	wt_mod2[0] = 0;
	for (i=1; i < 129; i <<= 1)
		for (j=0; j < i; ++j) wt_mod2[i+j] = 1 - wt_mod2[j];
}

void ECRYPT_keysetup(ECRYPT_ctx *ctx, const u8 *key8, u32 keysize, u32 ivsize)
{
	int i;

	/* privide the ivsize for the ECRYPT_ivsetup function */
	ctx->IV_size = ivsize;

	/* copy the key to the ctx structure, 2-byte words */
	for (i = 0; i < 8; ++i)
	  ctx->Key[i] = U8TO16_LITTLE(key8 + 2 * i);
}

void ECRYPT_ivsetup(ECRYPT_ctx* ctx, const u8* iv)
{
	u8 JC,				/* Jump Control bit */
	   i, j;
	u16 IVpart, ui1, ui2, ui3;
	u32 Nextbit = 0;

	/* load the registers with binary expansion of pi */
    for (i=0; i < 9; ++i) ctx->state[i] = pi[i];

	/* initialization phase */
	for (i=0; i < 128; ++i)
	{
		JC = ctx->state[8] & mask9;
        for (j=0; j < 8; ++j)
        {
			/* calculate the JC out */
            ui1 = KeyMap(ctx->state[j], ctx->Key[j]);

			/* update the state */
			ui2 = ctx->state[j] & fdk_mask;
			ui3 = ctx->state[j] & F_mask[0];
			ctx->state[j] <<= 1;
			if (wt_mod2[ui2 & 0x00FF] ^ wt_mod2[(ui2 & 0xFF00)>>8] ^ JC) ++(ctx->state[j]);
			ctx->state[j] ^= ui3;

			JC = F[ui1];
		}
		/* take care of the 9th section */
		ui2 = ctx->state[8] & fdk_mask;
		ui3 = ctx->state[8] & F_mask[0];
		ctx->state[8] <<= 1;
		if (wt_mod2[ui2 & 0x00FF] ^ wt_mod2[(ui2 & 0xFF00)>>8] ^ JC) ++(ctx->state[8]);
		ctx->state[8] ^= ui3;
	}

	/* cyclically add the IV bits to the state */
	for (i=0; i < 9; ++i)
	{
		IVpart = 0;
		if ((Nextbit&7) <= 2)
		{ /* Need only add bits from this and the next iv-byte */
			IVpart = (((u16)iv[Nextbit>>3])<<(6+(Nextbit&7))) |
					 (((u16)iv[((Nextbit>>3)+1)%(ctx->IV_size>>3)])>>(2-(Nextbit&7)));
			Nextbit = (Nextbit+14)%(ctx->IV_size);
		}
		else
		{ /* Need to add bits from three consecutive iv-bytes */
			IVpart = (((u16)iv[Nextbit>>3])<<(6+(Nextbit&7))) |
					 (((u16)iv[((Nextbit>>3)+1)%(ctx->IV_size>>3)])<<((Nextbit&7)-2)) |
					 (((u16)iv[((Nextbit>>3)+2)%(ctx->IV_size>>3)])>>(10-(Nextbit&7)));
			Nextbit = (Nextbit+14)%(ctx->IV_size);
		}
		/* Nextbit is the number for the next bit of the IV to be used as */
		/* the least signinficant bit to be added to the next state       */

		IVpart &= 0x3fff; /* Only 14-bit state */
		ctx->state[i] ^= IVpart;
	}

	for (i=0; i < 9; ++i)
		if (ctx->state[i] == 0) ++(ctx->state[i]);

	/* runup phase */
	for (i=0; i < 128; ++i)
	{
		JC = 0;
		for (j=0; j < 8; ++j)
		{
			/* calculate the JC out */
			ui1 = KeyMap(ctx->state[j], ctx->Key[j]);

			/* update the state */
			state_upd(ctx->state+j, F_mask[JC]);
			JC ^= F[ui1];
		}
		/* take care of the 9th section */
		state_upd(ctx->state+8, F_mask[JC]);
	}
}

void ECRYPT_process_bytes(int action, ECRYPT_ctx* ctx, const u8* input, u8* output, u32 msglen)
{
	u8 JC,					/* Jump Control bit */
	   keystr,				/* bit of the key stream */
	   keystr_byte,			/* byte of the key stream */
	   j, k;
	u16 ui1;
	u32 i;

	/* key stream generation phase */
	for (i=0; i < msglen; ++i)
	{
		keystr_byte = 0;
		for (k=0; k < 8; ++k)
		{
			keystr_byte <<= 1;
			JC = 0; keystr = 0;
			for (j=0; j < 8; ++j)
			{
				/* get the key stream contribution bit */
				if ((ctx->state[j] & out_mask) != 0) keystr ^= 1;

				/* calculate the JC out */
				ui1 = KeyMap(ctx->state[j], ctx->Key[j]);

				/* update the state */
				state_upd(ctx->state+j, F_mask[JC]);
				JC ^= F[ui1];
			}
			/* take care of the 9th section */
			if ((ctx->state[8] & out_mask) != 0) keystr ^= 1;
			state_upd(ctx->state+8, F_mask[JC]);
			if (keystr) ++keystr_byte;
		}
		/* encrypt/decrypt the byte */
		output[i] = input[i]^keystr_byte;
	}
}

void ECRYPT_keystream_bytes(ECRYPT_ctx* ctx, u8* keystream, u32 length)
{
	u8 JC,					/* Jump Control bit */
	   keystr,				/* bit of the key stream */
	   j, k;
	u16 ui1;
	u32 i;

	/* key stream generation phase */
	for (i=0; i < length; ++i)
	{
		keystream[i] = 0;
		for (k=0; k < 8; ++k)
		{
			keystream[i] <<= 1;
			JC = 0; keystr = 0;
			for (j=0; j < 8; ++j)
			{
				/* get the key stream contribution bit */
				if ((ctx->state[j] & out_mask) != 0) keystr ^= 1;

				/* calculate the JC out */
				ui1 = KeyMap(ctx->state[j], ctx->Key[j]);

				/* update the state */
				state_upd(ctx->state+j, F_mask[JC]);
				JC ^= F[ui1];
			}
			/* take care of the 9th section */
			if ((ctx->state[8] & out_mask) != 0) keystr ^= 1;
			state_upd(ctx->state+8, F_mask[JC]);
			if (keystr) ++keystream[i];
		}
	}
}

/* update state in 2-byte state using F_mask and fdk_mask in the generation mode */
void state_upd(u16 *state, const u16 F_mask)
{
	u16 ui1, ui2;

	ui1 = *state & fdk_mask;
	ui2 = *state & F_mask;
	*state <<= 1;
	if (wt_mod2[ui1 & 0x00FF] ^ wt_mod2[(ui1 & 0xFF00)>>8]) ++(*state);
	*state ^= ui2;
}

/* calculate KeyMap for state using tap_mask and Key (return before the 7-to-1 function) */
u16 KeyMap(const u16 state, const u16 Key)
{
	u16 tmp16;

	tmp16 = ((state & tap_mask[0])>>2)^((state & tap_mask[1])>>1);
	tmp16 ^= Key & 0x1FF;
	tmp16 = S[tmp16];
	tmp16 ^= (Key & 0xFE00)>>9;
	return(tmp16);
}

#ifndef ECRYPT_API

#include <stdio.h>

main()
{
	u8 Key[16] = {0xAF, 0x0, 0xAB, 0x3D, 0xAE, 0xBB, 0x71, 0xCC, 0xF3, 0x0, 0x3A, 0xF9, 0xBA, 0xBB, 0xC0, 0xC7};
	u8 IV[14] = {0x80, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0};
//	u8 IV[14] = {0xAB, 0x71, 0x3A, 0x70,  0x71, 0x20, 0x3A, 0xBA, 0x51, 0xD7, 0x02,  0xA8, 0xE1, 0x8C};

	u8 plaintext[0x00000400ul],				/* plain text */
	   ciphertext[0x00000400ul],			/* cipher text */
	   keystream[0x00000400ul];				/* key stream */

	u32 keysize = ECRYPT_KEYSIZE(0),		/* key size in bits = 128 */
		ivsize = ECRYPT_IVSIZE(7);			/* IV size in bits */

	ECRYPT_ctx cc;							/* states of the registers */
	ECRYPT_ctx *ctx = &cc;

	u32 msglen=0x00000400ul;				/* message length in bytes */

	u32 i;

	ECRYPT_init();

/* initialization phase */
	ECRYPT_keysetup(ctx, Key, keysize, ivsize);

/* IV loading phase */
	ECRYPT_ivsetup(ctx, IV);

/* generate key stream */
	ECRYPT_keystream_bytes(ctx, keystream, msglen);
	for (i=0; i < msglen; ++i) printf("%X", keystream[i]);

/* encrypt the sequence of bytes */
	ECRYPT_encrypt_bytes(ctx, plaintext, ciphertext, msglen);

/* decrypt the sequence of bytes */
	ECRYPT_decrypt_bytes(ctx, ciphertext, plaintext, msglen);
}

#endif
