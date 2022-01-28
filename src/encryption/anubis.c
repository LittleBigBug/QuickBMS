/**
 * <b>The Anubis block cipher.</b>
 *
 * <P>
 * <b>References</b>
 *
 * <P>
 * The Anubis algorithm was developed by
 * <a href="mailto:pbarreto@scopus.com.br">Paulo S. L. M. Barreto</a> and
 * <a href="mailto:vincent.rijmen@esat.kuleuven.ac.be">Vincent Rijmen</a>.
 *
 * See
 *		P.S.L.M. Barreto, V. Rijmen,
 *		``The Anubis block cipher,''
 *		ANUBIS submission, 2000.
 * 
 * @author	Paulo S.L.M. Barreto
 * @author	Vincent Rijmen.
 *
 * @version 1.0 (2000.09.29)
 * 
 * =============================================================================
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>

/* Definition of minimum-width integer types
 * 
 * u8   -> unsigned integer type, at least 8 bits, equivalent to unsigned char
 * u16  -> unsigned integer type, at least 16 bits
 * u32  -> unsigned integer type, at least 32 bits
 *
 * s8, s16, s32  -> signed counterparts of u8, u16, u32
 *
 * Always use macro's T8(), T16() or T32() to obtain exact-width results,
 * i.e., to specify the size of the result of each expression.
 */

typedef signed char s8;
typedef unsigned char u8;

#if UINT_MAX >= 4294967295UL

typedef signed short s16;
typedef signed int s32;
typedef unsigned short u16;
typedef unsigned int u32;

#define ONE32   0xffffffffU

#else

typedef signed int s16;
typedef signed long s32;
typedef unsigned int u16;
typedef unsigned long u32;

#define ONE32   0xffffffffUL

#endif

#define ONE8    0xffU
#define ONE16   0xffffU

#define T8(x)   ((x) & ONE8)
#define T16(x)  ((x) & ONE16)
#define T32(x)  ((x) & ONE32)

/*
 * If you want 64-bit values, uncomment the following lines; this
 * reduces portability.
 */
/*
  #if ((1UL << 31) * 2UL) != 0UL
  typedef unsigned long u64;
  typedef signed long s64;
  #define ONE64   0xffffffffffffffffUL
  #else
  typedef unsigned long long u64;
  typedef signed long long s64;
  #define ONE64   0xffffffffffffffffULL
  #endif
  #define T64(x)  ((x) & ONE64)
*/
/*
 * Note: the test is used to detect native 64-bit architectures;
 * if the unsigned long is strictly greater than 32-bit, it is
 * assumed to be at least 64-bit. This will not work correctly
 * on (old) 36-bit architectures (PDP-11 for instance).
 *
 * On non-64-bit architectures, "long long" is used.
 */

/*
 * U8TO32_BIG(c) returns the 32-bit value stored in big-endian convention
 * in the unsigned char array pointed to by c.
 */
#define U8TO32_BIG(c)  (((u32)T8(*(c)) << 24) | ((u32)T8(*((c) + 1)) << 16) \
                       ((u32)T8(*((c) + 2)) << 8) | ((u32)T8(*((c) + 3))))

/*
 * U8TO32_LITTLE(c) returns the 32-bit value stored in little-endian convention
 * in the unsigned char array pointed to by c.
 */
#define U8TO32_LITTLE(c)  (((u32)T8(*(c))) | ((u32)T8(*((c) + 1)) << 8) \
                      (u32)T8(*((c) + 2)) << 16) | ((u32)T8(*((c) + 3)) << 24))

/*
 * U8TO32_BIG(c, v) stores the 32-bit-value v in big-endian convention
 * into the unsigned char array pointed to by c.
 */
#define U32TO8_BIG(c, v)    do { \
		u32 x = (v); \
		u8 *d = (c); \
		d[0] = T8(x >> 24); \
		d[1] = T8(x >> 16); \
		d[2] = T8(x >> 8); \
		d[3] = T8(x); \
	} while (0)

/*
 * U8TO32_LITTLE(c, v) stores the 32-bit-value v in little-endian convention
 * into the unsigned char array pointed to by c.
 */
#define U32TO8_LITTLE(c, v)    do { \
		u32 x = (v); \
		u8 *d = (c); \
		d[0] = T8(x); \
		d[1] = T8(x >> 8); \
		d[2] = T8(x >> 16); \
		d[3] = T8(x >> 24); \
	} while (0)

/*
 * ROTL32(v, n) returns the value of the 32-bit unsigned value v after
 * a rotation of n bits to the left. It might be replaced by the appropriate
 * architecture-specific macro.
 *
 * It evaluates v and n twice.
 *
 * The compiler might emit a warning if n is the constant 0. The result
 * is undefined if n is greater than 31.
 */
#define ROTL32(v, n)   (T32((v) << (n)) | ((v) >> (32 - (n))))

#include "anubis.h"

/*
 * Though Anubis is endianness-neutral, the encryption tables are listed
 * in BIG-ENDIAN format, which is adopted throughout this implementation
 * (but little-endian notation would be equally suitable if consistently
 * employed).
 */

