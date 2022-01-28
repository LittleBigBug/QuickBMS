#**-------------------------------------------------------------------
#** This is the pure Decrunch-Routine
#** The Registers have to be loaded with the following values:
#** a1: Adr of Destination (normal)	** a2: Adr of Source (packed)
#** d1: Len of Destination		** d2: Len of Source
#**-------------------------------------------------------------------
    .text
    .globl crunchmania_FastDecruncher
    .globl _crunchmania_FastDecruncher
crunchmania_FastDecruncher:
_crunchmania_FastDecruncher:
    movl 4(%esp),%edi   # a1

    movl 8(%esp),%edx   # d1
    
    movl 12(%esp),%eax
    movl %eax, a2_l

    movl 16(%esp),%eax
    movl %eax, d2_l




#;;   move.l    a1,a5                  # [12]
	movl     %edi,a5_l
#;;   add.l     d1,a1                  # [13]
	lea      (%edi,%edx),%edi
#;;   add.l     d2,a2                  # [14]
	movl     d2_l,%eax
	addl     %eax,a2_l
#;;   move.w    -(a2),d0               # [15]
	subl     $6,a2_l
	movl     a2_l,%ecx
	movw     4(%ecx),%bx
#;;   move.l    -(a2),d6               # [16]
	movl     0(%ecx),%eax
	movl     %eax,d6_l
#;;   moveq     #16,d7                 # [17]
	movl     $16,d7_l
#;;   sub.w     d0,d7                  # [18]
	subw     %bx,d7_w
#;;   lsr.l     d7,d6                  # [19]
	movb     d7_b,%al
	movl     %eax,%ecx
	shrl     %cl,d6_l
#;;   move.w    d0,d7                  # [20]
	movw     %bx,d7_w
#;;   moveq     #16,d3                 # [21]
	movl     $16,d3_l
#;;   moveq     #0,d4                  # [22]
	movl     $0,d4_l
.DecrLoopx:
#;;   bsr.w     .BitTestx               # [24 (CC)]
	call     .BitTestx
#;;   bcc.s     .InsertSeqx             # [25]
	jnc      .InsertSeqx
#;;   moveq     #0,d4                  # [26]
	movl     $0,d4_l
.InsertBytesx:
#;;   moveq     #8,d1                  # [29]
	movl     $8,%edx
#;;   bsr.w     .GetBitsx               # [30]
	call     .GetBitsx
#;;   move.b    d0,-(a1)               # [31]
	lea      -1(%edi),%edi
	movb     %bl,(%edi)
#;;   cmp.l     a5,a1                  # [32 EQ]
	cmpl     a5_l,%edi
#;;   dbeq      d4,.InsertBytesx        # [33]
	je       _PA_21_
	decw     d4_w
	cmpw     $-1,d4_w
	jne      .InsertBytesx
_PA_21_:         
.More:
#;;   cmp.w     #-1,d4                 # [34 EQ]
	cmpw     $-1,d4_w
#;;   beq.b     .DecrLoopx              # [35]
	je       .DecrLoopx
#;;   rts                              # [36]
	ret      
.SpecialInsertx:
#;;   moveq     #14,d4                 # [39]
	movl     $14,d4_l
#;;   moveq     #5,d1                  # [40]
	movl     $5,%edx
#;;   bsr.s     .BitTestx               # [41 (CS)]
	call     .BitTestx
#;;   bcs.s     .IB1x                   # [42]
	jc       .IB1x
#;;   moveq     #14,d1                 # [43]
	movl     $14,%edx
.IB1x:
#;;   bsr.s     .GetBitsx               # [44]
	call     .GetBitsx
#;;   add.w     d0,d4                  # [45]
	addw     %bx,d4_w
#;;   bra.s     .InsertBytesx           # [46]
	jmp      .InsertBytesx
.InsertSeqx:
#;;   bsr.s     .BitTestx               # [50 (CS)]
	call     .BitTestx
#;;   bcs.s     .AB1x                   # [51]
	jc       .AB1x
#;;   moveq     #1,d1                  # [52]
	movl     $1,%edx
#;;   moveq     #1,d4                  # [53]
	movl     $1,d4_l
#;;   bra.s     .ABGetx                 # [54]
	jmp      .ABGetx
.AB1x:
#;;   bsr.s     .BitTestx               # [56 (CS)]
	call     .BitTestx
#;;   bcs.s     .AB2x                   # [57]
	jc       .AB2x
#;;   moveq     #2,d1                  # [58]
	movl     $2,%edx
#;;   moveq     #3,d4                  # [59]
	movl     $3,d4_l
#;;   bra.s     .ABGetx                 # [60]
	jmp      .ABGetx
