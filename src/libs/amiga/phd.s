#;------------------------------------------------------------------------------
#; PHD Decrunching Routine
#;
#; IN :	A0 = Pointer To Crunched Data
#;	A1 = Pointer To Decrunched Data
#;	A2 = Pointer To Work Buffer
#;
#; OUT:	Nothing
#;
    .text
    .global UPHD
    .global _UPHD
UPHD:
_UPHD:
    movl 4(%esp),%esi
    movl 8(%esp),%edi
    movl 12(%esp),%eax
    movl %eax,a2_l

#;;   move.l    a4,-(sp)               # [12]
	pushl    a4_l
#;;   addq.l    #8,a0                  # [13]
	lea      8(%esi),%esi
#;;   moveq     #0,d6                  # [14]
	movl     $0,d6_l
#;;   move.w    (a0)+,d6               # [15]
	lodsw    
	movw     %ax,d6_w
#;;   moveq     #15,d7                 # [16]
	movl     $15,d7_l
#;;   lea       $4c4(a2),a2            # [17]
	addl     $0x4c4,a2_l
#;;   lea       $4c6(a2),a3            # [18]
	movl     a2_l,%ecx
	lea      0x4c6(%ecx),%eax
#;;   lea       $98c(a2),a4            # [19]
	movl     %eax,a3_l
	lea      0x98c(%ecx),%eax
#;;   lea       uphd26(pc),a5          # [20]
	movl     $UPHD26,a5_l
	movl     %eax,a4_l
#;;   bra.b     uphd2                  # [21]
	jmp      UPHD2
UPHD1:
#;;   tst.w     (a2)                   # [23 NE]
	movl     a2_l,%ecx
	testw    $0xffff,(%ecx)
#;;   bne.b     uphd5                  # [24]
	jne      UPHD5
UPHD2:
#;;   movem.l   a3-a5,-(sp)            # [25]
	pushl    a5_l
	pushl    a4_l
	pushl    a3_l
#;;   lea       -$4c4(a2),a5           # [26]
	movl     a2_l,%ecx
	lea      -0x4c4(%ecx),%eax
#;;   lea       -$4c4(a3),a6           # [27]
	movl     a3_l,%ecx
	lea      -0x4c4(%ecx),%ebp
#;;   move.l    a5,a3                  # [28]
	movl     %eax,a3_l
#;;   move.w    #$131,d1               # [29]
	movw     $0x131,%dx
#;;   moveq     #1,d2                  # [30]
	movl     $1,d2_l
	movl     %eax,a5_l
UPHD3:
#;;   move.w    d2,(a5)+               # [31]
	movw     d2_w,%ax
	movl     a5_l,%ecx
	addl     $2,a5_l
	movw     %ax,(%ecx)
#;;   move.w    d1,(a6)+               # [32]
	movw     %dx,(%ebp)
	lea      2(%ebp),%ebp
#;;   dbra      d1,uphd3               # [33]
	decw     %dx
	cmpw     $-1,%dx
	jne      UPHD3
#;;   move.w    #$130,d1               # [34]
	movw     $0x130,%dx
#;;   move.w    #$fb3c,d2              # [35]
	movw     $0xfb3c,d2_w
#;;   moveq     #0,d3                  # [36]
	movl     $0,d3_l
UPHD4:
#;;   move.w    (a3)+,d0               # [37]
	movl     a3_l,%ecx
	movw     0(%ecx),%bx
#;;   add.w     (a3)+,d0               # [38]
	addl     $4,a3_l
	addw     2(%ecx),%bx
#;;   move.w    d0,(a5)+               # [39]
	movl     a5_l,%ecx
	addl     $2,a5_l
	movw     %bx,(%ecx)
#;;   move.w    d2,(a6)+               # [40]
	movw     d2_w,%ax
	movw     %ax,(%ebp)
	lea      2(%ebp),%ebp
#;;   addq.w    #4,d2                  # [41]
	addw     $4,d2_w
#;;   move.w    d3,-(a4)               # [42]
	movw     d3_w,%ax
	subl     $4,a4_l
	movl     a4_l,%ecx
	movw     %ax,2(%ecx)