static const u32 T0[256] = {
    0xa753a6f5U, 0xd3bb6bd0U, 0xe6d1bf6eU, 0x71e2d93bU,
    0xd0bd67daU, 0xac458acfU, 0x4d9a29b3U, 0x79f2f90bU,
    0x3a74e89cU, 0xc98f038cU, 0x913f7e41U, 0xfce5d732U,
    0x1e3c7844U, 0x478e018fU, 0x54a84de5U, 0xbd67cea9U,
    0x8c050a0fU, 0xa557aef9U, 0x7af4f501U, 0xfbebcb20U,
    0x63c69157U, 0xb86ddab7U, 0xdda753f4U, 0xd4b577c2U,
    0xe5d7b364U, 0xb37bf68dU, 0xc59733a4U, 0xbe61c2a3U,
    0xa94f9ed1U, 0x880d1a17U, 0x0c183028U, 0xa259b2ebU,
    0x3972e496U, 0xdfa35bf8U, 0x2952a4f6U, 0xdaa94fe6U,
    0x2b56acfaU, 0xa84d9ad7U, 0xcb8b0b80U, 0x4c982db5U,
    0x4b9631a7U, 0x224488ccU, 0xaa4992dbU, 0x244890d8U,
    0x4182199bU, 0x70e0dd3dU, 0xa651a2f3U, 0xf9efc32cU,
    0x5ab475c1U, 0xe2d9af76U, 0xb07dfa87U, 0x366cd8b4U,
    0x7dfae913U, 0xe4d5b762U, 0x3366ccaaU, 0xffe3db38U,
    0x60c09d5dU, 0x204080c0U, 0x08102030U, 0x8b0b161dU,
    0x5ebc65d9U, 0xab4b96ddU, 0x7ffee11fU, 0x78f0fd0dU,
    0x7cf8ed15U, 0x2c58b0e8U, 0x57ae41efU, 0xd2b96fd6U,
    0xdca557f2U, 0x6ddaa973U, 0x7efce519U, 0x0d1a342eU,
    0x53a651f7U, 0x94356a5fU, 0xc39b2bb0U, 0x2850a0f0U,
    0x274e9cd2U, 0x060c1814U, 0x5fbe61dfU, 0xad478ec9U,
    0x67ce814fU, 0x5cb86dd5U, 0x55aa49e3U, 0x48903dadU,
    0x0e1c3824U, 0x52a455f1U, 0xeac98f46U, 0x42841591U,
    0x5bb671c7U, 0x5dba69d3U, 0x3060c0a0U, 0x58b07dcdU,
    0x51a259fbU, 0x59b279cbU, 0x3c78f088U, 0x4e9c25b9U,
    0x3870e090U, 0x8a09121bU, 0x72e4d531U, 0x14285078U,
    0xe7d3bb68U, 0xc6913faeU, 0xdea15ffeU, 0x50a05dfdU,
    0x8e010203U, 0x9239724bU, 0xd1bf63dcU, 0x77eec12fU,
    0x933b764dU, 0x458a0983U, 0x9a29527bU, 0xce811f9eU,
    0x2d5ab4eeU, 0x03060c0aU, 0x62c49551U, 0xb671e293U,
    0xb96fdeb1U, 0xbf63c6a5U, 0x96316253U, 0x6bd6b167U,
    0x3f7efc82U, 0x070e1c12U, 0x1224486cU, 0xae4182c3U,
    0x40801d9dU, 0x3468d0b8U, 0x468c0589U, 0x3e7cf884U,
    0xdbab4be0U, 0xcf831b98U, 0xecc59752U, 0xcc851792U,
    0xc19f23bcU, 0xa15fbee1U, 0xc09d27baU, 0xd6b17fceU,
    0x1d3a744eU, 0xf4f5f702U, 0x61c2995bU, 0x3b76ec9aU,
    0x10204060U, 0xd8ad47eaU, 0x68d0bd6dU, 0xa05dbae7U,
    0xb17ffe81U, 0x0a14283cU, 0x69d2b96bU, 0x6cd8ad75U,
    0x499239abU, 0xfae9cf26U, 0x76ecc529U, 0xc49537a2U,
    0x9e214263U, 0x9b2b567dU, 0x6edca579U, 0x992f5e71U,
    0xc2992fb6U, 0xb773e695U, 0x982d5a77U, 0xbc65caafU,
    0x8f030605U, 0x85172e39U, 0x1f3e7c42U, 0xb475ea9fU,
    0xf8edc72aU, 0x11224466U, 0x2e5cb8e4U, 0x00000000U,
    0x254a94deU, 0x1c387048U, 0x2a54a8fcU, 0x3d7af48eU,
    0x050a141eU, 0x4f9e21bfU, 0x7bf6f107U, 0xb279f28bU,
    0x3264c8acU, 0x903d7a47U, 0xaf4386c5U, 0x19326456U,
    0xa35bb6edU, 0xf7f3fb08U, 0x73e6d137U, 0x9d274e69U,
    0x152a547eU, 0x74e8cd25U, 0xeec19f5eU, 0xca890f86U,
    0x9f234665U, 0x0f1e3c22U, 0x1b366c5aU, 0x75eac923U,
    0x86112233U, 0x84152a3fU, 0x9c254a6fU, 0x4a9435a1U,
    0x97336655U, 0x1a34685cU, 0x65ca8943U, 0xf6f1ff0eU,
    0xedc79354U, 0x09122436U, 0xbb6bd6bdU, 0x264c98d4U,
    0x831b362dU, 0xebcb8b40U, 0x6fdea17fU, 0x811f3e21U,
    0x04081018U, 0x6ad4b561U, 0x43861197U, 0x01020406U,
    0x172e5c72U, 0xe1dfa37cU, 0x87132635U, 0xf5f7f304U,
    0x8d070e09U, 0xe3dbab70U, 0x23468ccaU, 0x801d3a27U,
    0x44880d85U, 0x162c5874U, 0x66cc8549U, 0x214284c6U,
    0xfee1df3eU, 0xd5b773c4U, 0x3162c4a6U, 0xd9af43ecU,
    0x356ad4beU, 0x18306050U, 0x0204080cU, 0x64c88d45U,
    0xf2f9ef16U, 0xf1ffe31cU, 0x56ac45e9U, 0xcd871394U,
    0x8219322bU, 0xc88d078aU, 0xba69d2bbU, 0xf0fde71aU,
    0xefc39b58U, 0xe9cf834cU, 0xe8cd874aU, 0xfde7d334U,
    0x890f1e11U, 0xd7b37bc8U, 0xc7933ba8U, 0xb577ee99U,
    0xa455aaffU, 0x2f5ebce2U, 0x95376e59U, 0x13264c6aU,
    0x0b162c3aU, 0xf3fbeb10U, 0xe0dda77aU, 0x376edcb2U,
};