.AB2x:
#;;   bsr.s     .BitTestx               # [62 (CS)]
	call     .BitTestx
#;;   bcs.s     .AB3x                   # [63]
	jc       .AB3x
#;;   moveq     #4,d1                  # [64]
	movl     $4,%edx
#;;   moveq     #7,d4                  # [65]
	movl     $7,d4_l
#;;   bra.s     .ABGetx                 # [66]
	jmp      .ABGetx
.AB3x:
#;;   moveq     #8,d1                  # [68]
	movl     $8,%edx
#;;   moveq     #$17,d4                # [69]
	movl     $0x17,d4_l
.ABGetx:
#;;   bsr.s     .GetBitsx               # [71]
	call     .GetBitsx
#;;   add.w     d0,d4                  # [72]
	addw     %bx,d4_w
#;;   cmp.w     #22,d4                 # [73 EQ LT]
	cmpw     $22,d4_w
#;;   beq.s     .SpecialInsertx         # [74]
	je       .SpecialInsertx
#;;   blt.s     .Contx                  # [75]
	jl       .Contx
#;;   subq.w    #1,d4                  # [76]
	subw     $1,d4_w
.Contx:
#;;   bsr.s     .BitTestx               # [79 (CS)]
	call     .BitTestx
#;;   bcs.s     .DB1x                   # [80]
	jc       .DB1x
#;;   moveq     #9,d1                  # [81]
	movl     $9,%edx
#;;   moveq     #$20,d2                # [82]
	movl     $0x20,d2_l
#;;   bra.s     .DBGetx                 # [83]
	jmp      .DBGetx
.DB1x:
#;;   bsr.s     .BitTestx               # [85 (CS)]
	call     .BitTestx
#;;   bcs.s     .DB2x                   # [86]
	jc       .DB2x
#;;   moveq     #5,d1                  # [87]
	movl     $5,%edx
#;;   moveq     #0,d2                  # [88]
	movl     $0,d2_l
#;;   bra.s     .DBGetx                 # [89]
	jmp      .DBGetx
.DB2x:
#;;   moveq     #14,d1                 # [91]
	movl     $14,%edx
#;;   move.w    #$220,d2               # [92]
	movw     $0x220,d2_w
.DBGetx:
#;;   bsr.s     .GetBitsx               # [94]
	call     .GetBitsx
#;;   add.w     d2,d0                  # [95]
	addw     d2_w,%bx
#;;   lea       0(a1,d0.w),a3          # [96]
	movswl   %bx,%ecx
	lea      0(%edi,%ecx),%eax
	movl     %eax,a3_l
.InsSeqLoopx:
#;;   move.b    -(a3),-(a1)            # [98]
	subl     $1,a3_l
	lea      -1(%edi),%edi
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movb     %al,(%edi)
#;;   cmp.l     a5,a1                  # [99 EQ]
	cmpl     a5_l,%edi
#;;   dbeq      d4,.InsSeqLoopx         # [100]
	je       _PA_84_
	decw     d4_w
	cmpw     $-1,d4_w
	jne      .InsSeqLoopx
_PA_84_:         
#;;   bra.b     .More                  # [101]
	jmp      .More
.BitTestx:
#;;   subq.w    #1,d7                  # [104 NE]
	subw     $1,d7_w
#;;   bne.s     .BTNoLoopx              # [105]
	jne      .BTNoLoopx
#;;   moveq     #16,d7                 # [106]
	movl     $16,d7_l
#;;   move.w    d6,d0                  # [107]
	movw     d6_w,%bx
#;;   lsr.l     #1,d6                  # [108]
	shrl     $1,d6_l
#;;   swap      d6                     # [109]
	roll     $16,d6_l
#;;   move.w    -(a2),d6               # [110]
	subl     $2,a2_l
	movl     a2_l,%ecx
	movw     (%ecx),%ax
	movw     %ax,d6_w
#;;   swap      d6                     # [111]
	roll     $16,d6_l
#;;   lsr.w     #1,d0                  # [112 CC CS]
	shrw     $1,%bx
#;;   rts                              # [113]
	ret      
.BTNoLoopx:
#;;   lsr.l     #1,d6                  # [115 CC CS]
	shrl     $1,d6_l
#;;   rts                              # [116]
	ret      
.GetBitsx:
#;;   move.w    d6,d0                  # [119]
	movw     d6_w,%bx
#;;   lsr.l     d1,d6                  # [120]
	movl     %edx,%ecx
	shrl     %cl,d6_l
#;;   sub.w     d1,d7                  # [121 GT]
	subw     %dx,d7_w
