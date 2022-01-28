.386
.model flat,stdcall
option casemap:none

include \masm32\include\winmm.inc
include \masm32\include\masm32rt.inc
include \masm32\include\msvcrt.inc

includelib \masm32\lib\kernel32.lib
includelib \masm32\lib\user32.lib
includelib \masm32\lib\winmm.lib
includelib \masm32\lib\msvcrt.lib

.data

byte_8F50C4	db 0
		align 4
		db    1
		db    1
		db    2
		db    2
		db    3
		db    3
		db    4
		db    4
		db    5
		db    5
		db    6
		db    6
		db    7
		db    7
		db    8
		db    8
		db    9
		db    9
		db  0Ah
		db  0Ah
		db  0Bh
		db  0Bh
		db  0Ch
		db  0Ch
		db  0Dh
		db  0Dh
		db  0Eh
		db  0Eh
		db  0Fh
		db  0Fh
		db  10h
		db  10h
		db  11h
		db  11h
		db    0
		db    0

byte_8F50EC	db 1Dh
		db  1Dh
		db  1Dh
		db  1Dh
		db  1Ch
		db  1Ch
		db  1Bh
		db  1Bh
		db  1Ah
		db  1Ah
		db  19h
		db  19h
		db  18h
		db  18h
		db  17h
		db  17h
		db  16h
		db  16h
		db  15h
		db  15h
		db  14h
		db  14h
		db  13h
		db  13h
		db  12h
		db  12h
		db  11h
		db  11h
		db  10h
		db  10h
		db  0Fh
		db  0Fh
		db  0Eh
		db  0Eh
		db  0Dh
		db  0Dh
		db  0Ch
		db  0Ch
		db    0
		db    0
		db 0DCh	; ‹
		db 0BCh	; º
		db  86h	; Ü
		db    0