static const u32 T1[256] = {
    0x53a7f5a6U, 0xbbd3d06bU, 0xd1e66ebfU, 0xe2713bd9U,
    0xbdd0da67U, 0x45accf8aU, 0x9a4db329U, 0xf2790bf9U,
    0x743a9ce8U, 0x8fc98c03U, 0x3f91417eU, 0xe5fc32d7U,
    0x3c1e4478U, 0x8e478f01U, 0xa854e54dU, 0x67bda9ceU,
    0x058c0f0aU, 0x57a5f9aeU, 0xf47a01f5U, 0xebfb20cbU,
    0xc6635791U, 0x6db8b7daU, 0xa7ddf453U, 0xb5d4c277U,
    0xd7e564b3U, 0x7bb38df6U, 0x97c5a433U, 0x61bea3c2U,
    0x4fa9d19eU, 0x0d88171aU, 0x180c2830U, 0x59a2ebb2U,
    0x723996e4U, 0xa3dff85bU, 0x5229f6a4U, 0xa9dae64fU,
    0x562bfaacU, 0x4da8d79aU, 0x8bcb800bU, 0x984cb52dU,
    0x964ba731U, 0x4422cc88U, 0x49aadb92U, 0x4824d890U,
    0x82419b19U, 0xe0703dddU, 0x51a6f3a2U, 0xeff92cc3U,
    0xb45ac175U, 0xd9e276afU, 0x7db087faU, 0x6c36b4d8U,
    0xfa7d13e9U, 0xd5e462b7U, 0x6633aaccU, 0xe3ff38dbU,
    0xc0605d9dU, 0x4020c080U, 0x10083020U, 0x0b8b1d16U,
    0xbc5ed965U, 0x4babdd96U, 0xfe7f1fe1U, 0xf0780dfdU,
    0xf87c15edU, 0x582ce8b0U, 0xae57ef41U, 0xb9d2d66fU,
    0xa5dcf257U, 0xda6d73a9U, 0xfc7e19e5U, 0x1a0d2e34U,
    0xa653f751U, 0x35945f6aU, 0x9bc3b02bU, 0x5028f0a0U,
    0x4e27d29cU, 0x0c061418U, 0xbe5fdf61U, 0x47adc98eU,
    0xce674f81U, 0xb85cd56dU, 0xaa55e349U, 0x9048ad3dU,
    0x1c0e2438U, 0xa452f155U, 0xc9ea468fU, 0x84429115U,
    0xb65bc771U, 0xba5dd369U, 0x6030a0c0U, 0xb058cd7dU,
    0xa251fb59U, 0xb259cb79U, 0x783c88f0U, 0x9c4eb925U,
    0x703890e0U, 0x098a1b12U, 0xe47231d5U, 0x28147850U,
    0xd3e768bbU, 0x91c6ae3fU, 0xa1defe5fU, 0xa050fd5dU,
    0x018e0302U, 0x39924b72U, 0xbfd1dc63U, 0xee772fc1U,
    0x3b934d76U, 0x8a458309U, 0x299a7b52U, 0x81ce9e1fU,
    0x5a2deeb4U, 0x06030a0cU, 0xc4625195U, 0x71b693e2U,
    0x6fb9b1deU, 0x63bfa5c6U, 0x31965362U, 0xd66b67b1U,
    0x7e3f82fcU, 0x0e07121cU, 0x24126c48U, 0x41aec382U,
    0x80409d1dU, 0x6834b8d0U, 0x8c468905U, 0x7c3e84f8U,
    0xabdbe04bU, 0x83cf981bU, 0xc5ec5297U, 0x85cc9217U,
    0x9fc1bc23U, 0x5fa1e1beU, 0x9dc0ba27U, 0xb1d6ce7fU,
    0x3a1d4e74U, 0xf5f402f7U, 0xc2615b99U, 0x763b9aecU,
    0x20106040U, 0xadd8ea47U, 0xd0686dbdU, 0x5da0e7baU,
    0x7fb181feU, 0x140a3c28U, 0xd2696bb9U, 0xd86c75adU,
    0x9249ab39U, 0xe9fa26cfU, 0xec7629c5U, 0x95c4a237U,
    0x219e6342U, 0x2b9b7d56U, 0xdc6e79a5U, 0x2f99715eU,
    0x99c2b62fU, 0x73b795e6U, 0x2d98775aU, 0x65bcafcaU,
    0x038f0506U, 0x1785392eU, 0x3e1f427cU, 0x75b49feaU,
    0xedf82ac7U, 0x22116644U, 0x5c2ee4b8U, 0x00000000U,
    0x4a25de94U, 0x381c4870U, 0x542afca8U, 0x7a3d8ef4U,
    0x0a051e14U, 0x9e4fbf21U, 0xf67b07f1U, 0x79b28bf2U,
    0x6432acc8U, 0x3d90477aU, 0x43afc586U, 0x32195664U,
    0x5ba3edb6U, 0xf3f708fbU, 0xe67337d1U, 0x279d694eU,
    0x2a157e54U, 0xe87425cdU, 0xc1ee5e9fU, 0x89ca860fU,
    0x239f6546U, 0x1e0f223cU, 0x361b5a6cU, 0xea7523c9U,
    0x11863322U, 0x15843f2aU, 0x259c6f4aU, 0x944aa135U,
    0x33975566U, 0x341a5c68U, 0xca654389U, 0xf1f60effU,
    0xc7ed5493U, 0x12093624U, 0x6bbbbdd6U, 0x4c26d498U,
    0x1b832d36U, 0xcbeb408bU, 0xde6f7fa1U, 0x1f81213eU,
    0x08041810U, 0xd46a61b5U, 0x86439711U, 0x02010604U,
    0x2e17725cU, 0xdfe17ca3U, 0x13873526U, 0xf7f504f3U,
    0x078d090eU, 0xdbe370abU, 0x4623ca8cU, 0x1d80273aU,
    0x8844850dU, 0x2c167458U, 0xcc664985U, 0x4221c684U,
    0xe1fe3edfU, 0xb7d5c473U, 0x6231a6c4U, 0xafd9ec43U,
    0x6a35bed4U, 0x30185060U, 0x04020c08U, 0xc864458dU,
    0xf9f216efU, 0xfff11ce3U, 0xac56e945U, 0x87cd9413U,
    0x19822b32U, 0x8dc88a07U, 0x69babbd2U, 0xfdf01ae7U,
    0xc3ef589bU, 0xcfe94c83U, 0xcde84a87U, 0xe7fd34d3U,
    0x0f89111eU, 0xb3d7c87bU, 0x93c7a83bU, 0x77b599eeU,
    0x55a4ffaaU, 0x5e2fe2bcU, 0x3795596eU, 0x26136a4cU,
    0x160b3a2cU, 0xfbf310ebU, 0xdde07aa7U, 0x6e37b2dcU,
};

static const u32 T2[256] = {
    0xa6f5a753U, 0x6bd0d3bbU, 0xbf6ee6d1U, 0xd93b71e2U,
    0x67dad0bdU, 0x8acfac45U, 0x29b34d9aU, 0xf90b79f2U,
    0xe89c3a74U, 0x038cc98fU, 0x7e41913fU, 0xd732fce5U,
    0x78441e3cU, 0x018f478eU, 0x4de554a8U, 0xcea9bd67U,
    0x0a0f8c05U, 0xaef9a557U, 0xf5017af4U, 0xcb20fbebU,
    0x915763c6U, 0xdab7b86dU, 0x53f4dda7U, 0x77c2d4b5U,
    0xb364e5d7U, 0xf68db37bU, 0x33a4c597U, 0xc2a3be61U,
    0x9ed1a94fU, 0x1a17880dU, 0x30280c18U, 0xb2eba259U,
    0xe4963972U, 0x5bf8dfa3U, 0xa4f62952U, 0x4fe6daa9U,
    0xacfa2b56U, 0x9ad7a84dU, 0x0b80cb8bU, 0x2db54c98U,
    0x31a74b96U, 0x88cc2244U, 0x92dbaa49U, 0x90d82448U,
    0x199b4182U, 0xdd3d70e0U, 0xa2f3a651U, 0xc32cf9efU,
    0x75c15ab4U, 0xaf76e2d9U, 0xfa87b07dU, 0xd8b4366cU,
    0xe9137dfaU, 0xb762e4d5U, 0xccaa3366U, 0xdb38ffe3U,
    0x9d5d60c0U, 0x80c02040U, 0x20300810U, 0x161d8b0bU,
    0x65d95ebcU, 0x96ddab4bU, 0xe11f7ffeU, 0xfd0d78f0U,
    0xed157cf8U, 0xb0e82c58U, 0x41ef57aeU, 0x6fd6d2b9U,
    0x57f2dca5U, 0xa9736ddaU, 0xe5197efcU, 0x342e0d1aU,
    0x51f753a6U, 0x6a5f9435U, 0x2bb0c39bU, 0xa0f02850U,
    0x9cd2274eU, 0x1814060cU, 0x61df5fbeU, 0x8ec9ad47U,
    0x814f67ceU, 0x6dd55cb8U, 0x49e355aaU, 0x3dad4890U,
    0x38240e1cU, 0x55f152a4U, 0x8f46eac9U, 0x15914284U,
    0x71c75bb6U, 0x69d35dbaU, 0xc0a03060U, 0x7dcd58b0U,
    0x59fb51a2U, 0x79cb59b2U, 0xf0883c78U, 0x25b94e9cU,
    0xe0903870U, 0x121b8a09U, 0xd53172e4U, 0x50781428U,
    0xbb68e7d3U, 0x3faec691U, 0x5ffedea1U, 0x5dfd50a0U,
    0x02038e01U, 0x724b9239U, 0x63dcd1bfU, 0xc12f77eeU,
    0x764d933bU, 0x0983458aU, 0x527b9a29U, 0x1f9ece81U,
    0xb4ee2d5aU, 0x0c0a0306U, 0x955162c4U, 0xe293b671U,
    0xdeb1b96fU, 0xc6a5bf63U, 0x62539631U, 0xb1676bd6U,
    0xfc823f7eU, 0x1c12070eU, 0x486c1224U, 0x82c3ae41U,
    0x1d9d4080U, 0xd0b83468U, 0x0589468cU, 0xf8843e7cU,
    0x4be0dbabU, 0x1b98cf83U, 0x9752ecc5U, 0x1792cc85U,
    0x23bcc19fU, 0xbee1a15fU, 0x27bac09dU, 0x7fced6b1U,
    0x744e1d3aU, 0xf702f4f5U, 0x995b61c2U, 0xec9a3b76U,
    0x40601020U, 0x47ead8adU, 0xbd6d68d0U, 0xbae7a05dU,
    0xfe81b17fU, 0x283c0a14U, 0xb96b69d2U, 0xad756cd8U,
    0x39ab4992U, 0xcf26fae9U, 0xc52976ecU, 0x37a2c495U,
    0x42639e21U, 0x567d9b2bU, 0xa5796edcU, 0x5e71992fU,
    0x2fb6c299U, 0xe695b773U, 0x5a77982dU, 0xcaafbc65U,
    0x06058f03U, 0x2e398517U, 0x7c421f3eU, 0xea9fb475U,
    0xc72af8edU, 0x44661122U, 0xb8e42e5cU, 0x00000000U,
    0x94de254aU, 0x70481c38U, 0xa8fc2a54U, 0xf48e3d7aU,
    0x141e050aU, 0x21bf4f9eU, 0xf1077bf6U, 0xf28bb279U,
    0xc8ac3264U, 0x7a47903dU, 0x86c5af43U, 0x64561932U,
    0xb6eda35bU, 0xfb08f7f3U, 0xd13773e6U, 0x4e699d27U,
    0x547e152aU, 0xcd2574e8U, 0x9f5eeec1U, 0x0f86ca89U,
    0x46659f23U, 0x3c220f1eU, 0x6c5a1b36U, 0xc92375eaU,
    0x22338611U, 0x2a3f8415U, 0x4a6f9c25U, 0x35a14a94U,
    0x66559733U, 0x685c1a34U, 0x894365caU, 0xff0ef6f1U,
    0x9354edc7U, 0x24360912U, 0xd6bdbb6bU, 0x98d4264cU,
    0x362d831bU, 0x8b40ebcbU, 0xa17f6fdeU, 0x3e21811fU,
    0x10180408U, 0xb5616ad4U, 0x11974386U, 0x04060102U,
    0x5c72172eU, 0xa37ce1dfU, 0x26358713U, 0xf304f5f7U,
    0x0e098d07U, 0xab70e3dbU, 0x8cca2346U, 0x3a27801dU,
    0x0d854488U, 0x5874162cU, 0x854966ccU, 0x84c62142U,
    0xdf3efee1U, 0x73c4d5b7U, 0xc4a63162U, 0x43ecd9afU,
    0xd4be356aU, 0x60501830U, 0x080c0204U, 0x8d4564c8U,
    0xef16f2f9U, 0xe31cf1ffU, 0x45e956acU, 0x1394cd87U,
    0x322b8219U, 0x078ac88dU, 0xd2bbba69U, 0xe71af0fdU,
    0x9b58efc3U, 0x834ce9cfU, 0x874ae8cdU, 0xd334fde7U,
    0x1e11890fU, 0x7bc8d7b3U, 0x3ba8c793U, 0xee99b577U,
    0xaaffa455U, 0xbce22f5eU, 0x6e599537U, 0x4c6a1326U,
    0x2c3a0b16U, 0xeb10f3fbU, 0xa77ae0ddU, 0xdcb2376eU,
};