#;;   bgt.s     .GBNoLoopx              # [122]
	jg       .GBNoLoopx
#;;   add.w     d3,d7                  # [124]
	movw     d3_w,%ax
	addw     %ax,d7_w
#;;   ror.l     d7,d6                  # [125]
	movb     d7_b,%al
	movl     %eax,%ecx
	rorl     %cl,d6_l
#;;   move.w    -(a2),d6               # [126]
	subl     $2,a2_l
	movl     a2_l,%ecx
	movw     (%ecx),%ax
	movw     %ax,d6_w
#;;   rol.l     d7,d6                  # [127]
	movb     d7_b,%al
	movl     %eax,%ecx
	roll     %cl,d6_l
.GBNoLoopx:
#;;   add.w     d1,d1                  # [129]
	addw     %dx,%dx
#;;   and.w     .AndDatax-2(pc,d1.w),d0 # [130]
	movswl   %dx,%ecx
	andw     .AndDatax-2(%ecx),%bx
.DecrEndx:
#;;   rts                              # [132]
	ret      
	.data
	.p2align	2
.AndDatax:
#;;           dc.w    %1,%11,%111,%1111,%11111,%111111,%1111111 # [135]
	.short	0x1,0x3,0x7,0xf,0x1f,0x3f,0x7f
#;;           dc.w    %11111111,%111111111,%1111111111 # [136]
	.short	0xff,0x1ff,0x3ff
#;;           dc.w    %11111111111,%111111111111 # [137]
	.short	0x7ff,0xfff
#;;           dc.w    %1111111111111,%11111111111111 # [138]
	.short	0x1fff,0x3fff
	.text
	.p2align	2
    
    
    

#;------------------------------------------------------------------------------
#; Crunch Mania Decruncher Huffman
#;
#; IN :	A1 = Adr of Destination (normal)
#;	A2 = Adr of Source (packed)
#;	D1 = Len of Destination
#;	D2 = Len of Source
#;
#; OUT:	D0 = Success (0=Error)
#;
    
    .globl crunchmania_FastDecruncherHuff
    .globl _crunchmania_FastDecruncherHuff
crunchmania_FastDecruncherHuff:
_crunchmania_FastDecruncherHuff:
    movl 4(%esp),%edi   # a1

    movl 8(%esp),%edx   # d1
    
    movl 12(%esp),%eax
    movl %eax, a2_l

    movl 16(%esp),%eax
    movl %eax, d2_l
    
    movl %edx,UI_DecrunchLen
    movl %edi,UI_DecrunchAdr

    

#;;   move.l    a4,-(sp)               # [152]
	pushl    a4_l
#;;   move.l    #$4e0,d0               # [153]
	movl     $0x4e0,%ebx
#;;   bsr.w     alcmem                 # [154]
    pushl %ebx
	call     _malloc
#;;   move.l    d0,UI_Temp(a4)         # [155 EQ]
	movl     a4_l,%ecx
	movl     %ebx,UI_Temp
	testl    %ebx,%ebx
#;;   beq.w     fdehu6a                # [156]
	je       FDEHU6a
#;;   move.l    d0,a6                  # [158]
	movl     %ebx,%ebp
#;;   addq.l    #2,a6                  # [159]
	lea      2(%ebp),%ebp
#;;   move.l    a1,d3                  # [161]
	movl     %edi,d3_l
#;;   add.l     d1,a1                  # [162]
	lea      (%edi,%edx),%edi
#;;   add.l     d2,a2                  # [163]
	movl     d2_l,%eax
	addl     %eax,a2_l
#;;   move.w    -(a2),d0               # [164]
	subl     $6,a2_l
	movl     a2_l,%ecx
	movw     4(%ecx),%bx
#;;   move.l    -(a2),d6               # [165]
	movl     0(%ecx),%eax
	movl     %eax,d6_l
#;;   moveq     #16,d7                 # [166]
	movl     $16,d7_l
#;;   sub.w     d0,d7                  # [167]
	subw     %bx,d7_w
#;;   lsr.l     d7,d6                  # [168]
	movb     d7_b,%al
	movl     %eax,%ecx
	shrl     %cl,d6_l
#;;   move.w    d0,d7                  # [169]
	movw     %bx,d7_w
FDEHU1:
#;;   lea       $49e(a6),a0            # [171]
	lea      0x49e(%ebp),%esi
#;;   moveq     #15,d2                 # [172]
	movl     $15,d2_l
FDEHU2:
#;;   clr.l     (a0)+                  # [173]
	movl     $0,(%esi)
	lea      4(%esi),%esi
#;;   dbra      d2,fdehu2              # [174]
	decw     d2_w
	cmpw     $-1,d2_w
	jne      FDEHU2
