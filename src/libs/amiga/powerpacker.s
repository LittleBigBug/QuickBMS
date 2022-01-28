    .text
    .globl pp_DecrunchBuffer
    .globl _pp_DecrunchBuffer
pp_DecrunchBuffer:
_pp_DecrunchBuffer:

#; PowerPacker Decrunch assembler subroutine V1.1
#;
#; call as:
#;    DecrunchBuffer (endcrun, buffer, efficiency);
#; with:
#;    endcrun   : UBYTE * just after last byte of crunched file
#;    buffer    : UBYTE * to memory block to decrunch in
#;    efficiency: Longword defining efficiency with wich file was crunched

#;;   move.l    4(a7),a0               # [2]
	movl     4(%esp),%esi
#;;   move.l    8(a7),a1               # [3]
	movl     8(%esp),%edi
#;;   move.l    12(a7),d0              # [4]
	movl     12(%esp),%ebx
#;;   movem.l   d1-d7/a2-a6,-(a7)      # [5]
	pushl    %ebp
	pushl    a5_l
	pushl    a4_l
	pushl    a3_l
	pushl    a2_l
	pushl    d7_l
	pushl    d6_l
	pushl    d5_l
	pushl    d4_l
	pushl    d3_l
	pushl    d2_l
	pushl    %edx
#;;   bsr.s     Decrunch               # [6]
	call     Decrunch
#;;   movem.l   (a7)+,d1-d7/a2-a6      # [7]
	popl     %edx
	popl     d2_l
	popl     d3_l
	popl     d4_l
	popl     d5_l
	popl     d6_l
	popl     d7_l
	popl     a2_l
	popl     a3_l
	popl     a4_l
	popl     a5_l
	popl     %ebp
#;;   rts                              # [8]
	ret      
Decrunch:
#;;   lea       myBitsTable(PC),a5     # [10]
	movl     $myBitsTable,a5_l
#;;   move.l    d0,(a5)                # [11]
	movl     a5_l,%ecx
	movl     %ebx,(%ecx)
#;;   move.l    a1,a2                  # [12]
	movl     %edi,a2_l
#;;   move.l    -(a0),d5               # [13]
	lea      -8(%esi),%esi
	movl     4(%esi),%eax
	movl     %eax,d5_l
#;;   moveq     #0,d1                  # [14]
	movl     $0,%edx
#;;   move.b    d5,d1                  # [15]
	movb     d5_b,%dl
#;;   lsr.l     #8,d5                  # [16]
	shrl     $8,d5_l
#;;   add.l     d5,a1                  # [17]
	addl     d5_l,%edi
#;;   move.l    -(a0),d5               # [18]
	movl     0(%esi),%eax
	movl     %eax,d5_l
#;;   lsr.l     d1,d5                  # [19]
	movl     %edx,%ecx
	shrl     %cl,d5_l
#;;   move.b    #32,d7                 # [20]
	movb     $32,d7_b
#;;   sub.b     d1,d7                  # [21]
	subb     %dl,d7_b
LoopCheckCrunch:
#;;   bsr.s     ReadBit                # [23]
	call     ReadBit
#;;   tst.b     d1                     # [24 NE]
	testb    %dl,%dl
#;;   bne.s     CrunchedBytes          # [25]
	jne      CrunchedBytes
NormalBytes:
#;;   moveq     #0,d2                  # [27]
	movl     $0,d2_l
Read2BitsRow:
#;;   moveq     #2,d0                  # [29]
	movl     $2,%ebx
#;;   bsr.s     ReadD1                 # [30]
	call     ReadD1
#;;   add.w     d1,d2                  # [31]
	addw     %dx,d2_w
#;;   cmp.w     #3,d1                  # [32 EQ]
	cmpw     $3,%dx
#;;   beq.s     Read2BitsRow           # [33]
	je       Read2BitsRow
ReadNormalByte:
#;;   move.w    #8,d0                  # [35]
	movw     $8,%bx
