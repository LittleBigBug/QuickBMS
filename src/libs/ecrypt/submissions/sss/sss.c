/* $Id: ecrypt-sss.c 388 2005-04-28 21:04:09Z mwp $ */
/* Ecrypt API wrapper for SSS */

/*
THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND AGAINST
INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ecrypt-ssyn-ae.h"

void
ECRYPT_init(void)
{
}

void
ECRYPT_keysetup(ECRYPT_ctx* ctx, const u8* key, u32 keysize, u32 ivsize)
{
	sss_key(&ctx->ctx, (UCHAR *)key, keysize);
	ctx->ivsize = ivsize;
}

void
ECRYPT_ivsetup(ECRYPT_ctx* ctx, u8 *previous, const u8* iv)
{
	sss_nonce(&ctx->ctx, (UCHAR *)iv, ctx->ivsize);
}

void
ECRYPT_encrypt_bytes(ECRYPT_ctx* ctx, const u8 *previous,
	const u8* plaintext, u8* ciphertext, u32 msglen)
{
	memmove(ciphertext, plaintext, msglen);
	sss_enconly(&ctx->ctx, (UCHAR *)ciphertext, msglen);
}

void
ECRYPT_decrypt_bytes(ECRYPT_ctx* ctx, const u8 *previous,
	const u8* ciphertext, u8* plaintext, u32 msglen)
{
	memmove(plaintext, ciphertext, msglen);
	sss_deconly(&ctx->ctx, (UCHAR *)plaintext, msglen);
}

void
ECRYPT_AE_keysetup(ECRYPT_AE_ctx* ctx, const u8* key, u32 keysize,
	u32 ivsize, u32 macsize)
{
	sss_key(&ctx->ctx.ctx, (UCHAR *)key, keysize);
	ctx->ctx.ivsize = ivsize;
	ctx->macctx.macsize = macsize;
}

void
ECRYPT_AE_ivsetup(ECRYPT_AE_ctx* ctx, u8 *previous, const u8* iv)
{
	sss_nonce(&ctx->ctx.ctx, (UCHAR *)iv, ctx->ctx.ivsize);
}

void
ECRYPT_AE_encrypt_bytes(ECRYPT_AE_ctx* ctx, const u8 *previous,
	const u8* plaintext, u8* ciphertext, u32 msglen)
{
	memmove(ciphertext, plaintext, msglen);
	sss_encrypt(&ctx->ctx.ctx, (UCHAR *)ciphertext, msglen);
}

void
ECRYPT_AE_decrypt_bytes(ECRYPT_AE_ctx* ctx, const u8 *previous,
	const u8* ciphertext, u8* plaintext, u32 msglen)
{
	memmove(plaintext, ciphertext, msglen);
	sss_decrypt(&ctx->ctx.ctx, (UCHAR *)plaintext, msglen);
}

void
ECRYPT_AE_authenticate_bytes(ECRYPT_AE_ctx* ctx, u8 *previous,
	const u8* aad, u32 aadlen)
{
	sss_maconly(&ctx->ctx.ctx, (UCHAR *)aad, aadlen);
}

void
ECRYPT_AE_finalize(ECRYPT_AE_ctx* ctx, u8* mac)
{
	sss_finish(&ctx->ctx.ctx, (UCHAR *)mac, ctx->macctx.macsize);
}

#ifdef TEST_MAIN
/*
 * Test API.
 */
