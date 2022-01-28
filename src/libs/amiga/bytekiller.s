    .text
    .globl BYTUNP
    .globl _BYTUNP
BYTUNP:
_BYTUNP:
#;------------------------------------------------------------------------------
#; ByteKiller Unpacker Routine
#;
#; IN :	D0 = Crunched Size
#;	D4 = Checksum
#;	D5 = Check Byte 1
#;	D6 = Check Byte 2
#;	D7 = Check Byte 3
#;	A0 = Start Of Crunched Data
#;	A1 = Start Of Decrunched Data
#;	A2 = Decrunched Size
#;
#; OUT:	D0 = Success (0=Ok)
#;
movl     4(%esp), %ebx #d0_l
movl     8(%esp), %eax
movl        %eax, d4_l
movl    12(%esp), %eax
movl        %eax, d5_w
movl    16(%esp), %eax
movl        %eax, d6_w
movl    20(%esp), %eax
movl        %eax, d7_w
movl    24(%esp), %eax
movl        %eax, %esi #a0_l
movl    28(%esp), %eax
movl        %eax, %edi #a1_l
movl    32(%esp), %eax
movl        %eax, a2_l

    
#;;   add.l     a1,a2                  # [20]
	addl     %edi,a2_l
#;;   add.l     d0,a0                  # [21]
	lea      (%esi,%ebx),%esi
BYTUNP1:
#;;   move.l    -(a0),d0               # [23]
	lea      -4(%esi),%esi
	movl     (%esi),%ebx
#;;   eor.l     d0,d4                  # [24]
	xorl     %ebx,d4_l
BYTUNP2:
#;;   lsr.l     #1,d0                  # [25 CS NE]
	shrl     $1,%ebx
#;;   bne.b     bytunp3                # [26]
	jne      BYTUNP3
#;;   bsr.b     bytun15                # [27 (CS)]
	call     BYTUN15
BYTUNP3:
#;;   bcs.b     bytun13                # [28]
	jc       BYTUN13
#;;   move.w    d5,d1                  # [30]
	movw     d5_w,%dx
#;;   moveq     #1,d3                  # [31]
	movl     $1,d3_l
#;;   lsr.l     #1,d0                  # [32 CS NE]
	shrl     $1,%ebx
#;;   bne.b     bytunp4                # [33]
	jne      BYTUNP4
#;;   bsr.b     bytun15                # [34 (CS)]
	call     BYTUN15
BYTUNP4:
#;;   bcs.b     bytunp9                # [35]
	jc       BYTUNP9
#;;   moveq     #3,d1                  # [37]
	movl     $3,%edx
#;;   bsr.b     bytun16                # [38]
	call     BYTUN16
#;;   move.w    d2,d3                  # [39]
	movw     d2_w,%ax
	movw     %ax,d3_w
BYTUNP5:
#;;   moveq     #7,d1                  # [40]
	movl     $7,%edx
BYTUNP6:
#;;   lsr.l     #1,d0                  # [41 NE X]
	shrl     $1,%ebx
	setcb    xflag
#;;   bne.b     bytunp7                # [42]
	jne      BYTUNP7
#;;   bsr.b     bytun15                # [43 (X)]
	call     BYTUN15
	setcb    xflag
BYTUNP7:
#;;   roxl.l    #1,d2                  # [44]
	btw      $0,xflag
	rcll     $1,d2_l
#;;   dbra      d1,bytunp6             # [45]
	decw     %dx
	cmpw     $-1,%dx
	jne      BYTUNP6
#;;   move.b    d2,-(a2)               # [46]
	movb     d2_b,%al
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     %al,(%ecx)
#;;   cmp.l     a1,a2                  # [47 EQ]
	cmpl     %edi,a2_l
#;;   dbeq      d3,bytunp5             # [48]
	je       _PA_25_
	decw     d3_w
	cmpw     $-1,d3_w
	jne      BYTUNP5
_PA_25_:         
#;;   bra.b     bytun11                # [49]
	jmp      BYTUN11
BYTUNP8:
#;;   move.w    d7,d1                  # [51]
	movw     d7_w,%dx