#;;   lea       $4be(a6),a0            # [176]
	lea      0x4be(%ebp),%esi
#;;   lea       $9e(a6),a4             # [177]
	lea      0x9e(%ebp),%eax
#;;   moveq     #9,d2                  # [178]
	movl     $9,d2_l
	movl     %eax,a4_l
#;;   bsr.w     fdehu14                # [179]
	call     FDEHU14
#;;   lea       $49e(a6),a0            # [180]
	lea      0x49e(%ebp),%esi
#;;   lea       $80(a6),a4             # [181]
	lea      0x80(%ebp),%eax
#;;   moveq     #4,d2                  # [182]
	movl     $4,d2_l
	movl     %eax,a4_l
#;;   bsr.w     fdehu14                # [183]
	call     FDEHU14
#;;   lea       $4be(a6),a3            # [184]
	lea      0x4be(%ebp),%eax
#;;   lea       -2(a6),a4              # [185]
	movl     %eax,a3_l
	lea      -2(%ebp),%eax
	movl     %eax,a4_l
#;;   bsr.w     fdehu18                # [186]
	call     FDEHU18
#;;   lea       $49e(a6),a3            # [187]
	lea      0x49e(%ebp),%eax
#;;   lea       $1e(a6),a4             # [188]
	movl     %eax,a3_l
	lea      0x1e(%ebp),%eax
	movl     %eax,a4_l
#;;   bsr.w     fdehu18                # [189]
	call     FDEHU18
#;;   moveq     #16,d1                 # [190]
	movl     $16,%edx
#;;   bsr.w     fdehu11                # [191]
	call     FDEHU11
#;;   move.w    d0,d5                  # [192]
	movw     %bx,d5_w
#;;   lea       $9e(a6),a0             # [193]
	lea      0x9e(%ebp),%esi
#;;   lea       -$1e(a0),a5            # [194]
	lea      -0x1e(%esi),%eax
	movl     %eax,a5_l
FDEHU3:
#;;   move.l    a6,a4                  # [196]
	movl     %ebp,a4_l
#;;   bsr.b     fdehu7                 # [197]
	call     FDEHU7
#;;   btst      #8,d0                  # [198 NE]
	testl    $1<<((8)&31),%ebx
#;;   bne.b     fdehu6                 # [199]
	jne      FDEHU6
#;;   move.w    d0,d4                  # [200]
	movw     %bx,d4_w
#;;   lea       $20(a6),a4             # [201]
	lea      0x20(%ebp),%eax
#;;   exg       a0,a5                  # [202]
	xchgl    %esi,a5_l
	movl     %eax,a4_l
#;;   bsr.b     fdehu7                 # [203]
	call     FDEHU7
#;;   exg       a0,a5                  # [204]
	xchgl    %esi,a5_l
#;;   move.w    d0,d1                  # [205]
	movw     %bx,%dx
#;;   move.w    d0,d2                  # [206 NE]
	movw     %bx,d2_w
	testw    %bx,%bx
#;;   bne.b     fdehu4                 # [207]
	jne      FDEHU4
#;;   moveq     #1,d1                  # [208]
	movl     $1,%edx
#;;   moveq     #16,d2                 # [209]
	movl     $16,d2_l
FDEHU4:
#;;   bsr.b     fdehu11                # [210]
	call     FDEHU11
#;;   bset      d2,d0                  # [211]
	movl     d2_l,%eax
	andl     $31,%eax
	btsl     %eax,%ebx
#;;   lea       1(a1,d0.w),a3          # [212]
	movswl   %bx,%ecx
	lea      1(%edi,%ecx),%eax
	movl     %eax,a3_l
FDEHU5:
#;;   move.b    -(a3),-(a1)            # [213]
	subl     $1,a3_l
	lea      -1(%edi),%edi
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movb     %al,(%edi)
#;;   cmp.l     d3,a1                  # [214 EQ]
	cmpl     d3_l,%edi
#;;   dbeq      d4,fdehu5              # [215]
	je       _PA_178_
	decw     d4_w
	cmpw     $-1,d4_w
	jne      FDEHU5
_PA_178_:         
#;;   cmp.w     #-1,d4                 # [216 NE]
	cmpw     $-1,d4_w
#;;   bne.b     fdehu6a                # [217]
	jne      FDEHU6a
#;;   move.b    -(a3),-(a1)            # [218]
	subl     $1,a3_l
	lea      -1(%edi),%edi
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movb     %al,(%edi)
#;;   cmp.l     d3,a1                  # [219 EQ]
	cmpl     d3_l,%edi
#;;   beq.b     fdehu6a                # [220]
	je       FDEHU6a