#;;   move.w    d3,-(a4)               # [43]
	movw     %ax,0(%ecx)
#;;   subq.w    #2,d3                  # [44]
	subw     $2,d3_w
#;;   dbra      d1,uphd4               # [45]
	decw     %dx
	cmpw     $-1,%dx
	jne      UPHD4
#;;   movem.l   (sp)+,a3-a5            # [46]
	popl     a3_l
	popl     a4_l
	popl     a5_l
#;;   bra.b     uphd10                 # [47]
	jmp      UPHD10
UPHD5:
#;;   lea       (a2,d3.w),a6           # [49]
	movswl   d3_w,%ecx
	addl     a2_l,%ecx
	lea      0(%ecx),%ebp
#;;   move.w    (a6)+,d0               # [50]
	movw     0(%ebp),%bx
#;;   addq.w    #1,d0                  # [51]
	addw     $1,%bx
#;;   cmp.w     (a6)+,d0               # [52 LS]
	cmpw     2(%ebp),%bx
	lea      4(%ebp),%ebp
#;;   bls.b     uphd9                  # [53]
	jbe      UPHD9
UPHD6:
#;;   cmp.w     (a6)+,d0               # [54 HI]
	cmpw     (%ebp),%bx
	lea      2(%ebp),%ebp
#;;   bhi.b     uphd6                  # [55]
	ja       UPHD6
#;;   subq.l    #4,a6                  # [56]
	lea      -4(%ebp),%ebp
#;;   sub.l     a2,a6                  # [57]
	subl     a2_l,%ebp
#;;   move.w    (a3,a6.w),d1           # [58 PL]
	movswl   %bp,%ecx
	addl     a3_l,%ecx
	movw     0(%ecx),%dx
	testw    %dx,%dx
#;;   bpl.b     uphd7                  # [59]
	jns      UPHD7
#;;   move.w    d3,(a4,d1.w)           # [60]
	movw     d3_w,%ax
	movswl   %dx,%ecx
	addl     a4_l,%ecx
	movw     %ax,0(%ecx)
#;;   move.w    d3,2(a4,d1.w)          # [61]
	movw     %ax,2(%ecx)
UPHD7:
#;;   move.w    (a3,d3.w),d2           # [62 PL]
	movswl   d3_w,%ecx
	addl     a3_l,%ecx
	movw     0(%ecx),%ax
	movw     %ax,d2_w
	testw    %ax,%ax
#;;   bpl.b     uphd8                  # [63]
	jns      UPHD8
#;;   move.w    a6,(a4,d2.w)           # [64]
	movswl   d2_w,%ecx
	addl     a4_l,%ecx
	movw     %bp,0(%ecx)
#;;   move.w    a6,2(a4,d2.w)          # [65]
	movw     %bp,2(%ecx)
UPHD8:
#;;   move.w    d1,(a3,d3.w)           # [66]
	movswl   d3_w,%ecx
	addl     a3_l,%ecx
	movw     %dx,0(%ecx)
#;;   move.w    d2,(a3,a6.w)           # [67]
	movw     d2_w,%ax
	movswl   %bp,%ecx
	addl     a3_l,%ecx
	movw     %ax,0(%ecx)
#;;   move.w    a6,d3                  # [68]
	movw     %bp,d3_w
UPHD9:
#;;   move.w    d0,(a2,d3.w)           # [69]
	movswl   d3_w,%ecx
	addl     a2_l,%ecx
	movw     %bx,0(%ecx)
#;;   move.w    (a4,d3.w),d3           # [70 NE]
	movswl   d3_w,%ecx
	addl     a4_l,%ecx
	movw     0(%ecx),%ax
	movw     %ax,d3_w
	testw    %ax,%ax
#;;   bne.b     uphd5                  # [71]
	jne      UPHD5
#;;   addq.w    #1,(a2)                # [72]
	movl     a2_l,%ecx
	addw     $1,(%ecx)
UPHD10:
#;;   moveq     #0,d3                  # [73]
	movl     $0,d3_l
UPHD11:
#;;   move.w    (a3,d3.w),d4           # [74 PL]
	movswl   d3_w,%ecx
	addl     a3_l,%ecx
	movw     0(%ecx),%ax
	movw     %ax,d4_w
	testw    %ax,%ax