int
main()
{
	const char *key = "test key 128bits";
	const char nonce[] = {0, 0, 0, 0};

	const char zero_vector[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	#define ENCRYPT_BYTES 1000020
	const char encrypt_start_vector[] = {
		0xf0, 0x94, 0x31, 0xe6, 0x15, 0x99, 0xec, 0xed, 0x55, 0xe5,
		0x02, 0x31, 0xf2, 0x5e, 0x98, 0xb3, 0xf8, 0x49, 0xb7, 0xfd, 
	};
	const char encrypt_end_vector [] = {
		0x0c, 0xae, 0x2e, 0x83, 0xd2, 0x3d, 0x1a, 0xc7, 0xa5, 0xac,
		0x18, 0xb5, 0x05, 0x58, 0xde, 0x32, 0xa8, 0x5f, 0xcc, 0xae, 
	};


	#define MAC_BYTES 100
	const char mac_vector [] = {
		0xf5, 0x00, 0x74, 0xe9, 0xd7, 0xf9, 0x14, 0x47, 0x28, 0x0e,
		0x5d, 0x96, 0xfc, 0xa0, 0xd3, 0x68, 0xb4, 0x66, 0xf7, 0x4c, 
	};
	const char encmac_vector [] = {
		0x13, 0x7a, 0x97, 0x1e, 0x09, 0x9c, 0x0e, 0x76, 0x27, 0x1b,
		0x31, 0x35, 0x97, 0x26, 0x5a, 0x09, 0x7c, 0x86, 0x2f, 0xf7, 
	};

	ECRYPT_ctx ctx;
	ECRYPT_AE_ctx ae_ctx;
	char buffer[ENCRYPT_BYTES];
	char mac[sizeof(mac_vector)];
	int failures = 0;

#	define REPORT(name, predicate) (\
		(predicate)\
			? (printf ("%s: passed\n", (name)), 1)\
			: (printf ("%s: failed\n", (name)), 0)\
	)

	ECRYPT_init();

	/* encryption only */
	ECRYPT_keysetup(&ctx, key, strlen(key), sizeof(nonce));
	ECRYPT_ivsetup(&ctx, 0, nonce);
	memset(buffer, 0, ENCRYPT_BYTES);
	ECRYPT_encrypt_bytes (&ctx, 0, buffer, buffer, ENCRYPT_BYTES);
	failures += !REPORT ("encrypt only",
		memcmp (buffer, encrypt_start_vector,
			sizeof(encrypt_start_vector)) == 0
		&& memcmp (buffer + ENCRYPT_BYTES - sizeof(encrypt_end_vector),
			encrypt_end_vector, sizeof(encrypt_end_vector)) == 0);

	/* decryption only */
	ECRYPT_ivsetup(&ctx, 0, nonce);
	ECRYPT_decrypt_bytes (&ctx, 0, buffer, buffer, ENCRYPT_BYTES);
	failures += !REPORT ("decrypt only",
		memcmp (buffer, zero_vector, sizeof(zero_vector)) == 0
		&& memcmp (buffer + ENCRYPT_BYTES - sizeof(zero_vector),
			zero_vector, sizeof(zero_vector)) == 0);

	/* mac only */
	ECRYPT_AE_keysetup(&ae_ctx, key, strlen(key), sizeof(nonce),
		sizeof(mac));
	ECRYPT_AE_ivsetup(&ae_ctx, 0, nonce);
	memset(buffer, 0, MAC_BYTES);
	ECRYPT_AE_authenticate_bytes(&ae_ctx, 0, buffer, MAC_BYTES);
	ECRYPT_AE_finalize(&ae_ctx, mac);
	failures += !REPORT ("mac only",
		memcmp (mac, mac_vector, sizeof(mac_vector)) == 0);

	/* encrypt and mac */
	ECRYPT_AE_ivsetup(&ae_ctx, 0, nonce);
	memset(buffer, 0, MAC_BYTES);
	ECRYPT_AE_encrypt_bytes(&ae_ctx, 0, buffer, buffer, MAC_BYTES);
	ECRYPT_AE_finalize(&ae_ctx, mac);
	failures += !REPORT ("encrypt+mac",
		memcmp (buffer, encrypt_start_vector,
			sizeof(encrypt_start_vector)) == 0
		&& memcmp (mac, encmac_vector, sizeof(encmac_vector)) == 0);

	/* decrypt and mac */
	ECRYPT_AE_ivsetup(&ae_ctx, 0, nonce);
	ECRYPT_AE_decrypt_bytes(&ae_ctx, 0, buffer, buffer, MAC_BYTES);
	ECRYPT_AE_finalize(&ae_ctx, mac);
	failures += !REPORT ("decrypt+mac",
		memcmp (buffer, zero_vector, sizeof(zero_vector)) == 0
		&& memcmp (mac, encmac_vector, sizeof(encmac_vector)) == 0);

	return (failures);
}
#endif /* TEST_MAIN */