#;;   add.w     d2,d1                  # [52]
	addw     d2_w,%dx
#;;   addq.w    #2,d2                  # [53]
	addw     $2,d2_w
#;;   move.w    d2,d3                  # [54]
	movw     d2_w,%ax
	movw     %ax,d3_w
BYTUNP9:
#;;   bsr.b     bytun16                # [55]
	call     BYTUN16
BYTUN10:
#;;   move.b    -1(a2,d2.w),-(a2)      # [56]
	subl     $1,a2_l
	movswl   d2_w,%ecx
	addl     a2_l,%ecx
	movb     -1(%ecx),%al
	movl     a2_l,%ecx
	movb     %al,(%ecx)
#;;   cmp.l     a1,a2                  # [57 EQ]
	cmpl     %edi,a2_l
#;;   dbeq      d3,bytun10             # [58]
	je       _PA_34_
	decw     d3_w
	cmpw     $-1,d3_w
	jne      BYTUN10
_PA_34_:         
BYTUN11:
#;;   cmp.w     #-1,d3                 # [59 EQ]
	cmpw     $-1,d3_w
#;;   beq.b     bytunp2                # [60]
	je       BYTUNP2
#;;   move.l    d4,d0                  # [61]
	movl     d4_l,%ebx
#;;   rts                              # [62]
    movl %ebx,%eax
	ret      
BYTUN13:
#;;   moveq     #2,d1                  # [64]
	movl     $2,%edx
#;;   bsr.b     bytun16                # [65]
	call     BYTUN16
#;;   cmp.b     #2,d2                  # [66 LT]
	cmpb     $2,d2_b
#;;   blt.b     bytunp8                # [67]
	jl       BYTUNP8
#;;   cmp.b     #3,d2                  # [68 NE]
	cmpb     $3,d2_b
#;;   bne.b     bytun14                # [69]
	jne      BYTUN14
#;;   moveq     #8,d1                  # [70]
	movl     $8,%edx
#;;   bsr.b     bytun16                # [71]
	call     BYTUN16
#;;   move.w    d2,d3                  # [72]
	movw     d2_w,%ax
	movw     %ax,d3_w
#;;   addq.w    #8,d3                  # [73]
	addw     $8,d3_w
#;;   bra.b     bytunp5                # [74]
	jmp      BYTUNP5
BYTUN14:
#;;   moveq     #8,d1                  # [76]
	movl     $8,%edx
#;;   bsr.b     bytun16                # [77]
	call     BYTUN16
#;;   move.w    d2,d3                  # [78]
	movw     d2_w,%ax
	movw     %ax,d3_w
#;;   move.w    d6,d1                  # [79]
	movw     d6_w,%dx
#;;   bra.b     bytunp9                # [80]
	jmp      BYTUNP9
BYTUN15:
#;;   move.l    -(a0),d0               # [82]
	lea      -4(%esi),%esi
	movl     (%esi),%ebx
#;;   eor.l     d0,d4                  # [83]
	xorl     %ebx,d4_l
#;;   move.w    #$10,ccr               # [84 X]
	movb     $((0x10)>>4)&1,xflag
#;;   roxr.l    #1,d0                  # [85 CS X]
	btw      $0,xflag
	rcrl     $1,%ebx
	setcb    xflag
#;;   rts                              # [86]
	ret     
BYTUN16:
#;;   subq.w    #1,d1                  # [88]
	subw     $1,%dx
#;;   moveq     #0,d2                  # [89]
	movl     $0,d2_l
BYTUN17:
#;;   lsr.l     #1,d0                  # [90 NE X]
	shrl     $1,%ebx
	setcb    xflag
#;;   bne.b     bytun18                # [91]
	jne      BYTUN18
#;;   bsr.b     bytun15                # [92 (X)]
	call     BYTUN15
	setcb    xflag
BYTUN18:
#;;   roxl.l    #1,d2                  # [93]
	btw      $0,xflag
	rcll     $1,d2_l
#;;   dbra      d1,bytun17             # [94]
	decw     %dx
	cmpw     $-1,%dx
	jne      BYTUN17
#;;   rts                              # [95]
	ret     