dword_889930	dd 1
		db    9
		db    0
		db    0
		db    0
		db  11h
		db    0
		db    0
		db    0
		db  19h
		db    0
		db    0
		db    0
		db  21h	; !
		db    0
		db    0
		db    0
		db  31h	; 1
		db    0
		db    0
		db    0
		db  41h	; A
		db    0
		db    0
		db    0
		db  61h	; a
		db    0
		db    0
		db    0
		db  81h	; Å
		db    0
		db    0
		db    0
		db 0C1h	; ¡
		db    0
		db    0
		db    0
		db    1
		db    1
		db    0
		db    0
		db  81h	; Å
		db    1
		db    0
		db    0
		db    1
		db    2
		db    0
		db    0
		db    1
		db    3
		db    0
		db    0
		db    1
		db    4
		db    0
		db    0
		db    1
		db    6
		db    0
		db    0
		db    1
		db    8
		db    0
		db    0
		db    1
		db  0Ch
		db    0
		db    0
		db    1
		db  10h
		db    0
		db    0
		db    1
		db  18h
		db    0
		db    0
		db    1
		db  20h
		db    0
		db    0
		db    1
		db  30h	; 0
		db    0
		db    0
		db    1
		db  40h	; @
		db    0
		db    0
		db    1
		db  60h	; `
		db    0
		db    0
		db    1
		db  80h	; Ä
		db    0
		db    0
		db    1
		db 0C0h	; ¿
		db    0
		db    0
		db    1
		db    0
		db    1
		db    0
		db    1
		db  80h	; Ä
		db    1
		db    0
		db    1
		db    0
		db    2
		db    0
		db    1
		db    0
		db    3
		db    0
		db    1
		db    0
		db    4
		db    0
		db    1
		db    0
		db    6
		db    0
		db    1
		db    0
		db    8
		db    0
		db    1
		db    0
		db  0Ch
		db    0
		db    1
		db    0
		db  10h
		db    0
		db    1
		db    0
		db  18h
		db    0
		db    1
		db    0
		db  20h
		db    0
		db    1
		db    0
		db  30h	; 0
		db    0
		db 0DBh	; €
		db  0Fh
		db  49h	; I
		db  40h	; @
		db 0DBh	; €
		db  0Fh
		db 0C9h	; …
		db  40h	; @
		db  35h	; 5
		db 0FAh	; ˙
		db  8Eh	; é
		db  3Ch	; <
		db 0E1h	; ·
		db  2Eh	; .
		db  65h	; e
		db  42h	; B
		db 0DBh	; €
		db  0Fh
		db  49h	; I
		db  40h	; @
		db 0DBh	; €
		db  0Fh
		db 0C9h	; …
		db  40h	; @
		db  35h	; 5
		db 0FAh	; ˙
		db  8Eh	; é
		db  3Ch	; <
		db 0E1h	; ·
		db  2Eh	; .
		db  65h	; e
		db  42h	; B
		db 0FFh
		db 0FFh
		db  7Fh	; 
		db 0FFh
		db 0FFh
		db 0FFh
		db  7Fh	; 
		db  7Fh	; 
		db 0DBh	; €
		db  0Fh
		db  49h	; I
		db  40h	; @
		db 0DBh	; €
		db  0Fh
		db 0C9h	; …
		db  40h	; @
		db 0F3h	; Û
		db    4
		db  35h	; 5
		db  3Fh	; ?
		db  35h	; 5
		db 0FAh	; ˙
		db  8Eh	; é
		db  3Ch	; <
		db 0E1h	; ·
		db  2Eh	; .
		db  65h	; e
		db  42h	; B
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    1
		db    0
		db    0
		db    0
		db    0
		db 0DBh	; €
		db  0Fh
		db  49h	; I
		db  40h	; @
		db 0DBh	; €
		db  0Fh
		db 0C9h	; …
		db  40h	; @
		db  35h	; 5
		db 0FAh	; ˙
		db  8Eh	; é
		db  3Ch	; <
		db 0E1h	; ·
		db  2Eh	; .
		db  65h	; e
		db  42h	; B
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0


.xmm
.code

DllEntry proc hInstance:HINSTANCE, reason:DWORD, reserved1:DWORD
	mov  eax,TRUE
	ret
DllEntry Endp

zen_decompress	proc near

var_3630	= dword	ptr -3630h
var_362C	= dword	ptr -362Ch
var_3628	= dword	ptr -3628h
var_3624	= dword	ptr -3624h
var_3620	= dword	ptr -3620h
var_361C	= dword	ptr -361Ch
var_3618	= dword	ptr -3618h
var_3614	= dword	ptr -3614h
var_3610	= dword	ptr -3610h
var_360C	= byte ptr -360Ch
var_340C	= word ptr -340Ch
var_2C0C	= word ptr -2C0Ch
var_240C	= dword	ptr -240Ch
var_2408	= byte ptr -2408h
var_2208	= word ptr -2208h
var_1A08	= word ptr -1A08h
var_1208	= dword	ptr -1208h
var_1204	= byte ptr -1204h
var_1004	= word ptr -1004h
var_804		= word ptr -804h
var_4		= dword	ptr -4
arg_0		= dword	ptr  4
arg_4		= dword	ptr  8
arg_8		= dword	ptr  0Ch

		mov	eax, 3630h
		call	__alloca_probe
		mov	[esp+3630h+var_4], eax
		mov	eax, [esp+3630h+arg_0]
		movzx	ecx, byte ptr [eax+1]
		movzx	edx, byte ptr [eax+2]
		push	ebx
		mov	ebx, [esp+3634h+arg_4]
		push	ebp
		push	esi
		movzx	esi, byte ptr [eax]
		shl	esi, 8
		or	esi, ecx
		shl	esi, 8
		or	esi, edx
		add	eax, 3
		shl	esi, 8
		push	edi
		mov	[esp+3640h+var_361C], ebx
		mov	[esp+3640h+var_1208], 13h
		mov	[esp+3640h+var_3610], 200h
		mov	[esp+3640h+var_240C], 134h
		mov	edi, 18h
		mov	[esp+3640h+var_362C], eax
		test	al, 1
		jz	short loc_6BCFD9
		movzx	ecx, byte ptr [eax]
		or	esi, ecx
		inc	eax
		mov	edi, 20h
		mov	[esp+3640h+var_362C], eax
		jmp	short loc_6BCFD9


loc_6BCFD5:
		mov	eax, [esp+3640h+var_362C]

loc_6BCFD9:
		cmp	edi, 10h
		jg	short loc_6BCFFF
		movzx	edx, byte ptr [eax]
		movzx	ecx, byte ptr [eax+1]
		shl	edx, 8
		or	edx, ecx
		mov	ecx, 10h
		sub	ecx, edi
		shl	edx, cl
		add	eax, 2
		mov	[esp+3640h+var_362C], eax
		or	esi, edx
		add	edi, 10h

loc_6BCFFF:
		test	esi, esi
		jns	short loc_6BD06F
		dec	edi
		add	esi, esi
		cmp	edi, 10h
		jg	short loc_6BD02C
		movzx	edx, byte ptr [eax]
		movzx	ecx, byte ptr [eax+1]
		shl	edx, 8
		or	edx, ecx
		mov	ecx, 10h
		sub	ecx, edi
		shl	edx, cl
		add	eax, 2
		mov	[esp+3640h+var_362C], eax
		or	esi, edx
		add	edi, 10h

loc_6BD02C:
		mov	ebp, esi
		sub	edi, 10h
		shr	ebp, 10h
		shl	esi, 10h
		cmp	edi, 10h
		jg	short loc_6BD05D
		movzx	edx, byte ptr [eax]
		movzx	ecx, byte ptr [eax+1]
		shl	edx, 8
		or	edx, ecx
		mov	ecx, 10h
		sub	ecx, edi
		shl	edx, cl
		add	eax, 2
		mov	[esp+3640h+var_362C], eax
		or	esi, edx
		add	edi, 10h

loc_6BD05D:
		mov	edx, esi
		shr	edx, 18h
		shl	ebp, 8
		or	edx, ebp
		shl	esi, 8
		sub	edi, 8
		jmp	short loc_6BD080

loc_6BD06F:
		add	esi, esi
		mov	edx, esi
		shr	edx, 17h
		inc	edx
		shl	edx, 0Ah
		shl	esi, 9
		sub	edi, 0Ah

loc_6BD080:	
		cmp	edi, 10h
		mov	[esp+3640h+var_3618], edx
		jg	short loc_6BD0AA
		movzx	ebp, byte ptr [eax]
		movzx	ecx, byte ptr [eax+1]
		shl	ebp, 8
		or	ebp, ecx
		mov	ecx, 10h
		sub	ecx, edi
		shl	ebp, cl
		add	eax, 2
		mov	[esp+3640h+var_362C], eax
		or	esi, ebp
		add	edi, 10h

loc_6BD0AA:
		dec	edi
		test	esi, esi
		jns	loc_6BD530
		xor	ecx, ecx
		mov	ebp, 10h
		add	esi, esi
		mov	[esp+3640h+var_3624], ecx
		sub	ebp, edi

loc_6BD0C2:
		cmp	edi, 10h
		jg	short loc_6BD0E6
		movzx	edx, byte ptr [eax]
		movzx	ecx, byte ptr [eax+1]
		shl	edx, 8
		or	edx, ecx
		mov	ecx, ebp
		shl	edx, cl
		mov	ecx, [esp+3640h+var_3624]
		add	eax, 2
		add	edi, 10h
		or	esi, edx
		sub	ebp, 10h

loc_6BD0E6:
		mov	edx, esi
		add	esi, esi
		shr	edx, 1Dh
		add	esi, esi
		mov	[esp+ecx+3640h+var_1204], dl
		inc	ecx
		add	esi, esi
		sub	edi, 3
		add	ebp, 3
		cmp	ecx, 13h
		mov	[esp+3640h+var_3624], ecx
		jl	short loc_6BD0C2
		lea	ecx, [esp+3640h+var_1208]
		mov	[esp+3640h+var_362C], eax
		call	sub_6B92B0
		xor	edx, edx
		mov	[esp+3640h+var_3628], edx
		mov	[esp+3640h+var_3630], edx

loc_6BD122:
		cmp	edi, 10h
		jg	short loc_6BD14C
		mov	eax, [esp+3640h+var_362C]
		movzx	ebp, byte ptr [eax]
		movzx	ecx, byte ptr [eax+1]
		shl	ebp, 8
		or	ebp, ecx
		mov	ecx, 10h
		sub	ecx, edi
		shl	ebp, cl
		add	eax, 2
		mov	[esp+3640h+var_362C], eax
		or	esi, ebp
		add	edi, 10h

loc_6BD14C:	
		mov	ecx, esi
		rol	ecx, 0Ah
		mov	eax, ecx
		and	eax, 3FFh
		movsx	eax, [esp+eax*2+3640h+var_1004]
		test	eax, eax
		jge	short loc_6BD179

loc_6BD164:
		rol	ecx, 1
		mov	ebp, ecx
		and	ebp, 1
		sub	ebp, eax
		movsx	eax, [esp+ebp*2+3640h+var_804]
		test	eax, eax
		jl	short loc_6BD164

loc_6BD179:
		movzx	ecx, [esp+eax+3640h+var_1204]
		shl	esi, cl
		sub	edi, ecx
		cmp	eax, 10h
		jge	short loc_6BD19D
		mov	[esp+edx+3640h+var_360C], al
		inc	edx
		mov	[esp+3640h+var_3630], edx
		test	eax, eax
		jz	short loc_6BD203
		mov	[esp+3640h+var_3628], eax
		jmp	short loc_6BD203

loc_6BD19D:
		mov	ebp, esi
		jnz	short loc_6BD1BA
		shr	ebp, 1Eh
		add	esi, esi
		add	ebp, 3
		add	esi, esi
		sub	edi, 2
		test	ebp, ebp
		jle	short loc_6BD203
		mov	ecx, [esp+3640h+var_3628]
		push	ebp
		push	ecx
		jmp	short loc_6BD1EE

loc_6BD1BA:
		cmp	eax, 11h
		jnz	short loc_6BD1DB
		add	esi, esi
		shr	ebp, 1Dh
		add	esi, esi
		add	ebp, 3
		add	esi, esi
		sub	edi, 3
		test	ebp, ebp
		jle	short loc_6BD203
		push	ebp
		lea	eax, [esp+edx+3644h+var_360C]
		push	0
		jmp	short loc_6BD1F6

loc_6BD1DB:
		shr	ebp, 19h
		add	ebp, 0Bh
		shl	esi, 7
		sub	edi, 7
		test	ebp, ebp
		jle	short loc_6BD203
		push	ebp		; size_t
		push	0		; int

loc_6BD1EE:
		mov	eax, [esp+3648h+var_3630]
		lea	eax, [esp+eax+3648h+var_360C]

loc_6BD1F6:
		push	eax
		;call	_memset
		call	crt_memset
		add	esp, 0Ch
		add	[esp+3640h+var_3630], ebp

loc_6BD203:
		mov	edx, [esp+3640h+var_3630]
		cmp	edx, [esp+3640h+var_3610]
		jl	loc_6BD122
		lea	ecx, [esp+3640h+var_3610]
		call	sub_6B92B0
		mov	[esp+3640h+var_3630], 0

loc_6BD222:
		cmp	edi, 10h
		jg	short loc_6BD24C
		mov	eax, [esp+3640h+var_362C]
		movzx	edx, byte ptr [eax]
		movzx	ecx, byte ptr [eax+1]
		shl	edx, 8
		or	edx, ecx
		mov	ecx, 10h
		sub	ecx, edi
		shl	edx, cl
		add	eax, 2
		mov	[esp+3640h+var_362C], eax
		or	esi, edx
		add	edi, 10h

loc_6BD24C:
		mov	ecx, esi
		rol	ecx, 0Ah
		mov	edx, ecx
		and	edx, 3FFh
		movsx	eax, [esp+edx*2+3640h+var_1004]
		test	eax, eax
		jge	short loc_6BD27A

loc_6BD265:
		rol	ecx, 1
		mov	edx, ecx
		and	edx, 1
		sub	edx, eax
		movsx	eax, [esp+edx*2+3640h+var_804]
		test	eax, eax
		jl	short loc_6BD265

loc_6BD27A:	
		movzx	ecx, [esp+eax+3640h+var_1204]
		shl	esi, cl
		sub	edi, ecx
		cmp	eax, 10h
		jge	short loc_6BD2A5
		mov	ecx, [esp+3640h+var_3630]
		mov	[esp+ecx+3640h+var_2408], al
		inc	ecx
		mov	[esp+3640h+var_3630], ecx
		test	eax, eax
		jz	short loc_6BD303
		mov	[esp+3640h+var_3628], eax
		jmp	short loc_6BD303

loc_6BD2A5:
		mov	ebp, esi
		jnz	short loc_6BD2C2
		shr	ebp, 1Eh
		add	esi, esi
		add	ebp, 3
		add	esi, esi
		sub	edi, 2
		test	ebp, ebp
		jle	short loc_6BD303
		mov	ecx, [esp+3640h+var_3628]
		push	ebp
		push	ecx
		jmp	short loc_6BD2EB

loc_6BD2C2:
		cmp	eax, 11h
		jnz	short loc_6BD2D8
		add	esi, esi
		shr	ebp, 1Dh
		add	esi, esi
		add	ebp, 3
		add	esi, esi
		sub	edi, 3
		jmp	short loc_6BD2E4

loc_6BD2D8:	
		shr	ebp, 19h
		add	ebp, 0Bh
		shl	esi, 7
		sub	edi, 7

loc_6BD2E4:
		test	ebp, ebp
		jle	short loc_6BD303
		push	ebp		; size_t
		push	0		; int

loc_6BD2EB:
		mov	eax, [esp+3648h+var_3630]
		lea	eax, [esp+eax+3648h+var_2408]
		push	eax		; void *
		;call	_memset
		call	crt_memset
		add	esp, 0Ch
		add	[esp+3640h+var_3630], ebp

loc_6BD303:
		mov	edx, [esp+3640h+var_3630]
		cmp	edx, [esp+3640h+var_240C]
		jl	loc_6BD222
		lea	ecx, [esp+3640h+var_240C]
		call	sub_6B92B0
		mov	eax, 1
		mov	[esp+3640h+var_3620], eax
		mov	[esp+3640h+var_3630], eax
		mov	[esp+3640h+var_3628], eax
		mov	[esp+3640h+var_3624], eax
		mov	eax, [esp+3640h+var_3618]
		lea	ecx, [ebx+eax]
		mov	[esp+3640h+var_3614], ecx

loc_6BD340:
		cmp	edi, 10h
		jg	short loc_6BD36A
		mov	eax, [esp+3640h+var_362C]
		movzx	edx, byte ptr [eax]
		movzx	ecx, byte ptr [eax+1]
		shl	edx, 8
		or	edx, ecx
		mov	ecx, 10h
		sub	ecx, edi
		shl	edx, cl
		add	eax, 2
		mov	[esp+3640h+var_362C], eax
		or	esi, edx
		add	edi, 10h

loc_6BD36A:
		mov	eax, esi
		rol	eax, 0Ah
		mov	edx, eax
		and	edx, 3FFh
		movsx	edx, [esp+edx*2+3640h+var_340C]
		test	edx, edx
		jge	short loc_6BD398

loc_6BD383:
		rol	eax, 1
		mov	ecx, eax
		and	ecx, 1
		sub	ecx, edx
		movsx	edx, [esp+ecx*2+3640h+var_2C0C]
		test	edx, edx
		jl	short loc_6BD383

loc_6BD398:
		movzx	ecx, [esp+edx+3640h+var_360C]
		shl	esi, cl
		sub	edi, ecx
		cmp	edx, 100h
		jge	short loc_6BD3B1
		mov	[ebx], dl
		inc	ebx
		jmp	loc_6BD520

loc_6BD3B1:
		cmp	edi, 10h
		jg	short loc_6BD3DB
		mov	eax, [esp+3640h+var_362C]
		movzx	ebp, byte ptr [eax]
		movzx	ecx, byte ptr [eax+1]
		shl	ebp, 8
		or	ebp, ecx
		mov	ecx, 10h
		sub	ecx, edi
		shl	ebp, cl
		add	eax, 2
		mov	[esp+3640h+var_362C], eax
		or	esi, ebp
		add	edi, 10h

loc_6BD3DB:
		mov	ecx, esi
		rol	ecx, 0Ah
		mov	eax, ecx
		and	eax, 3FFh
		movsx	eax, [esp+eax*2+3640h+var_2208]
		test	eax, eax
		jge	short loc_6BD408

loc_6BD3F3:
		rol	ecx, 1
		mov	ebp, ecx
		and	ebp, 1
		sub	ebp, eax
		movsx	eax, [esp+ebp*2+3640h+var_1A08]
		test	eax, eax
		jl	short loc_6BD3F3

loc_6BD408:
		movzx	ecx, [esp+eax+3640h+var_2408]
		mov	ebp, [esp+3640h+var_3620]
		shl	esi, cl
		movzx	ecx, [esp+eax+3640h+var_2408]
		sub	edi, ecx
		cmp	eax, 4
		jge	short loc_6BD484
		test	eax, eax
		jnz	short loc_6BD42D
		mov	eax, ebp
		jmp	short loc_6BD45F

loc_6BD42D:
		cmp	eax, 1
		jnz	short loc_6BD438
		mov	eax, [esp+3640h+var_3630]
		jmp	short loc_6BD457

loc_6BD438:
		cmp	eax, 2
		jnz	short loc_6BD443
		mov	eax, [esp+3640h+var_3628]
		jmp	short loc_6BD44F

loc_6BD443:
		mov	ecx, [esp+3640h+var_3628]
		mov	eax, [esp+3640h+var_3624]
		mov	[esp+3640h+var_3624], ecx

loc_6BD44F:
		mov	ecx, [esp+3640h+var_3630]
		mov	[esp+3640h+var_3628], ecx

loc_6BD457:
		mov	[esp+3640h+var_3630], ebp
		mov	[esp+3640h+var_3620], eax

loc_6BD45F:
		mov	ecx, ebx
		sub	ecx, eax
		mov	cl, [ecx]
		mov	[ebx], cl
		inc	ebx
		and	edx, 0FFh
		mov	ecx, ebx
		mov	ebp, edx
		sub	ecx, eax

loc_6BD474:
		mov	dl, [ecx]
		mov	[ebx], dl
		inc	ebx
		inc	ecx
		sub	ebp, 1
		jns	short loc_6BD474
		jmp	loc_6BD520

loc_6BD484:
		cmp	edi, 10h
		jg	short loc_6BD4AE
		mov	ebx, [esp+3640h+var_362C]
		movzx	ebp, byte ptr [ebx]
		movzx	ecx, byte ptr [ebx+1]
		shl	ebp, 8
		or	ebp, ecx
		mov	ecx, 10h
		sub	ecx, edi
		shl	ebp, cl
		add	ebx, 2
		mov	[esp+3640h+var_362C], ebx
		or	esi, ebp
		add	edi, 10h

loc_6BD4AE:
		add	eax, 0FFFFFFFCh
		mov	ebp, eax
		sar	ebp, 3
		movzx	ecx, ss:byte_8F50EC[ebp]
		mov	ebx, esi
		shr	ebx, cl
		movzx	ecx, ss:byte_8F50C4[ebp]
		mov	[esp+3640h+var_3624], ecx
		shl	esi, cl
		sub	edi, ecx
		mov	ecx, [esp+3640h+var_3628]
		mov	[esp+3640h+var_3624], ecx
		mov	ecx, [esp+3640h+var_3630]
		mov	[esp+3640h+var_3628], ecx
		mov	ecx, [esp+3640h+var_3620]
		and	ebx, 0FFFFFFF8h
		and	eax, 7
		or	eax, ebx
		add	eax, ds:dword_889930[ebp*4]
		mov	ebx, [esp+3640h+var_361C]
		mov	[esp+3640h+var_3630], ecx
		mov	ecx, ebx
		sub	ecx, eax
		movzx	ecx, byte ptr [ecx]
		mov	[ebx], cl
		inc	ebx
		and	edx, 0FFh
		mov	ecx, ebx
		mov	[esp+3640h+var_3620], eax
		mov	ebp, edx
		sub	ecx, eax

loc_6BD515:
		mov	dl, [ecx]
		mov	[ebx], dl
		inc	ebx
		inc	ecx
		sub	ebp, 1
		jns	short loc_6BD515

loc_6BD520:
		mov	[esp+3640h+var_361C], ebx
		cmp	ebx, [esp+3640h+var_3614]
		jb	loc_6BD340
		jmp	short loc_6BD570


loc_6BD530:
		mov	ecx, edi
		add	esi, esi
		and	ecx, 7
		jz	short loc_6BD53E
		shl	esi, cl
		and	edi, 0FFFFFFF8h

loc_6BD53E:	
		mov	ebp, edx
		test	edi, edi
		jz	short loc_6BD55B

loc_6BD544:
		test	ebp, ebp
		jz	short loc_6BD55B
		mov	ecx, esi
		shr	ecx, 18h
		mov	[ebx], cl
		sub	edi, 8
		inc	ebx
		shl	esi, 8
		dec	ebp
		test	edi, edi
		jnz	short loc_6BD544

loc_6BD55B:	
		;push	ebp
		;push	eax
		;push	ebx
		;call	unknown_libname_436
		add	[esp+364Ch+var_362C], ebp
		add	esp, 0Ch
		add	ebx, ebp
		mov	[esp+3640h+var_361C], ebx

loc_6BD570:
		mov	eax, [esp+3640h+arg_8]
		sub	eax, [esp+3640h+var_3618]
		mov	[esp+3640h+arg_8], eax
		test	eax, eax
		jg	loc_6BCFD5
		mov	ecx, [esp+3640h+var_4]
		pop	edi
		pop	esi
		pop	ebp
		pop	ebx
		add	esp, 3630h
		retn
zen_decompress	endp

__alloca_probe	proc near
		push	ecx
		lea	ecx, [esp+4]
		sub	ecx, eax
		sbb	eax, eax
		not	eax
		and	ecx, eax
		mov	eax, esp
		and	eax, 0FFFFF000h

cs10:
		cmp	ecx, eax
		jb	short cs20
		mov	eax, ecx
		pop	ecx
		xchg	eax, esp
		mov	eax, [eax]
		mov	[esp+0], eax
		retn

cs20:
		sub	eax, 1000h
		test	[eax], eax
		jmp	short cs10
__alloca_probe	endp

sub_6B92B0	proc near
var_110		= dword	ptr -110h
var_10C		= dword	ptr -10Ch
var_108		= dword	ptr -108h
var_104		= dword	ptr -104h
var_100		= dword	ptr -100h
var_FC		= dword	ptr -0FCh
var_F8		= dword	ptr -0F8h
var_F4		= dword	ptr -0F4h
var_F0		= dword	ptr -0F0h
var_EC		= dword	ptr -0ECh
var_E8		= dword	ptr -0E8h
var_E4		= dword	ptr -0E4h
var_E0		= dword	ptr -0E0h
var_DC		= dword	ptr -0DCh
var_D8		= dword	ptr -0D8h
var_D4		= dword	ptr -0D4h
var_D0		= dword	ptr -0D0h
var_CC		= dword	ptr -0CCh
var_C8		= dword	ptr -0C8h
var_C4		= dword	ptr -0C4h
var_88		= dword	ptr -88h
var_84		= dword	ptr -84h
var_80		= dword	ptr -80h
var_48		= dword	ptr -48h
var_44		= dword	ptr -44h
var_40		= dword	ptr -40h
var_3C		= dword	ptr -3Ch

		sub	esp, 110h
		xor	eax, eax
		push	ebx
		push	ebp
		push	esi
		mov	esi, ecx
		mov	ebx, [esi]
		xor	ebp, ebp
		cmp	ebx, ebp
		push	edi
		mov	[esp+120h+var_110], eax
		mov	[esp+120h+var_10C], eax
		mov	[esp+120h+var_108], eax
		mov	[esp+120h+var_104], eax
		mov	[esp+120h+var_100], eax
		mov	[esp+120h+var_FC], eax
		mov	[esp+120h+var_F8], eax
		mov	[esp+120h+var_F4], eax
		mov	[esp+120h+var_F0], eax
		mov	[esp+120h+var_EC], eax
		mov	[esp+120h+var_E8], eax
		mov	[esp+120h+var_E4], eax
		mov	[esp+120h+var_E0], eax
		mov	[esp+120h+var_DC], eax
		mov	[esp+120h+var_D8], eax
		mov	[esp+120h+var_D4], eax
		mov	[esp+120h+var_D0], eax
		jle	short loc_6B9321
		lea	ecx, [esi+4]
		mov	edx, ebx
		nop

loc_6B9310:
		movzx	eax, byte ptr [ecx]
		inc	[esp+eax*4+120h+var_110]
		lea	eax, [esp+eax*4+120h+var_110]
		inc	ecx
		sub	edx, 1
		jnz	short loc_6B9310

loc_6B9321:
		mov	eax, 1
		mov	[esp+120h+var_84], eax
		mov	[esp+120h+var_40], eax
		lea	ecx, [eax+1]
		mov	[esp+120h+var_C8], ebp
		xor	eax, eax
		lea	ecx, [ecx+0]

loc_6B9340:
		mov	edx, [esp+eax+120h+var_10C]
		mov	edi, [esp+eax+120h+var_C8]
		add	edi, edx
		add	edi, edi
		mov	[esp+eax+120h+var_C4], edi
		mov	edi, [esp+eax+120h+var_84]
		add	edi, ecx
		sub	ecx, edx
		mov	[esp+eax+120h+var_80], edi
		mov	[esp+eax+120h+var_3C], edi
		add	eax, 4
		add	ecx, ecx
		cmp	eax, 38h
		jle	short loc_6B9340
		test	ebx, ebx
		jle	loc_6B943E
		jmp	short loc_6B9380

loc_6B9380:
		movzx	eax, byte ptr [esi+ebp+4]
		test	eax, eax
		jz	loc_6B9435
		cmp	eax, 0Ah
		mov	ebx, [esp+eax*4+120h+var_CC]
		lea	ecx, [ebx+1]
		mov	[esp+eax*4+120h+var_CC], ecx
		jg	short loc_6B93D5
		mov	ecx, 0Ah
		sub	ecx, eax
		mov	eax, 1
		shl	eax, cl
		shl	ebx, cl
		test	eax, eax
		jle	loc_6B9435
		mov	ecx, eax
		mov	eax, ebp
		movzx	edx, ax
		mov	eax, edx
		shl	edx, 10h
		or	eax, edx
		shr	ecx, 1
		lea	edi, [esi+ebx*2+204h]
		rep stosd
		adc	ecx, ecx
		rep stosw
		jmp	short loc_6B9435

loc_6B93D5:
		cmp	eax, 0Bh
		mov	edx, [esp+eax*4+120h+var_88]
		lea	ecx, [edx+1]
		mov	[esp+eax*4+120h+var_88], ecx
		mov	[esi+edx*2+0A04h], bp
		mov	ecx, eax
		jle	short loc_6B9424

loc_6B93F5:
		test	bl, 1
		jnz	short loc_6B9435
		mov	eax, edx
		sub	eax, [esp+ecx*4+120h+var_44]
		neg	edx
		sar	eax, 1
		add	eax, [esp+ecx*4+120h+var_48]
		dec	ecx
		add	eax, [esp+ecx*4+120h+var_110]
		sar	ebx, 1
		cmp	ecx, 0Bh
		mov	[esi+eax*2+0A04h], dx
		mov	edx, eax
		jg	short loc_6B93F5

loc_6B9424:
		test	bl, 1
		jnz	short loc_6B9435
		neg	edx
		sar	ebx, 1
		mov	[esi+ebx*2+204h], dx

loc_6B9435:
		inc	ebp
		cmp	ebp, [esi]
		jl	loc_6B9380

loc_6B943E:
		pop	edi
		pop	esi
		pop	ebp
		pop	ebx
		add	esp, 110h
		retn
sub_6B92B0	endp

End DllEntry