#;;   bsr.s     ReadD1                 # [36]
	call     ReadD1
#;;   move.b    d1,-(a1)               # [37]
	lea      -1(%edi),%edi
	movb     %dl,(%edi)
#;;   dbf       d2,ReadNormalByte      # [38]
	decw     d2_w
	cmpw     $-1,d2_w
	jne      ReadNormalByte
#;;   cmp.l     a1,a2                  # [39 CS]
	cmpl     %edi,a2_l
#;;   bcs.s     CrunchedBytes          # [40]
	jc       CrunchedBytes
#;;   rts                              # [41]
	ret      
CrunchedBytes:
#;;   moveq     #2,d0                  # [43]
	movl     $2,%ebx
#;;   bsr.s     ReadD1                 # [44]
	call     ReadD1
#;;   moveq     #0,d0                  # [45]
	movl     $0,%ebx
#;;   move.b    (a5,d1.w),d0           # [46]
	movswl   %dx,%ecx
	addl     a5_l,%ecx
	movb     0(%ecx),%bl
#;;   move.l    d0,d4                  # [47]
	movl     %ebx,d4_l
#;;   move.w    d1,d2                  # [48]
	movw     %dx,d2_w
#;;   addq.w    #1,d2                  # [49]
	addw     $1,d2_w
#;;   cmp.w     #4,d2                  # [50 NE]
	cmpw     $4,d2_w
#;;   bne.s     ReadOffset             # [51]
	jne      ReadOffset
#;;   bsr.s     ReadBit                # [52]
	call     ReadBit
#;;   move.l    d4,d0                  # [53]
	movl     d4_l,%ebx
#;;   tst.b     d1                     # [54 NE]
	testb    %dl,%dl
#;;   bne.s     LongBlockOffset        # [55]
	jne      LongBlockOffset
#;;   moveq     #7,d0                  # [56]
	movl     $7,%ebx
LongBlockOffset:
#;;   bsr.s     ReadD1                 # [58]
	call     ReadD1
#;;   move.w    d1,d3                  # [59]
	movw     %dx,d3_w
Read3BitsRow:
#;;   moveq     #3,d0                  # [61]
	movl     $3,%ebx
#;;   bsr.s     ReadD1                 # [62]
	call     ReadD1
#;;   add.w     d1,d2                  # [63]
	addw     %dx,d2_w
#;;   cmp.w     #7,d1                  # [64 EQ]
	cmpw     $7,%dx
#;;   beq.s     Read3BitsRow           # [65]
	je       Read3BitsRow
#;;   bra.s     DecrunchBlock          # [66]
	jmp      DecrunchBlock
ReadOffset:
#;;   bsr.s     ReadD1                 # [68]
	call     ReadD1
#;;   move.w    d1,d3                  # [69]
	movw     %dx,d3_w
DecrunchBlock:
#;;   move.b    (a1,d3.w),d0           # [71]
	movswl   d3_w,%ecx
	movb     0(%edi,%ecx),%bl
#;;   move.b    d0,-(a1)               # [72]
	lea      -1(%edi),%edi
	movb     %bl,(%edi)
#;;   dbf       d2,DecrunchBlock       # [73]
	decw     d2_w
	cmpw     $-1,d2_w
	jne      DecrunchBlock
EndOfLoop:
_pp_DecrunchColor:
#;;   move.w    a1,$dff1a2             # [76]
	movw     %di,0xdff1a2
#;;   cmp.l     a1,a2                  # [77 CS]
	cmpl     %edi,a2_l
#;;   bcs.s     LoopCheckCrunch        # [78]
	jc       LoopCheckCrunch
#;;   rts                              # [79]
	ret      
ReadBit:
#;;   moveq     #1,d0                  # [81]
	movl     $1,%ebx
ReadD1:
#;;   moveq     #0,d1                  # [83]
	movl     $0,%edx
#;;   subq.w    #1,d0                  # [84]
	subw     $1,%bx