#;;   bpl.b     uphd13                 # [75]
	jns      UPHD13
#;;   add.w     d6,d6                  # [76 CC]
	movw     d6_w,%ax
	addw     %ax,d6_w
#;;   bcc.b     uphd12                 # [77]
	jnc      UPHD12
#;;   addq.w    #2,d4                  # [78]
	addw     $2,d4_w
UPHD12:
#;;   move.w    d4,d3                  # [79]
	movw     d4_w,%ax
	movw     %ax,d3_w
#;;   dbra      d7,uphd11              # [80]
	decw     d7_w
	cmpw     $-1,d7_w
	jne      UPHD11
#;;   moveq     #15,d7                 # [81]
	movl     $15,d7_l
#;;   move.w    (a0)+,d6               # [82]
	lodsw    
	movw     %ax,d6_w
#;;   bra.b     uphd11                 # [83]
	jmp      UPHD11
UPHD13:
#;;   cmp.w     #$100,d4               # [85 CC NE]
	cmpw     $0x100,d4_w
#;;   bcc.b     uphd16                 # [86]
	jnc      UPHD16
#;;   move.b    d4,(a1)+               # [87]
	movb     d4_b,%al
	stosb    
#;;   bra.w     uphd1                  # [88]
	jmp      UPHD1
UPHD14:
#;;   cmp.w     #$131,d4               # [90 EQ]
	cmpw     $0x131,d4_w
#;;   beq.b     uphd15                 # [91]
	je       UPHD15
#;;   lea       $18(a5),a5             # [92]
	addl     $0x18,a5_l
#;;   bra.w     uphd1                  # [93]
	jmp      UPHD1
UPHD15:
#;;   move.l    (sp)+,a4               # [95]
	popl     a4_l
#;;   rts                              # [96]
	ret      
UPHD16:
#;;   bne.b     uphd17                 # [98]
	jne      UPHD17
#;;   moveq     #12,d0                 # [99]
	movl     $12,%ebx
#;;   bra.b     uphd20                 # [100]
	jmp      UPHD20
UPHD17:
#;;   cmp.w     #$12f,d4               # [102 CS HI]
	cmpw     $0x12f,d4_w
#;;   bcs.b     uphd19                 # [103]
	jc       UPHD19
#;;   bhi.b     uphd14                 # [104]
	ja       UPHD14
#;;   moveq     #6,d1                  # [105]
	movl     $6,%edx
#;;   cmp.b     d1,d7                  # [106 CC]
	cmpb     %dl,d7_b
#;;   bcc.b     uphd18                 # [107]
	jnc      UPHD18
#;;   addq.b    #1,d7                  # [108]
	addb     $1,d7_b
#;;   lsl.l     d7,d6                  # [109]
	movb     d7_b,%al
	movl     %eax,%ecx
	shll     %cl,d6_l
#;;   sub.b     d7,d1                  # [110]
	subb     d7_b,%dl
#;;   move.w    (a0)+,d6               # [111]
	lodsw    
	movw     %ax,d6_w
#;;   moveq     #15,d7                 # [112]
	movl     $15,d7_l
UPHD18:
#;;   lsl.l     d1,d6                  # [113]
	movl     %edx,%ecx
	shll     %cl,d6_l
#;;   sub.b     d1,d7                  # [114]
	subb     %dl,d7_b
#;;   swap      d6                     # [115]
	roll     $16,d6_l
#;;   add.w     d6,d4                  # [116]
	movw     d6_w,%ax
	addw     %ax,d4_w
#;;   clr.w     d6                     # [117]
	movw     $0,d6_w
#;;   swap      d6                     # [118]
	roll     $16,d6_l
UPHD19:
#;;   moveq     #0,d0                  # [119]
	movl     $0,%ebx
UPHD20:
#;;   add.w     d6,d6                  # [120 CC]
	movw     d6_w,%ax
	addw     %ax,d6_w
#;;   bcc.b     uphd22                 # [121]
	jnc      UPHD22
#;;   addq.w    #4,d0                  # [122]
	addw     $4,%bx
#;;   dbra      d7,uphd21              # [123]
	decw     d7_w
	cmpw     $-1,d7_w
	jne      UPHD21