static const u32 T3[256] = {
    0xf5a653a7U, 0xd06bbbd3U, 0x6ebfd1e6U, 0x3bd9e271U,
    0xda67bdd0U, 0xcf8a45acU, 0xb3299a4dU, 0x0bf9f279U,
    0x9ce8743aU, 0x8c038fc9U, 0x417e3f91U, 0x32d7e5fcU,
    0x44783c1eU, 0x8f018e47U, 0xe54da854U, 0xa9ce67bdU,
    0x0f0a058cU, 0xf9ae57a5U, 0x01f5f47aU, 0x20cbebfbU,
    0x5791c663U, 0xb7da6db8U, 0xf453a7ddU, 0xc277b5d4U,
    0x64b3d7e5U, 0x8df67bb3U, 0xa43397c5U, 0xa3c261beU,
    0xd19e4fa9U, 0x171a0d88U, 0x2830180cU, 0xebb259a2U,
    0x96e47239U, 0xf85ba3dfU, 0xf6a45229U, 0xe64fa9daU,
    0xfaac562bU, 0xd79a4da8U, 0x800b8bcbU, 0xb52d984cU,
    0xa731964bU, 0xcc884422U, 0xdb9249aaU, 0xd8904824U,
    0x9b198241U, 0x3ddde070U, 0xf3a251a6U, 0x2cc3eff9U,
    0xc175b45aU, 0x76afd9e2U, 0x87fa7db0U, 0xb4d86c36U,
    0x13e9fa7dU, 0x62b7d5e4U, 0xaacc6633U, 0x38dbe3ffU,
    0x5d9dc060U, 0xc0804020U, 0x30201008U, 0x1d160b8bU,
    0xd965bc5eU, 0xdd964babU, 0x1fe1fe7fU, 0x0dfdf078U,
    0x15edf87cU, 0xe8b0582cU, 0xef41ae57U, 0xd66fb9d2U,
    0xf257a5dcU, 0x73a9da6dU, 0x19e5fc7eU, 0x2e341a0dU,
    0xf751a653U, 0x5f6a3594U, 0xb02b9bc3U, 0xf0a05028U,
    0xd29c4e27U, 0x14180c06U, 0xdf61be5fU, 0xc98e47adU,
    0x4f81ce67U, 0xd56db85cU, 0xe349aa55U, 0xad3d9048U,
    0x24381c0eU, 0xf155a452U, 0x468fc9eaU, 0x91158442U,
    0xc771b65bU, 0xd369ba5dU, 0xa0c06030U, 0xcd7db058U,
    0xfb59a251U, 0xcb79b259U, 0x88f0783cU, 0xb9259c4eU,
    0x90e07038U, 0x1b12098aU, 0x31d5e472U, 0x78502814U,
    0x68bbd3e7U, 0xae3f91c6U, 0xfe5fa1deU, 0xfd5da050U,
    0x0302018eU, 0x4b723992U, 0xdc63bfd1U, 0x2fc1ee77U,
    0x4d763b93U, 0x83098a45U, 0x7b52299aU, 0x9e1f81ceU,
    0xeeb45a2dU, 0x0a0c0603U, 0x5195c462U, 0x93e271b6U,
    0xb1de6fb9U, 0xa5c663bfU, 0x53623196U, 0x67b1d66bU,
    0x82fc7e3fU, 0x121c0e07U, 0x6c482412U, 0xc38241aeU,
    0x9d1d8040U, 0xb8d06834U, 0x89058c46U, 0x84f87c3eU,
    0xe04babdbU, 0x981b83cfU, 0x5297c5ecU, 0x921785ccU,
    0xbc239fc1U, 0xe1be5fa1U, 0xba279dc0U, 0xce7fb1d6U,
    0x4e743a1dU, 0x02f7f5f4U, 0x5b99c261U, 0x9aec763bU,
    0x60402010U, 0xea47add8U, 0x6dbdd068U, 0xe7ba5da0U,
    0x81fe7fb1U, 0x3c28140aU, 0x6bb9d269U, 0x75add86cU,
    0xab399249U, 0x26cfe9faU, 0x29c5ec76U, 0xa23795c4U,
    0x6342219eU, 0x7d562b9bU, 0x79a5dc6eU, 0x715e2f99U,
    0xb62f99c2U, 0x95e673b7U, 0x775a2d98U, 0xafca65bcU,
    0x0506038fU, 0x392e1785U, 0x427c3e1fU, 0x9fea75b4U,
    0x2ac7edf8U, 0x66442211U, 0xe4b85c2eU, 0x00000000U,
    0xde944a25U, 0x4870381cU, 0xfca8542aU, 0x8ef47a3dU,
    0x1e140a05U, 0xbf219e4fU, 0x07f1f67bU, 0x8bf279b2U,
    0xacc86432U, 0x477a3d90U, 0xc58643afU, 0x56643219U,
    0xedb65ba3U, 0x08fbf3f7U, 0x37d1e673U, 0x694e279dU,
    0x7e542a15U, 0x25cde874U, 0x5e9fc1eeU, 0x860f89caU,
    0x6546239fU, 0x223c1e0fU, 0x5a6c361bU, 0x23c9ea75U,
    0x33221186U, 0x3f2a1584U, 0x6f4a259cU, 0xa135944aU,
    0x55663397U, 0x5c68341aU, 0x4389ca65U, 0x0efff1f6U,
    0x5493c7edU, 0x36241209U, 0xbdd66bbbU, 0xd4984c26U,
    0x2d361b83U, 0x408bcbebU, 0x7fa1de6fU, 0x213e1f81U,
    0x18100804U, 0x61b5d46aU, 0x97118643U, 0x06040201U,
    0x725c2e17U, 0x7ca3dfe1U, 0x35261387U, 0x04f3f7f5U,
    0x090e078dU, 0x70abdbe3U, 0xca8c4623U, 0x273a1d80U,
    0x850d8844U, 0x74582c16U, 0x4985cc66U, 0xc6844221U,
    0x3edfe1feU, 0xc473b7d5U, 0xa6c46231U, 0xec43afd9U,
    0xbed46a35U, 0x50603018U, 0x0c080402U, 0x458dc864U,
    0x16eff9f2U, 0x1ce3fff1U, 0xe945ac56U, 0x941387cdU,
    0x2b321982U, 0x8a078dc8U, 0xbbd269baU, 0x1ae7fdf0U,
    0x589bc3efU, 0x4c83cfe9U, 0x4a87cde8U, 0x34d3e7fdU,
    0x111e0f89U, 0xc87bb3d7U, 0xa83b93c7U, 0x99ee77b5U,
    0xffaa55a4U, 0xe2bc5e2fU, 0x596e3795U, 0x6a4c2613U,
    0x3a2c160bU, 0x10ebfbf3U, 0x7aa7dde0U, 0xb2dc6e37U,
};