ReadBits:
#;;   lsr.l     #1,d5                  # [86 X]
	shrl     $1,d5_l
#;;   roxl.l    #1,d1                  # [87]
	rcll     $1,%edx
#;;   subq.b    #1,d7                  # [88 NE]
	subb     $1,d7_b
#;;   bne.s     No32Read               # [89]
	jne      No32Read
#;;   move.b    #32,d7                 # [90]
	movb     $32,d7_b
#;;   move.l    -(a0),d5               # [91]
	lea      -4(%esi),%esi
	movl     (%esi),%eax
	movl     %eax,d5_l
No32Read:
#;;   dbf       d0,ReadBits            # [93]
	decw     %bx
	cmpw     $-1,%bx
	jne      ReadBits
#;;   rts                              # [94]
	ret      
	.data
	.p2align	2
myBitsTable:
#;;           dc.b $09,$0a,$0b,$0b     # [96]
	.byte	0x9,0xa,0xb,0xb
	.text
	.p2align	2
_pp_CalcCheckSum:
#;;   move.l    4(a7),a0               # [99]
	movl     4(%esp),%esi
#;;   moveq     #0,d0                  # [100]
	movl     $0,%ebx
#;;   moveq     #0,d1                  # [101]
	movl     $0,%edx
sumloop:
#;;   move.b    (a0)+,d1               # [103 EQ]
	movb     (%esi),%dl
	testb    %dl,%dl
	lea      1(%esi),%esi
#;;   beq.s     exitasm                # [104]
	je       exitasm
#;;   ror.w     d1,d0                  # [105]
	movw     %dx,%cx
	rorw     %cl,%bx
#;;   add.w     d1,d0                  # [106]
	addw     %dx,%bx
#;;   bra.s     sumloop                # [107]
	jmp      sumloop
_pp_CalcPasskey:
#;;   move.l    4(a7),a0               # [109]
	movl     4(%esp),%esi
#;;   moveq     #0,d0                  # [110]
	movl     $0,%ebx
#;;   moveq     #0,d1                  # [111]
	movl     $0,%edx
keyloop:
#;;   move.b    (a0)+,d1               # [113 EQ]
	movb     (%esi),%dl
	testb    %dl,%dl
	lea      1(%esi),%esi
#;;   beq.s     exitasm                # [114]
	je       exitasm
#;;   rol.l     #1,d0                  # [115]
	roll     $1,%ebx
#;;   add.l     d1,d0                  # [116]
	addl     %edx,%ebx
#;;   swap      d0                     # [117]
	roll     $16,%ebx
#;;   bra.s     keyloop                # [118]
	jmp      keyloop
exitasm:
#;;   rts                              # [120]
	ret      
_pp_Decrypt:
#;;   move.l    4(a7),a0               # [122]
	movl     4(%esp),%esi
#;;   move.l    8(a7),d1               # [123]
	movl     8(%esp),%edx
#;;   move.l    12(a7),d0              # [124]
	movl     12(%esp),%ebx
#;;   move.l    d2,-(a7)               # [125]
	pushl    d2_l
#;;   addq.l    #3,d1                  # [126]
	addl     $3,%edx
#;;   lsr.l     #2,d1                  # [127]
	shrl     $2,%edx
#;;   subq.l    #1,d1                  # [128]
	subl     $1,%edx
encryptloop:
#;;   move.l    (a0),d2                # [130]
	movl     (%esi),%eax
	movl     %eax,d2_l
#;;   eor.l     d0,d2                  # [131]
	xorl     %ebx,d2_l
#;;   move.l    d2,(a0)+               # [132]
	movl     d2_l,%eax
	movl     %eax,(%esi)
	lea      4(%esi),%esi
#;;   dbf       d1,encryptloop         # [133]
	decw     %dx
	cmpw     $-1,%dx
	jne      encryptloop
#;;   move.l    (a7)+,d2               # [134]
	popl     d2_l
#;;   rts                              # [135]
	ret      