#;;   move.b    -(a3),d0               # [221]
	subl     $1,a3_l
	movl     a3_l,%ecx
	movb     (%ecx),%bl
FDEHU6:
#;;   move.b    d0,-(a1)               # [222]
	lea      -1(%edi),%edi
	movb     %bl,(%edi)
#;;   cmp.l     d3,a1                  # [223 EQ]
	cmpl     d3_l,%edi
#;;   dbeq      d5,fdehu3              # [224]
	je       _PA_187_
	decw     d5_w
	cmpw     $-1,d5_w
	jne      FDEHU3
_PA_187_:         
#;;   cmp.w     #-1,d5                 # [225 NE]
	cmpw     $-1,d5_w
#;;   bne.b     fdehu6a                # [226]
	jne      FDEHU6a
#;;   moveq     #1,d1                  # [227]
	movl     $1,%edx
#;;   bsr.b     fdehu11                # [228 (NE)]
	call     FDEHU11
#;;   bne.w     fdehu1                 # [229]
	jne      FDEHU1
FDEHU6a:
#;;   move.l    (sp)+,a4               # [231]
	popl     a4_l
#;;   move.l    UI_Temp(a4),a1         # [232]
	movl     a4_l,%ecx
	movl     UI_Temp,%edi
#;;   bsr.w     fremem                 # [233]
    pushl %edi
	call     _free
#;;   clr.l     UI_Temp(a4)            # [234]
	movl     a4_l,%ecx
	movl     $0,UI_Temp
#;;   rts                              # [235]
	ret      
FDEHU7:
#;;   moveq     #0,d1                  # [237]
	movl     $0,%edx
FDEHU8:
#;;   subq.w    #1,d7                  # [238 EQ]
	subw     $1,d7_w
#;;   beq.b     fdehu9                 # [239]
	je       FDEHU9
#;;   lsr.l     #1,d6                  # [240 X]
	shrl     $1,d6_l
	setcb    xflag
#;;   bra.b     fdehu10                # [241]
	jmp      FDEHU10
FDEHU9:
#;;   moveq     #16,d7                 # [243]
	movl     $16,d7_l
#;;   move.w    d6,d0                  # [244]
	movw     d6_w,%bx
#;;   lsr.l     #1,d6                  # [245]
	shrl     $1,d6_l
#;;   swap      d6                     # [246]
	roll     $16,d6_l
#;;   move.w    -(a2),d6               # [247]
	subl     $2,a2_l
	movl     a2_l,%ecx
	movw     (%ecx),%ax
	movw     %ax,d6_w
#;;   swap      d6                     # [248]
	roll     $16,d6_l
#;;   lsr.w     #1,d0                  # [249 X]
	shrw     $1,%bx
	setcb    xflag
FDEHU10:
#;;   roxl.w    #1,d1                  # [250]
	btw      $0,xflag
	rclw     $1,%dx
#;;   move.w    (a4)+,d0               # [251]
	movl     a4_l,%ecx
	addl     $2,a4_l
	movw     (%ecx),%bx
#;;   cmp.w     d1,d0                  # [252 LS]
	cmpw     %dx,%bx
#;;   bls.b     fdehu8                 # [253]
	jbe      FDEHU8
#;;   add.w     $3e(a4),d1             # [254]
	movl     a4_l,%ecx
	addw     0x3e(%ecx),%dx
#;;   add.w     d1,d1                  # [255]
	addw     %dx,%dx
#;;   move.w    (a0,d1.w),d0           # [256]
	movswl   %dx,%ecx
	movw     0(%esi,%ecx),%bx
#;;   rts                              # [257]
	ret      
FDEHU11:
#;;   move.w    d6,d0                  # [259]
	movw     d6_w,%bx
#;;   lsr.l     d1,d6                  # [260]
	movl     %edx,%ecx
	shrl     %cl,d6_l
#;;   sub.w     d1,d7                  # [261 GT]
	subw     %dx,d7_w
#;;   bgt.b     fdehu12                # [262]
	jg       FDEHU12
#;;   add.w     #16,d7                 # [263]
	addw     $16,d7_w
#;;   ror.l     d7,d6                  # [264]
	movb     d7_b,%al
	movl     %eax,%ecx
	rorl     %cl,d6_l
#;;   move.w    -(a2),d6               # [265]
	subl     $2,a2_l
	movl     a2_l,%ecx
	movw     (%ecx),%ax
	movw     %ax,d6_w
#;;   rol.l     d7,d6                  # [266]
	movb     d7_b,%al
	movl     %eax,%ecx
	roll     %cl,d6_l
FDEHU12:
#;;   add.w     d1,d1                  # [267]
	addw     %dx,%dx