static const u32 T4[256] = {
    0xa7a7a7a7U, 0xd3d3d3d3U, 0xe6e6e6e6U, 0x71717171U,
    0xd0d0d0d0U, 0xacacacacU, 0x4d4d4d4dU, 0x79797979U,
    0x3a3a3a3aU, 0xc9c9c9c9U, 0x91919191U, 0xfcfcfcfcU,
    0x1e1e1e1eU, 0x47474747U, 0x54545454U, 0xbdbdbdbdU,
    0x8c8c8c8cU, 0xa5a5a5a5U, 0x7a7a7a7aU, 0xfbfbfbfbU,
    0x63636363U, 0xb8b8b8b8U, 0xddddddddU, 0xd4d4d4d4U,
    0xe5e5e5e5U, 0xb3b3b3b3U, 0xc5c5c5c5U, 0xbebebebeU,
    0xa9a9a9a9U, 0x88888888U, 0x0c0c0c0cU, 0xa2a2a2a2U,
    0x39393939U, 0xdfdfdfdfU, 0x29292929U, 0xdadadadaU,
    0x2b2b2b2bU, 0xa8a8a8a8U, 0xcbcbcbcbU, 0x4c4c4c4cU,
    0x4b4b4b4bU, 0x22222222U, 0xaaaaaaaaU, 0x24242424U,
    0x41414141U, 0x70707070U, 0xa6a6a6a6U, 0xf9f9f9f9U,
    0x5a5a5a5aU, 0xe2e2e2e2U, 0xb0b0b0b0U, 0x36363636U,
    0x7d7d7d7dU, 0xe4e4e4e4U, 0x33333333U, 0xffffffffU,
    0x60606060U, 0x20202020U, 0x08080808U, 0x8b8b8b8bU,
    0x5e5e5e5eU, 0xababababU, 0x7f7f7f7fU, 0x78787878U,
    0x7c7c7c7cU, 0x2c2c2c2cU, 0x57575757U, 0xd2d2d2d2U,
    0xdcdcdcdcU, 0x6d6d6d6dU, 0x7e7e7e7eU, 0x0d0d0d0dU,
    0x53535353U, 0x94949494U, 0xc3c3c3c3U, 0x28282828U,
    0x27272727U, 0x06060606U, 0x5f5f5f5fU, 0xadadadadU,
    0x67676767U, 0x5c5c5c5cU, 0x55555555U, 0x48484848U,
    0x0e0e0e0eU, 0x52525252U, 0xeaeaeaeaU, 0x42424242U,
    0x5b5b5b5bU, 0x5d5d5d5dU, 0x30303030U, 0x58585858U,
    0x51515151U, 0x59595959U, 0x3c3c3c3cU, 0x4e4e4e4eU,
    0x38383838U, 0x8a8a8a8aU, 0x72727272U, 0x14141414U,
    0xe7e7e7e7U, 0xc6c6c6c6U, 0xdedededeU, 0x50505050U,
    0x8e8e8e8eU, 0x92929292U, 0xd1d1d1d1U, 0x77777777U,
    0x93939393U, 0x45454545U, 0x9a9a9a9aU, 0xcecececeU,
    0x2d2d2d2dU, 0x03030303U, 0x62626262U, 0xb6b6b6b6U,
    0xb9b9b9b9U, 0xbfbfbfbfU, 0x96969696U, 0x6b6b6b6bU,
    0x3f3f3f3fU, 0x07070707U, 0x12121212U, 0xaeaeaeaeU,
    0x40404040U, 0x34343434U, 0x46464646U, 0x3e3e3e3eU,
    0xdbdbdbdbU, 0xcfcfcfcfU, 0xececececU, 0xccccccccU,
    0xc1c1c1c1U, 0xa1a1a1a1U, 0xc0c0c0c0U, 0xd6d6d6d6U,
    0x1d1d1d1dU, 0xf4f4f4f4U, 0x61616161U, 0x3b3b3b3bU,
    0x10101010U, 0xd8d8d8d8U, 0x68686868U, 0xa0a0a0a0U,
    0xb1b1b1b1U, 0x0a0a0a0aU, 0x69696969U, 0x6c6c6c6cU,
    0x49494949U, 0xfafafafaU, 0x76767676U, 0xc4c4c4c4U,
    0x9e9e9e9eU, 0x9b9b9b9bU, 0x6e6e6e6eU, 0x99999999U,
    0xc2c2c2c2U, 0xb7b7b7b7U, 0x98989898U, 0xbcbcbcbcU,
    0x8f8f8f8fU, 0x85858585U, 0x1f1f1f1fU, 0xb4b4b4b4U,
    0xf8f8f8f8U, 0x11111111U, 0x2e2e2e2eU, 0x00000000U,
    0x25252525U, 0x1c1c1c1cU, 0x2a2a2a2aU, 0x3d3d3d3dU,
    0x05050505U, 0x4f4f4f4fU, 0x7b7b7b7bU, 0xb2b2b2b2U,
    0x32323232U, 0x90909090U, 0xafafafafU, 0x19191919U,
    0xa3a3a3a3U, 0xf7f7f7f7U, 0x73737373U, 0x9d9d9d9dU,
    0x15151515U, 0x74747474U, 0xeeeeeeeeU, 0xcacacacaU,
    0x9f9f9f9fU, 0x0f0f0f0fU, 0x1b1b1b1bU, 0x75757575U,
    0x86868686U, 0x84848484U, 0x9c9c9c9cU, 0x4a4a4a4aU,
    0x97979797U, 0x1a1a1a1aU, 0x65656565U, 0xf6f6f6f6U,
    0xededededU, 0x09090909U, 0xbbbbbbbbU, 0x26262626U,
    0x83838383U, 0xebebebebU, 0x6f6f6f6fU, 0x81818181U,
    0x04040404U, 0x6a6a6a6aU, 0x43434343U, 0x01010101U,
    0x17171717U, 0xe1e1e1e1U, 0x87878787U, 0xf5f5f5f5U,
    0x8d8d8d8dU, 0xe3e3e3e3U, 0x23232323U, 0x80808080U,
    0x44444444U, 0x16161616U, 0x66666666U, 0x21212121U,
    0xfefefefeU, 0xd5d5d5d5U, 0x31313131U, 0xd9d9d9d9U,
    0x35353535U, 0x18181818U, 0x02020202U, 0x64646464U,
    0xf2f2f2f2U, 0xf1f1f1f1U, 0x56565656U, 0xcdcdcdcdU,
    0x82828282U, 0xc8c8c8c8U, 0xbabababaU, 0xf0f0f0f0U,
    0xefefefefU, 0xe9e9e9e9U, 0xe8e8e8e8U, 0xfdfdfdfdU,
    0x89898989U, 0xd7d7d7d7U, 0xc7c7c7c7U, 0xb5b5b5b5U,
    0xa4a4a4a4U, 0x2f2f2f2fU, 0x95959595U, 0x13131313U,
    0x0b0b0b0bU, 0xf3f3f3f3U, 0xe0e0e0e0U, 0x37373737U,
};

