#**-------------------------------------------------------------------
#** This is the pure Decrunch-Routine
#** The Registers have to be loaded with the following values:
#** a1: Adr of Destination (normal)	** a2: Adr of Source (packed)
#** d1: Len of Destination		** d2: Len of Source
#**-------------------------------------------------------------------
    .text
    .globl crunchmania_17b
    .globl _crunchmania_17b
crunchmania_17b:
_crunchmania_17b:
    movl 4(%esp),%edi   # a1

    movl 8(%esp),%edx   # d1
    
    movl 12(%esp),%eax
    movl %eax,a2_l
    
    movl 16(%esp),%eax
    movl %eax,d2_l


#;;   move.l    a1,a5                  # [9]
	movl     %edi,a5_l
#;;   add.l     d1,a1                  # [10]
	lea      (%edi,%edx),%edi
#;;   add.l     d2,a2                  # [11]
	movl     d2_l,%eax
	addl     %eax,a2_l
#;;   move.w    -(a2),d0               # [12]
	subl     $6,a2_l
	movl     a2_l,%ecx
	movw     4(%ecx),%bx
#;;   move.l    -(a2),d6               # [13]
	movl     0(%ecx),%eax
	movl     %eax,d6_l
#;;   moveq     #16,d7                 # [14]
	movl     $16,d7_l
#;;   sub.w     d0,d7                  # [15]
	subw     %bx,d7_w
#;;   lsr.l     d7,d6                  # [16]
	movb     d7_b,%al
	movl     %eax,%ecx
	shrl     %cl,d6_l
#;;   move.w    d0,d7                  # [17]
	movw     %bx,d7_w
#;;   moveq     #16,d3                 # [18]
	movl     $16,d3_l
#;;   moveq     #0,d4                  # [19]
	movl     $0,d4_l
.DecrLoop:
#;;   cmp.l     a5,a1                  # [21 LE]
	cmpl     a5_l,%edi
#;;   ble.l     .DecrEnd               # [22]
	jle      .DecrEnd
#;;   bsr.s     .BitTest               # [24 (CC)]
	call     .BitTest
#;;   bcc.s     .InsertSeq             # [25]
	jnc      .InsertSeq
#;;   moveq     #0,d4                  # [26]
	movl     $0,d4_l
.InsertBytes:
#;;   moveq     #8,d1                  # [29]
	movl     $8,%edx
#;;   bsr.w     .GetBits               # [30]
	call     .GetBits
#;;   move.b    d0,-(a1)               # [31]
	lea      -1(%edi),%edi
	movb     %bl,(%edi)
#;;   dbf       d4,.InsertBytes        # [32]
	decw     d4_w
	cmpw     $-1,d4_w
	jne      .InsertBytes
#;;   bra.s     .DecrLoop              # [33]
	jmp      .DecrLoop
.SpecialInsert:
#;;   moveq     #14,d4                 # [36]
	movl     $14,d4_l
#;;   moveq     #5,d1                  # [37]
	movl     $5,%edx
#;;   bsr.s     .BitTest               # [38 (CS)]
	call     .BitTest
#;;   bcs.s     .IB1                   # [39]
	jc       .IB1
#;;   moveq     #14,d1                 # [40]
	movl     $14,%edx
.IB1:
#;;   bsr.s     .GetBits               # [41]
	call     .GetBits
#;;   add.w     d0,d4                  # [42]
	addw     %bx,d4_w
#;;   bra.s     .InsertBytes           # [43]
	jmp      .InsertBytes
.InsertSeq:
#;;   bsr.s     .BitTest               # [47 (CS)]
	call     .BitTest
#;;   bcs.s     .AB1                   # [48]
	jc       .AB1
#;;   moveq     #1,d1                  # [49]
	movl     $1,%edx
#;;   moveq     #1,d4                  # [50]
	movl     $1,d4_l
#;;   bra.s     .ABGet                 # [51]
	jmp      .ABGet
.AB1:
#;;   bsr.s     .BitTest               # [53 (CS)]
	call     .BitTest
#;;   bcs.s     .AB2                   # [54]
	jc       .AB2
#;;   moveq     #2,d1                  # [55]
	movl     $2,%edx
#;;   moveq     #3,d4                  # [56]
	movl     $3,d4_l
#;;   bra.s     .ABGet                 # [57]
	jmp      .ABGet
.AB2:
#;;   bsr.s     .BitTest               # [59 (CS)]
	call     .BitTest
#;;   bcs.s     .AB3                   # [60]
	jc       .AB3
#;;   moveq     #4,d1                  # [61]
	movl     $4,%edx
#;;   moveq     #7,d4                  # [62]
	movl     $7,d4_l
#;;   bra.s     .ABGet                 # [63]
	jmp      .ABGet
.AB3:
#;;   moveq     #8,d1                  # [65]
	movl     $8,%edx
#;;   moveq     #$17,d4                # [66]
	movl     $0x17,d4_l
.ABGet:
#;;   bsr.s     .GetBits               # [68]
	call     .GetBits
#;;   add.w     d0,d4                  # [69]
	addw     %bx,d4_w
#;;   cmp.w     #22,d4                 # [70 EQ LT]
	cmpw     $22,d4_w