#;;   and.w     fdehu13-2(pc,d1.w),d0  # [268 NE]
	movswl   %dx,%ecx
	andw     FDEHU13-2(%ecx),%bx
#;;   rts                              # [269]
	ret      
	.data
	.p2align	2
#;;   FDEHU13 dc.w    $0001,$0003,$0007,$000f,$001f,$003f,$007f,$00ff # [271]
FDEHU13:
	.short	0x1,0x3,0x7,0xf,0x1f,0x3f,0x7f,0xff
#;;           dc.w    $01ff,$03ff,$07ff,$0fff,$1fff,$3fff,$7fff,$ffff # [272]
	.short	0x1ff,0x3ff,0x7ff,0xfff,0x1fff,0x3fff,0x7fff,0xffff
	.text
	.p2align	2
FDEHU14:
#;;   movem.l   d1-d5/a3,-(sp)         # [274]
	pushl    a3_l
	pushl    d5_l
	pushl    d4_l
	pushl    d3_l
	pushl    d2_l
	pushl    %edx
#;;   moveq     #4,d1                  # [275]
	movl     $4,%edx
#;;   bsr.b     fdehu11                # [276]
	call     FDEHU11
#;;   move.w    d0,d5                  # [277]
	movw     %bx,d5_w
#;;   subq.w    #1,d5                  # [278]
	subw     $1,d5_w
#;;   moveq     #0,d4                  # [279]
	movl     $0,d4_l
#;;   sub.l     a3,a3                  # [280]
	movl     a3_l,%eax
	subl     %eax,a3_l
FDEHU15:
#;;   addq.w    #1,d4                  # [281]
	addw     $1,d4_w
#;;   move.w    d4,d1                  # [282]
	movw     d4_w,%dx
#;;   cmp.w     d2,d1                  # [283 LE]
	cmpw     d2_w,%dx
#;;   ble.b     fdehu16                # [284]
	jle      FDEHU16
#;;   move.w    d2,d1                  # [285]
	movw     d2_w,%dx
FDEHU16:
#;;   bsr.b     fdehu11                # [286]
	call     FDEHU11
#;;   move.w    d0,(a0)+               # [287]
	movw     %bx,(%esi)
	lea      2(%esi),%esi
#;;   add.w     d0,a3                  # [288]
	movswl   %bx,%eax
	addl     %eax,a3_l
#;;   dbra      d5,fdehu15             # [289]
	decw     d5_w
	cmpw     $-1,d5_w
	jne      FDEHU15
#;;   move.w    a3,d5                  # [290]
	movw     a3_w,%ax
	movw     %ax,d5_w
#;;   subq.w    #1,d5                  # [291]
	subw     $1,d5_w
FDEHU17:
#;;   move.w    d2,d1                  # [292]
	movw     d2_w,%dx
#;;   bsr.b     fdehu11                # [293]
	call     FDEHU11
#;;   move.w    d0,(a4)+               # [294]
	movl     a4_l,%ecx
	addl     $2,a4_l
	movw     %bx,(%ecx)
#;;   dbra      d5,fdehu17             # [295]
	decw     d5_w
	cmpw     $-1,d5_w
	jne      FDEHU17
#;;   movem.l   (sp)+,d1-d5/a3         # [296]
	popl     %edx
	popl     d2_l
	popl     d3_l
	popl     d4_l
	popl     d5_l
	popl     a3_l
#;;   rts                              # [297]
	ret      
FDEHU18:
#;;   movem.l   d0-d7,-(sp)            # [299]
	pushl    d7_l
	pushl    d6_l
	pushl    d5_l
	pushl    d4_l
	pushl    d3_l
	pushl    d2_l
	pushl    %edx
	pushl    %ebx
#;;   clr.w     (a4)+                  # [300]
	movl     a4_l,%ecx
	addl     $2,a4_l
	movw     $0,(%ecx)
#;;   moveq     #14,d7                 # [301]
	movl     $14,d7_l
#;;   moveq     #-1,d4                 # [302]
	movl     $-1,d4_l
#;;   moveq     #0,d3                  # [303]
	movl     $0,d3_l
#;;   moveq     #0,d2                  # [304]
	movl     $0,d2_l
#;;   moveq     #1,d1                  # [305]
	movl     $1,%edx
FDEHU19:
#;;   move.w    (a3)+,d6               # [306]
	movl     a3_l,%ecx
	movw     (%ecx),%ax
	addl     $2,a3_l
	movw     %ax,d6_w
#;;   move.w    d3,$40(a4)             # [307]
	movw     d3_w,%ax
	movl     a4_l,%ecx
	movw     %ax,0x40(%ecx)