#;;   moveq     #15,d7                 # [124]
	movl     $15,d7_l
#;;   move.w    (a0)+,d6               # [125]
	lodsw    
	movw     %ax,d6_w
UPHD21:
#;;   add.w     d6,d6                  # [126 CC]
	movw     d6_w,%ax
	addw     %ax,d6_w
#;;   bcc.b     uphd22                 # [127]
	jnc      UPHD22
#;;   addq.w    #4,d0                  # [128]
	addw     $4,%bx
UPHD22:
#;;   dbra      d7,uphd23              # [129]
	decw     d7_w
	cmpw     $-1,d7_w
	jne      UPHD23
#;;   moveq     #15,d7                 # [130]
	movl     $15,d7_l
#;;   move.w    (a0)+,d6               # [131]
	lodsw    
	movw     %ax,d6_w
UPHD23:
#;;   move.w    (a5,d0.w),d1           # [132]
	movswl   %bx,%ecx
	addl     a5_l,%ecx
	movw     0(%ecx),%dx
#;;   cmp.b     d1,d7                  # [133 CC]
	cmpb     %dl,d7_b
#;;   bcc.b     uphd24                 # [134]
	jnc      UPHD24
#;;   addq.b    #1,d7                  # [135]
	addb     $1,d7_b
#;;   lsl.l     d7,d6                  # [136]
	movb     d7_b,%al
	movl     %eax,%ecx
	shll     %cl,d6_l
#;;   sub.b     d7,d1                  # [137]
	subb     d7_b,%dl
#;;   move.w    (a0)+,d6               # [138]
	lodsw    
	movw     %ax,d6_w
#;;   moveq     #15,d7                 # [139]
	movl     $15,d7_l
UPHD24:
#;;   lsl.l     d1,d6                  # [140]
	movl     %edx,%ecx
	shll     %cl,d6_l
#;;   sub.b     d1,d7                  # [141]
	subb     %dl,d7_b
#;;   swap      d6                     # [142]
	roll     $16,d6_l
#;;   add.w     2(a5,d0.w),d6          # [143]
	movswl   %bx,%ecx
	addl     a5_l,%ecx
	movw     2(%ecx),%ax
	addw     %ax,d6_w
#;;   neg.w     d6                     # [144]
	negw     d6_w
#;;   lea       -1(a1,d6.w),a6         # [145]
	movswl   d6_w,%ecx
	lea      -1(%edi,%ecx),%ebp
#;;   clr.w     d6                     # [146]
	movw     $0,d6_w
#;;   swap      d6                     # [147]
	roll     $16,d6_l
#;;   sub.w     #$fe,d4                # [148]
	subw     $0xfe,d4_w
UPHD25:
#;;   move.b    (a6)+,(a1)+            # [149]
	movb     (%ebp),%al
	stosb    
	lea      1(%ebp),%ebp
#;;   dbra      d4,uphd25              # [150]
	decw     d4_w
	cmpw     $-1,d4_w
	jne      UPHD25
#;;   bra.w     uphd1                  # [151]
	jmp      UPHD1
	.data
	.p2align	2
#;;   UPHD26  dc.w    5,0,7,32,9,160,5,0,7,32,9,160 # [153]
UPHD26:
	.short	5,0,7,32,9,160,5,0,7,32,9,160
#;;           dc.w    6,0,8,64,10,320,6,0,8,64,10,320 # [154]
	.short	6,0,8,64,10,320,6,0,8,64,10,320
#;;           dc.w    7,0,9,128,11,640,7,0,9,128,11,640 # [155]
	.short	7,0,9,128,11,640,7,0,9,128,11,640
#;;           dc.w    8,0,10,256,12,1280,7,0,9,128,11,640 # [156]
	.short	8,0,10,256,12,1280,7,0,9,128,11,640
#;;           dc.w    8,0,10,256,13,1280,7,0,9,128,11,640 # [157]
	.short	8,0,10,256,13,1280,7,0,9,128,11,640
#;;           dc.w    9,0,11,512,14,2560,7,0,9,128,11,640 # [158]
	.short	9,0,11,512,14,2560,7,0,9,128,11,640