#;;   beq.s     .SpecialInsert         # [71]
	je       .SpecialInsert
#;;   blt.s     .Cont                  # [72]
	jl       .Cont
#;;   subq.w    #1,d4                  # [73]
	subw     $1,d4_w
.Cont:
#;;   bsr.s     .BitTest               # [76 (CS)]
	call     .BitTest
#;;   bcs.s     .DB1                   # [77]
	jc       .DB1
#;;   moveq     #9,d1                  # [78]
	movl     $9,%edx
#;;   moveq     #$20,d2                # [79]
	movl     $0x20,d2_l
#;;   bra.s     .DBGet                 # [80]
	jmp      .DBGet
.DB1:
#;;   bsr.s     .BitTest               # [82 (CS)]
	call     .BitTest
#;;   bcs.s     .DB2                   # [83]
	jc       .DB2
#;;   moveq     #5,d1                  # [84]
	movl     $5,%edx
#;;   moveq     #0,d2                  # [85]
	movl     $0,d2_l
#;;   bra.s     .DBGet                 # [86]
	jmp      .DBGet
.DB2:
#;;   moveq     #14,d1                 # [88]
	movl     $14,%edx
#;;   move.w    #$220,d2               # [89]
	movw     $0x220,d2_w
.DBGet:
#;;   bsr.s     .GetBits               # [91]
	call     .GetBits
#;;   add.w     d2,d0                  # [92]
	addw     d2_w,%bx
#;;   lea       0(a1,d0.w),a3          # [93]
	movswl   %bx,%ecx
	lea      0(%edi,%ecx),%eax
	movl     %eax,a3_l
.InsSeqLoop:
#;;   move.b    -(a3),-(a1)            # [95]
	subl     $1,a3_l
	lea      -1(%edi),%edi
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movb     %al,(%edi)
#;;   dbf       d4,.InsSeqLoop         # [96]
	decw     d4_w
	cmpw     $-1,d4_w
	jne      .InsSeqLoop
#;;   bra.w     .DecrLoop              # [98]
	jmp      .DecrLoop
.BitTest:
#;;   subq.w    #1,d7                  # [101 NE]
	subw     $1,d7_w
#;;   bne.s     .BTNoLoop              # [102]
	jne      .BTNoLoop
#;;   moveq     #16,d7                 # [103]
	movl     $16,d7_l
#;;   move.w    d6,d0                  # [104]
	movw     d6_w,%bx
#;;   lsr.l     #1,d6                  # [105]
	shrl     $1,d6_l
#;;   swap      d6                     # [106]
	roll     $16,d6_l
#;;   move.w    -(a2),d6               # [107]
	subl     $2,a2_l
	movl     a2_l,%ecx
	movw     (%ecx),%ax
	movw     %ax,d6_w
#;;   swap      d6                     # [108]
	roll     $16,d6_l
#;;   lsr.w     #1,d0                  # [109 CC CS]
	shrw     $1,%bx
#;;   rts                              # [110]
	ret      
.BTNoLoop:
#;;   lsr.l     #1,d6                  # [112 CC CS]
	shrl     $1,d6_l
#;;   rts                              # [113]
	ret      
.GetBits:
#;;   move.w    d6,d0                  # [116]
	movw     d6_w,%bx
#;;   lsr.l     d1,d6                  # [117]
	movl     %edx,%ecx
	shrl     %cl,d6_l
#;;   sub.w     d1,d7                  # [118 GT]
	subw     %dx,d7_w
#;;   bgt.s     .GBNoLoop              # [119]
	jg       .GBNoLoop
#;;   add.w     d3,d7                  # [121]
	movw     d3_w,%ax
	addw     %ax,d7_w
#;;   ror.l     d7,d6                  # [122]
	movb     d7_b,%al
	movl     %eax,%ecx
	rorl     %cl,d6_l
#;;   move.w    -(a2),d6               # [123]
	subl     $2,a2_l
	movl     a2_l,%ecx
	movw     (%ecx),%ax
	movw     %ax,d6_w
#;;   rol.l     d7,d6                  # [124]
	movb     d7_b,%al
	movl     %eax,%ecx
	roll     %cl,d6_l
.GBNoLoop:
#;;   add.w     d1,d1                  # [126]
	addw     %dx,%dx
#;;   and.w     .AndData-2(pc,d1.w),d0 # [127]
	movswl   %dx,%ecx
	andw     .AndData-2(%ecx),%bx
#;;   rts                              # [128]
	ret      
	.data
	.p2align	2
.AndData:
#;;           dc.w    %1,%11,%111,%1111,%11111,%111111,%1111111 # [131]
	.short	0x1,0x3,0x7,0xf,0x1f,0x3f,0x7f
#;;           dc.w    %11111111,%111111111,%1111111111 # [132]
	.short	0xff,0x1ff,0x3ff
#;;           dc.w    %11111111111,%111111111111 # [133]
	.short	0x7ff,0xfff
#;;           dc.w    %1111111111111,%11111111111111 # [134]
	.short	0x1fff,0x3fff
	.text
	.p2align	2
.DecrEnd:
#;;   rts                              # [137]
	ret      