#;;   move.w    -2(a4),d0              # [308]
	movw     -2(%ecx),%bx
#;;   add.w     d0,d0                  # [309]
	addw     %bx,%bx
#;;   sub.w     d0,$40(a4)             # [310]
	subw     %bx,0x40(%ecx)
#;;   add.w     d6,d3                  # [311]
	movw     d6_w,%ax
	addw     %ax,d3_w
#;;   mulu      d1,d6                  # [312]
	movzwl   %ax,%eax
	movzwl   %dx,%ecx
	imull    %ecx,%eax
#;;   add.w     d6,d2                  # [313]
	addw     %ax,d2_w
#;;   move.w    d2,(a4)+               # [314]
	movw     d2_w,%ax
	movl     a4_l,%ecx
	addl     $2,a4_l
	movw     %ax,(%ecx)
#;;   lsl.w     #1,d2                  # [315]
	shlw     $1,d2_w
#;;   dbra      d7,fdehu19             # [316]
	decw     d7_w
	cmpw     $-1,d7_w
	jne      FDEHU19
#;;   movem.l   (sp)+,d0-d7            # [317]
	popl     %ebx
	popl     %edx
	popl     d2_l
	popl     d3_l
	popl     d4_l
	popl     d5_l
	popl     d6_l
	popl     d7_l
#;;   rts                              # [318]
	ret      
CRMSDEC:
#;;   jsr       (a6)                   # [327]
	call     *%ebp
#;;   move.l    UI_DecrunchLen(a4),d1  # [329]
	movl     a4_l,%ecx
	movl     UI_DecrunchLen,%edx
#;;   move.l    UI_DecrunchAdr(a4),a0  # [330]
	movl     UI_DecrunchAdr,%esi
#;;   move.b    (a0)+,d2               # [331]
	lodsb    
	movb     %al,d2_b
#;;   subq.l    #1,d1                  # [332]
	subl     $1,%edx
CRMSDE1:
#;;   add.b     (a0),d2                # [333]
	movb     (%esi),%al
	addb     %al,d2_b
#;;   move.b    d2,(a0)+               # [334]
	movb     d2_b,%al
	movb     %al,(%esi)
	lea      1(%esi),%esi
#;;   subq.l    #1,d1                  # [335 NE]
	subl     $1,%edx
#;;   bne.b     crmsde1                # [336]
	jne      CRMSDE1
#;;   moveq     #-1,d0                 # [338]
	movl     $-1,%ebx
#;;   rts                              # [339]
    movl %ebx,%eax
	ret      
CRMPASS:
#;;   move.w    -(a1),d2               # [348]
	lea      -2(%edi),%edi
	movw     (%edi),%ax
	movw     %ax,d2_w
#;;   and.w     #$fff0,d2              # [349]
	andw     $0xfff0,d2_w
#;;   lea       UI_PasswordStr(a4),a2  # [350]
	movl     a4_l,%ecx
	lea      UI_PasswordStr,%eax
#;;   moveq     #0,d0                  # [351]
	movl     $0,%ebx
#;;   moveq     #0,d1                  # [352]
	movl     $0,%edx
	movl     %eax,a2_l
CRMPAS1:
#;;   move.b    (a2)+,d0               # [353 EQ]
	movl     a2_l,%ecx
	addl     $1,a2_l
	movb     (%ecx),%bl
	testb    %bl,%bl
#;;   beq.b     crmpas2                # [354]
	je       CRMPAS2
#;;   add.w     d0,d1                  # [355]
	addw     %bx,%dx
#;;   add.w     d1,d1                  # [356]
	addw     %dx,%dx
#;;   add.w     d1,d1                  # [357]
	addw     %dx,%dx
#;;   bra.b     crmpas1                # [358]
	jmp      CRMPAS1
CRMPAS2:
#;;   move.w    d1,d0                  # [360]
	movw     %dx,%bx
#;;   and.w     #$fff0,d0              # [361]
	andw     $0xfff0,%bx
#;;   and.w     #15,d1                 # [362]
	andw     $15,%dx
#;;   lsl.w     #4,d1                  # [363]
	shlw     $4,%dx
#;;   add.w     d1,d0                  # [364]
	addw     %dx,%bx
#;;   cmp.w     d0,d2                  # [365 EQ]
	cmpw     %bx,d2_w
#;;   beq.b     crmpaso                # [366]
	je       CRMPASO
#;;   move.w    #UERR_Password,UI_ErrorNum(a4) # [367]
	movl     a4_l,%ecx
	movw     $-1,UI_ErrorNum
#;;   moveq     #0,d0                  # [368]
	movl     $0,%ebx