static const u32 T5[256] = {
    0x00000000U, 0x01020608U, 0x02040c10U, 0x03060a18U,
    0x04081820U, 0x050a1e28U, 0x060c1430U, 0x070e1238U,
    0x08103040U, 0x09123648U, 0x0a143c50U, 0x0b163a58U,
    0x0c182860U, 0x0d1a2e68U, 0x0e1c2470U, 0x0f1e2278U,
    0x10206080U, 0x11226688U, 0x12246c90U, 0x13266a98U,
    0x142878a0U, 0x152a7ea8U, 0x162c74b0U, 0x172e72b8U,
    0x183050c0U, 0x193256c8U, 0x1a345cd0U, 0x1b365ad8U,
    0x1c3848e0U, 0x1d3a4ee8U, 0x1e3c44f0U, 0x1f3e42f8U,
    0x2040c01dU, 0x2142c615U, 0x2244cc0dU, 0x2346ca05U,
    0x2448d83dU, 0x254ade35U, 0x264cd42dU, 0x274ed225U,
    0x2850f05dU, 0x2952f655U, 0x2a54fc4dU, 0x2b56fa45U,
    0x2c58e87dU, 0x2d5aee75U, 0x2e5ce46dU, 0x2f5ee265U,
    0x3060a09dU, 0x3162a695U, 0x3264ac8dU, 0x3366aa85U,
    0x3468b8bdU, 0x356abeb5U, 0x366cb4adU, 0x376eb2a5U,
    0x387090ddU, 0x397296d5U, 0x3a749ccdU, 0x3b769ac5U,
    0x3c7888fdU, 0x3d7a8ef5U, 0x3e7c84edU, 0x3f7e82e5U,
    0x40809d3aU, 0x41829b32U, 0x4284912aU, 0x43869722U,
    0x4488851aU, 0x458a8312U, 0x468c890aU, 0x478e8f02U,
    0x4890ad7aU, 0x4992ab72U, 0x4a94a16aU, 0x4b96a762U,
    0x4c98b55aU, 0x4d9ab352U, 0x4e9cb94aU, 0x4f9ebf42U,
    0x50a0fdbaU, 0x51a2fbb2U, 0x52a4f1aaU, 0x53a6f7a2U,
    0x54a8e59aU, 0x55aae392U, 0x56ace98aU, 0x57aeef82U,
    0x58b0cdfaU, 0x59b2cbf2U, 0x5ab4c1eaU, 0x5bb6c7e2U,
    0x5cb8d5daU, 0x5dbad3d2U, 0x5ebcd9caU, 0x5fbedfc2U,
    0x60c05d27U, 0x61c25b2fU, 0x62c45137U, 0x63c6573fU,
    0x64c84507U, 0x65ca430fU, 0x66cc4917U, 0x67ce4f1fU,
    0x68d06d67U, 0x69d26b6fU, 0x6ad46177U, 0x6bd6677fU,
    0x6cd87547U, 0x6dda734fU, 0x6edc7957U, 0x6fde7f5fU,
    0x70e03da7U, 0x71e23bafU, 0x72e431b7U, 0x73e637bfU,
    0x74e82587U, 0x75ea238fU, 0x76ec2997U, 0x77ee2f9fU,
    0x78f00de7U, 0x79f20befU, 0x7af401f7U, 0x7bf607ffU,
    0x7cf815c7U, 0x7dfa13cfU, 0x7efc19d7U, 0x7ffe1fdfU,
    0x801d2774U, 0x811f217cU, 0x82192b64U, 0x831b2d6cU,
    0x84153f54U, 0x8517395cU, 0x86113344U, 0x8713354cU,
    0x880d1734U, 0x890f113cU, 0x8a091b24U, 0x8b0b1d2cU,
    0x8c050f14U, 0x8d07091cU, 0x8e010304U, 0x8f03050cU,
    0x903d47f4U, 0x913f41fcU, 0x92394be4U, 0x933b4decU,
    0x94355fd4U, 0x953759dcU, 0x963153c4U, 0x973355ccU,
    0x982d77b4U, 0x992f71bcU, 0x9a297ba4U, 0x9b2b7dacU,
    0x9c256f94U, 0x9d27699cU, 0x9e216384U, 0x9f23658cU,
    0xa05de769U, 0xa15fe161U, 0xa259eb79U, 0xa35bed71U,
    0xa455ff49U, 0xa557f941U, 0xa651f359U, 0xa753f551U,
    0xa84dd729U, 0xa94fd121U, 0xaa49db39U, 0xab4bdd31U,
    0xac45cf09U, 0xad47c901U, 0xae41c319U, 0xaf43c511U,
    0xb07d87e9U, 0xb17f81e1U, 0xb2798bf9U, 0xb37b8df1U,
    0xb4759fc9U, 0xb57799c1U, 0xb67193d9U, 0xb77395d1U,
    0xb86db7a9U, 0xb96fb1a1U, 0xba69bbb9U, 0xbb6bbdb1U,
    0xbc65af89U, 0xbd67a981U, 0xbe61a399U, 0xbf63a591U,
    0xc09dba4eU, 0xc19fbc46U, 0xc299b65eU, 0xc39bb056U,
    0xc495a26eU, 0xc597a466U, 0xc691ae7eU, 0xc793a876U,
    0xc88d8a0eU, 0xc98f8c06U, 0xca89861eU, 0xcb8b8016U,
    0xcc85922eU, 0xcd879426U, 0xce819e3eU, 0xcf839836U,
    0xd0bddaceU, 0xd1bfdcc6U, 0xd2b9d6deU, 0xd3bbd0d6U,
    0xd4b5c2eeU, 0xd5b7c4e6U, 0xd6b1cefeU, 0xd7b3c8f6U,
    0xd8adea8eU, 0xd9afec86U, 0xdaa9e69eU, 0xdbabe096U,
    0xdca5f2aeU, 0xdda7f4a6U, 0xdea1febeU, 0xdfa3f8b6U,
    0xe0dd7a53U, 0xe1df7c5bU, 0xe2d97643U, 0xe3db704bU,
    0xe4d56273U, 0xe5d7647bU, 0xe6d16e63U, 0xe7d3686bU,
    0xe8cd4a13U, 0xe9cf4c1bU, 0xeac94603U, 0xebcb400bU,
    0xecc55233U, 0xedc7543bU, 0xeec15e23U, 0xefc3582bU,
    0xf0fd1ad3U, 0xf1ff1cdbU, 0xf2f916c3U, 0xf3fb10cbU,
    0xf4f502f3U, 0xf5f704fbU, 0xf6f10ee3U, 0xf7f308ebU,
    0xf8ed2a93U, 0xf9ef2c9bU, 0xfae92683U, 0xfbeb208bU,
    0xfce532b3U, 0xfde734bbU, 0xfee13ea3U, 0xffe338abU,
};

