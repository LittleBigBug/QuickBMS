/***************************************************
ZKdef.h	
ver: 3.00 (25 Jun 2006)


ZK-Crypt 

Submited by: Carmi Gressel et al    (carmi@fortressgb.com)
		FortressGB					(avi@fortressgb.com)

Response to Ecrypt call for eSTREAM Profile II (HW)

Code IS NOT OPTIMIZED for speed
***************************************************/


#ifndef ZKdef_H
#define ZKdef_H


typedef u32 DWORD;
typedef u8 BYTE;
typedef u16 WORD;
typedef signed char	BOOL;
#define TRUE	(1)
#define FALSE	(0)



#define BIT_ZERO				0x00

#define BIT_ONE					0x01
#define BIT_TWO					0x02
#define BIT_THREE				0x04
#define BIT_FOUR				0x08
#define BIT_FIVE				0x10
#define BIT_SIX					0x20
#define BIT_SEVEN				0x40
#define BIT_EIGHT				0x80


#define GETBIT_ONE( _byte)		(((_byte) & 0x0001))
#define GETBIT_TWO( _byte)		(((_byte) & 0x0002)>>1)
#define GETBIT_THREE( _byte)	(((_byte) & 0x0004)>>2)
#define GETBIT_FOUR( _byte)		(((_byte) & 0x0008)>>3)
#define GETBIT_FIVE( _byte)		(((_byte) & 0x0010)>>4)
#define GETBIT_SIX( _byte)		(((_byte) & 0x0020)>>5)
#define GETBIT_SEVEN( _byte)	(((_byte) & 0x0040)>>6)
#define GETBIT_EIGHT( _byte)	(((_byte) & 0x0080)>>7)
#define GETBIT_NINE( _byte)		(((_byte) & 0x0100)>>8)

#define GETBIT_COUNT(_byte, _count)	(((_byte) & (_count -1)) & BIT_ONE)

#define ZK_OPTI_DEFUALT			0x00
#define ZK_OPTI_SUPER			0x01
#define ZK_OPTI_LEGACY			0x02


#define CTRL_PRND_CLOCK			0x01

#define CTRL_NLFSR_CLOCK		0x01
#define CTRL_NLFSR_LOAD			0x02
#define CTRL_NLFSR_SLIP_LEFT	0x04
#define CTRL_NLFSR_SLIP_RIGHT	0x08
#define CTRL_NLFSR_BROWN		0x10
#define CTRL_NLFSR_FB			0x20
#define CTRL_NLFSR_XX1			0x40
#define CTRL_NLFSR_XX2			0x80


#define CTRL_FEEDBACK_ENABLE	0x01
#define CTRL_FEEDBACK_RESET		0x02
#define CTRL_FEEDBACK_MAC		0x04
#define CTRL_FEEDBACK_CIPH		0x08

#define CTRL_FEEDBACK_OUTPUT	0x80		/* result output from the engine */



#define CTRL_HASH_VECTOR_A		0x01
#define CTRL_HASH_VECTOR_B		0x02
#define CTRL_HASH_VECTOR_C		0x04
#define CTRL_HASH_VECTOR_D		0x08
#define CTRL_HASH_ODDN_TOP		0x10
#define CTRL_HASH_ODDN_MID		0x20
#define CTRL_HASH_ODDN_BOT		0x40
#define CTRL_HASH_ODDN_4TH		0x80

#define CTRL_CLOCKS_LOAD		0x02
#define CTRL_CLOCKS_DAS			0x04
#define CTRL_CLOCKS_JUGG		0x08
#define CTRL_CLOCKS_4TH			0x10
#define CTRL_CLOCKS_SLIP		0x20


#define	CTRL_CLOCKS_FB_Q12		0x0001
#define	CTRL_CLOCKS_FB_Q18		0x0002
#define	CTRL_CLOCKS_FB_Q13		0x0004
#define	CTRL_CLOCKS_FB_Q17		0x0008
#define	CTRL_CLOCKS_FB_Q14		0x0010
#define	CTRL_CLOCKS_FB_Q16		0x0020
#define CTRL_CLOCKS_FB_THSH08	0x0040
#define	CTRL_CLOCKS_FB_THSH15	0x0080
#define	CTRL_CLOCKS_FB_THSH18	0x0100
#define	CTRL_CLOCKS_FB_THSH31	0x0200
#define	CTRL_CLOCKS_FB_JUGG		0x0400
#define	CTRL_CLOCKS_FB_DAS		0x0800
#define	CTRL_CLOCKS_FB_4TH		0x1000
#define	CTRL_CLOCKS_FB_ODDN_MID	0x2000
#define CTRL_CLOCKS_FB_QTA		0x4000
#define CTRL_CLOCKS_FB_XX2		0x8000



#define OUTP_CLOCKS_BRN			0x01
#define OUTP_CLOCKS_LFT_SLP		0x02
#define OUTP_CLOCKS_RGT_SLP		0x04
#define OUTP_CLOCKS_CTRL		0x08

#define DELAY_TOP_CLOCKS		0x0001
#define DELAY_MID_CLOCKS		0x0002
#define DELAY_BOT_CLOCKS		0x0004
#define DELAY_TOP_BROWN			0x0008
#define DELAY_MID_BROWN			0x0010
#define DELAY_BOT_BROWN			0x0020
#define DELAY_LH_SLIP			0x0040
#define DELAY_RH_SLIP			0x0080
#define	DELAY_PR_SLIP			0x0100





#endif /* #ifndef ZKdef_H */