#;;   rts                              # [369]
	movl %ebx,%eax
	ret      
CRMPASO:
#;;   moveq     #-1,d0                 # [371]
	movl     $-1,%ebx
#;;   rts                              # [372]
	movl %ebx,%eax
	ret      
CRMDCOD:
#;;   movem.l   d0-d3/a0-a4,-(sp)      # [384]
	pushl    a4_l
	pushl    a3_l
	pushl    a2_l
	pushl    %edi
	pushl    %esi
	pushl    d3_l
	pushl    d2_l
	pushl    %edx
	pushl    %ebx
#;;   move.l    sp,a2                  # [385]
	movl     %esp,a2_l
#;;   lea       -256(sp),sp            # [386]
	lea      -256(%esp),%esp
#;;   lsr.l     #1,d0                  # [387]
	shrl     $1,%ebx
#;;   subq.l    #2,d0                  # [388 MI]
	subl     $2,%ebx
#;;   bmi.b     crmdco6                # [389]
	js       CRMDCO6
#;;   move.w    d0,d1                  # [390]
	movw     %bx,%dx
#;;   swap      d0                     # [391]
	roll     $16,%ebx
#;;   move.l    a1,a4                  # [392]
	movl     %edi,a4_l
CRMDCO1:
#;;   clr.b     -(a2)                  # [393]
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     $0,(%ecx)
#;;   tst.b     (a1)+                  # [394 NE]
	testb    $0xff,(%edi)
	lea      1(%edi),%edi
#;;   bne.b     crmdco1                # [395]
	jne      CRMDCO1
#;;   clr.b     -(a2)                  # [396]
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     $0,(%ecx)
#;;   move.l    a2,d2                  # [397]
	movl     a2_l,%eax
	movl     %eax,d2_l
#;;   btst      #0,d2                  # [398 EQ]
	testl    $1<<((0)&31),d2_l
#;;   beq.b     crmdco2                # [399]
	je       CRMDCO2
#;;   clr.b     -(a2)                  # [400]
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     $0,(%ecx)
CRMDCO2:
#;;   move.l    a2,a1                  # [402]
	movl     a2_l,%edi
CRMDCO3:
#;;   move.b    (a4)+,(a1)+            # [403 NE]
	movl     a4_l,%ecx
	movb     (%ecx),%al
	addl     $1,a4_l
	stosb    
	testb    %al,%al
#;;   bne.b     crmdco3                # [404]
	jne      CRMDCO3
#;;   move.l    a2,a1                  # [406]
	movl     a2_l,%edi
#;;   move.l    a1,a4                  # [407]
	movl     %edi,a4_l
CRMDCO4:
#;;   move.w    (a1),d2                # [408 NE]
	movw     (%edi),%ax
	movw     %ax,d2_w
	testw    %ax,%ax
#;;   bne.b     crmdco5                # [409]
	jne      CRMDCO5
#;;   move.l    a4,a1                  # [410]
	movl     a4_l,%edi
#;;   move.w    (a1),d2                # [411]
	movw     (%edi),%ax
	movw     %ax,d2_w
CRMDCO5:
#;;   move.w    (a0)+,d3               # [412]
	lodsw    
	movw     %ax,d3_w
#;;   eor.w     d3,d2                  # [413]
	movw     d3_w,%ax
	xorw     %ax,d2_w
#;;   move.w    d2,(a3)+               # [414]
	movw     d2_w,%ax
	movl     a3_l,%ecx
	addl     $2,a3_l
	movw     %ax,(%ecx)
#;;   add.w     d2,(a1)+               # [415]
	addw     %ax,(%edi)
	lea      2(%edi),%edi
#;;   dbra      d1,crmdco4             # [416]
	decw     %dx
	cmpw     $-1,%dx
	jne      CRMDCO4
#;;   dbra      d0,crmdco4             # [417]
	decw     %bx
	cmpw     $-1,%bx
	jne      CRMDCO4
#;;   move.w    (a0),d0                # [419]
	movw     (%esi),%bx
#;;   and.w     #15,d0                 # [420]
	andw     $15,%bx
#;;   move.w    d0,(a3)                # [421]
	movl     a3_l,%ecx
	movw     %bx,(%ecx)
CRMDCO6:
#;;   lea       256(sp),sp             # [423]
	lea      256(%esp),%esp
#;;   movem.l   (sp)+,d0-d3/a0-a4      # [424]
	popl     %ebx
	popl     %edx
	popl     d2_l
	popl     d3_l
	popl     %esi
	popl     %edi
	popl     a2_l
	popl     a3_l
	popl     a4_l
#;;   rts                              # [425]
	ret      