/**
 * The round constants.
 */
static const u32 rc[] = {
    0xa7d3e671U, 0xd0ac4d79U, 0x3ac991fcU, 0x1e4754bdU,
	0x8ca57afbU, 0x63b8ddd4U, 0xe5b3c5beU, 0xa9880ca2U,
	0x39df29daU, 0x2ba8cb4cU, 0x4b22aa24U, 0x4170a6f9U,
	0x5ae2b036U, 0x7de433ffU, 0x6020088bU, 0x5eab7f78U,
	0x7c2c57d2U, 0xdc6d7e0dU, 0x5394c328U,
};


/**
 * Create the Anubis key schedule for a given cipher key.
 * Both encryption and decryption key schedules are generated.
 * 
 * @param key			The 32N-bit cipher key.
 * @param structpointer	Pointer to the structure that will hold the expanded key.
 */
void ANUBISkeysetup(const unsigned char * const key,
					struct ANUBISstruct * const structpointer) {

	int N, R, i, pos, r;
	u32 kappa[MAX_N];
	u32 inter[MAX_N];

	structpointer->keyBits = KEYSIZEB*8;

	/*
	 * determine the N length parameter:
	 * (N.B. it is assumed that the key length is valid!)
	 */
	N = structpointer->keyBits >> 5;

	/*
	 * determine number of rounds from key size:
	 */
	structpointer->R = R = 8 + N;

    /*
	 * map cipher key to initial key state (mu):
	 */
    for (i = 0, pos = 0; i < N; i++, pos += 4) {
		kappa[i] =
			(key[pos    ] << 24) ^
			(key[pos + 1] << 16) ^
			(key[pos + 2] <<  8) ^
			(key[pos + 3]      );
    }

	/*
	 * generate R + 1 round keys:
	 */
	for (r = 0; r <= R; r++) {
		u32 K0, K1, K2, K3;
		/*
		 * generate r-th round key K^r:
		 */
		K0 = T4[(kappa[N - 1] >> 24)       ];
		K1 = T4[(kappa[N - 1] >> 16) & 0xff];
		K2 = T4[(kappa[N - 1] >>  8) & 0xff];
		K3 = T4[(kappa[N - 1]      ) & 0xff];
		for (i = N - 2; i >= 0; i--) {
			K0 = T4[(kappa[i] >> 24)       ] ^
				(T5[(K0 >> 24)       ] & 0xff000000U) ^
				(T5[(K0 >> 16) & 0xff] & 0x00ff0000U) ^
				(T5[(K0 >>  8) & 0xff] & 0x0000ff00U) ^
				(T5[(K0      ) & 0xff] & 0x000000ffU);
			K1 = T4[(kappa[i] >> 16) & 0xff] ^
				(T5[(K1 >> 24)       ] & 0xff000000U) ^
				(T5[(K1 >> 16) & 0xff] & 0x00ff0000U) ^
				(T5[(K1 >>  8) & 0xff] & 0x0000ff00U) ^
				(T5[(K1      ) & 0xff] & 0x000000ffU);
			K2 = T4[(kappa[i] >>  8) & 0xff] ^
				(T5[(K2 >> 24)       ] & 0xff000000U) ^
				(T5[(K2 >> 16) & 0xff] & 0x00ff0000U) ^
				(T5[(K2 >>  8) & 0xff] & 0x0000ff00U) ^
				(T5[(K2      ) & 0xff] & 0x000000ffU);
			K3 = T4[(kappa[i]      ) & 0xff] ^
				(T5[(K3 >> 24)       ] & 0xff000000U) ^
				(T5[(K3 >> 16) & 0xff] & 0x00ff0000U) ^
				(T5[(K3 >>  8) & 0xff] & 0x0000ff00U) ^
				(T5[(K3      ) & 0xff] & 0x000000ffU);
		}
		/*
		-- this is the code to use with the large U tables:
		K0 = K1 = K2 = K3 = 0;
		for (i = 0; i < N; i++) {
			K0 ^= U[i][(kappa[i] >> 24)       ];
			K1 ^= U[i][(kappa[i] >> 16) & 0xff];
			K2 ^= U[i][(kappa[i] >>  8) & 0xff];
			K3 ^= U[i][(kappa[i]      ) & 0xff];
		}
		*/
		structpointer->roundKeyEnc[r][0] = K0;
		structpointer->roundKeyEnc[r][1] = K1;
		structpointer->roundKeyEnc[r][2] = K2;
		structpointer->roundKeyEnc[r][3] = K3;

		/*
		 * compute kappa^{r+1} from kappa^r:
		 */
		if (r == R) {
			break;
		}
		for (i = 0; i < N; i++) {
			int j = i;
			inter[i]  = T0[(kappa[j--] >> 24)       ]; if (j < 0) j = N - 1;
			inter[i] ^= T1[(kappa[j--] >> 16) & 0xff]; if (j < 0) j = N - 1;
			inter[i] ^= T2[(kappa[j--] >>  8) & 0xff]; if (j < 0) j = N - 1;
			inter[i] ^= T3[(kappa[j  ]      ) & 0xff];
		}
		kappa[0] = inter[0] ^ rc[r];
		for (i = 1; i < N; i++) {
			kappa[i] = inter[i];
		}
	}

	/*
	 * generate inverse key schedule: K'^0 = K^R, K'^R = K^0, K'^r = theta(K^{R-r}):
	 */
	for (i = 0; i < 4; i++) {
		structpointer->roundKeyDec[0][i] = structpointer->roundKeyEnc[R][i];
		structpointer->roundKeyDec[R][i] = structpointer->roundKeyEnc[0][i];
	}
	for (r = 1; r < R; r++) {
		for (i = 0; i < 4; i++) {
			u32 v = structpointer->roundKeyEnc[R - r][i];
			structpointer->roundKeyDec[r][i] =
				T0[T4[(v >> 24)       ] & 0xff] ^
				T1[T4[(v >> 16) & 0xff] & 0xff] ^
				T2[T4[(v >>  8) & 0xff] & 0xff] ^
				T3[T4[(v      ) & 0xff] & 0xff];
		}
	}
}

/**
 * Either encrypt or decrypt a data block, according to the key schedule.
 * 
 * @param	block		the data block to be encrypted/decrypted.
 * @param	roundKey	the key schedule to be used.
 * @param	R			number of rounds.
 */
static void crypt(const u8 plaintext[/*16*/], u8 ciphertext[/*16*/],
				  const u32 roundKey[MAX_ROUNDS + 1][4], int R) {
	int i, pos, r;
	u32 state[4];
	u32 inter[4];

    /*
	 * map plaintext block to cipher state (mu)
	 * and add initial round key (sigma[K^0]):
	 */
    for (i = 0, pos = 0; i < 4; i++, pos += 4) {
		state[i] =
			(plaintext[pos    ] << 24) ^
			(plaintext[pos + 1] << 16) ^
			(plaintext[pos + 2] <<  8) ^
			(plaintext[pos + 3]      ) ^
			roundKey[0][i];
    }

    /*
     * R - 1 full rounds:
     */
    for (r = 1; r < R; r++) {
		inter[0] =
			T0[(state[0] >> 24)       ] ^
			T1[(state[1] >> 24)       ] ^
			T2[(state[2] >> 24)       ] ^
			T3[(state[3] >> 24)       ] ^
			roundKey[r][0];
		inter[1] =
			T0[(state[0] >> 16) & 0xff] ^
			T1[(state[1] >> 16) & 0xff] ^
			T2[(state[2] >> 16) & 0xff] ^
			T3[(state[3] >> 16) & 0xff] ^
			roundKey[r][1];
		inter[2] =
			T0[(state[0] >>  8) & 0xff] ^
			T1[(state[1] >>  8) & 0xff] ^
			T2[(state[2] >>  8) & 0xff] ^
			T3[(state[3] >>  8) & 0xff] ^
			roundKey[r][2];
		inter[3] =
			T0[(state[0]      ) & 0xff] ^
			T1[(state[1]      ) & 0xff] ^
			T2[(state[2]      ) & 0xff] ^
			T3[(state[3]      ) & 0xff] ^
			roundKey[r][3];
		state[0] = inter[0];
		state[1] = inter[1];
		state[2] = inter[2];
		state[3] = inter[3];
    }

    /*
	 * last round:
	 */
	inter[0] =
		(T0[(state[0] >> 24)       ] & 0xff000000U) ^
		(T1[(state[1] >> 24)       ] & 0x00ff0000U) ^
		(T2[(state[2] >> 24)       ] & 0x0000ff00U) ^
		(T3[(state[3] >> 24)       ] & 0x000000ffU) ^
		roundKey[R][0];
	inter[1] =
		(T0[(state[0] >> 16) & 0xff] & 0xff000000U) ^
		(T1[(state[1] >> 16) & 0xff] & 0x00ff0000U) ^
		(T2[(state[2] >> 16) & 0xff] & 0x0000ff00U) ^
		(T3[(state[3] >> 16) & 0xff] & 0x000000ffU) ^
		roundKey[R][1];
	inter[2] =
		(T0[(state[0] >>  8) & 0xff] & 0xff000000U) ^
		(T1[(state[1] >>  8) & 0xff] & 0x00ff0000U) ^
		(T2[(state[2] >>  8) & 0xff] & 0x0000ff00U) ^
		(T3[(state[3] >>  8) & 0xff] & 0x000000ffU) ^
		roundKey[R][2];
	inter[3] =
		(T0[(state[0]      ) & 0xff] & 0xff000000U) ^
		(T1[(state[1]      ) & 0xff] & 0x00ff0000U) ^
		(T2[(state[2]      ) & 0xff] & 0x0000ff00U) ^
		(T3[(state[3]      ) & 0xff] & 0x000000ffU) ^
		roundKey[R][3];

	/*
	 * map cipher state to ciphertext block (mu^{-1}):
	 */
    for (i = 0, pos = 0; i < 4; i++, pos += 4) {
        u32 w = inter[i];
        ciphertext[pos    ] = (u8)(w >> 24);
        ciphertext[pos + 1] = (u8)(w >> 16);
        ciphertext[pos + 2] = (u8)(w >>  8);
        ciphertext[pos + 3] = (u8)(w      );
    }
}

/**
 * Encrypt a data block.
 * 
 * @param	structpointer	the expanded key.
 * @param	plaintext		the data block to be encrypted.
 * @param	ciphertext		the encrypted data block.
 */
void ANUBISencrypt(const struct ANUBISstruct * const structpointer,
				   const unsigned char * const plaintext,
				         unsigned char * const ciphertext) {
	crypt(plaintext, ciphertext, structpointer->roundKeyEnc, structpointer->R);
}

/**
 * Decrypt a data block.
 * 
 * @param	structpointer	the expanded key.
 * @param	ciphertext		the data block to be decrypted.
 * @param	plaintext		the decrypted data block.
 */
void ANUBISdecrypt(const struct ANUBISstruct * const structpointer,
				   const unsigned char * const ciphertext,
				         unsigned char * const plaintext) {
	crypt(ciphertext, plaintext, structpointer->roundKeyDec, structpointer->R);
}

/*
#include <time.h>

#define ITERATIONS 1000000

static void keySetupTiming(int keyBits) {
	int i;
	struct ANUBISstruct a;
	u8 key[MAX_KEYSIZEB], block[BLOCKSIZEB];
	clock_t elapsed;
	float sec;
	
	memset(key, 0, MAX_KEYSIZEB);
	memset(block, 0, BLOCKSIZEB);
	printf("%d-bit keys...", keyBits);
	a.keyBits = keyBits;
	elapsed = -clock();
	for (i = 0; i < ITERATIONS; i++) {
		ANUBISkeysetup(key, &a);
	}
	elapsed += clock();
	sec = (float)elapsed/CLOCKS_PER_SEC;
	printf(" %.1f s, %.1f cycles.\n",
		sec,
		(float)550e6*sec/ITERATIONS);
}

static void cipherTiming(int keyBits) {
	int i;
	struct ANUBISstruct a;
	u8 key[MAX_KEYSIZEB], block[BLOCKSIZEB];
	clock_t elapsed;
	float sec, mbs;
	
	memset(key, 0, MAX_KEYSIZEB);
	memset(block, 0, BLOCKSIZEB);
	printf("%d-bit keys...", keyBits);
	a.keyBits = keyBits;
	ANUBISkeysetup(key, &a);
	elapsed = -clock();
	for (i = 0; i < ITERATIONS; i++) {
		ANUBISencrypt(&a, block, block);
	}
	elapsed += clock();
	sec = (float)elapsed/CLOCKS_PER_SEC;
	mbs = (float)8*BLOCKSIZEB*ITERATIONS/sec/1000000;
	printf(" %.1f s, %.1f Mb/s, %.1f cyc/byte, %.0f cyc/block.\n",
		sec,
		mbs,
		(float)550*8/mbs,
		(float)550*128/mbs);
}
*/

