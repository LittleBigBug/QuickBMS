#;------------------------------------------------------------------------------
#; IAM/ATM5 Decruncher
#;
#; IN :	A0 = Pointer To (De)crunched Data
#;
#; OUT:	Nothing
#;
    .text
    .globl IAMATM
    .globl _IAMATM
IAMATM:
_IAMATM:
    movl 4(%esp),%esi


#;;   link      a2,#-$1c               # [9]
	pushl    a2_l
	movl     %esp,a2_l
	lea      (-0x1c)(%esp),%esp
#;;   addq.l    #4,a0                  # [10]
	lea      4(%esi),%esi
#;;   move.l    (a0)+,d0               # [11]
	movl     (%esi),%ebx
	lea      4(%esi),%esi
#;;   lea       4(a0,d0.l),a5          # [12]
	lea      4(%esi,%ebx),%eax
#;;   move.l    d0,-(sp)               # [13]
	pushl    %ebx
#;;   move.l    a5,a4                  # [14]
	movl     %eax,a4_l
#;;   lea       -12(a4),a4             # [15]
	addl     $(-12),a4_l
#;;   move.l    (a0)+,d0               # [16]
	movl     (%esi),%ebx
	lea      4(%esi),%esi
#;;   move.l    a0,a6                  # [17]
	movl     %esi,%ebp
#;;   add.l     d0,a6                  # [18]
	lea      (%ebp,%ebx),%ebp
#;;   subq.l    #1,a6                  # [19]
	lea      -1(%ebp),%ebp
#;;   move.b    -(a6),d7               # [20]
	movl     %eax,a5_l
	lea      -1(%ebp),%ebp
	movb     (%ebp),%al
	movb     %al,d7_b
#;;   bra.w     iamat32                # [21]
	jmp      IAMAT32
IAMATM1:
#;;   move.w    d3,d5                  # [23]
	movw     d3_w,%ax
	movw     %ax,d5_w
IAMATM2:
#;;   add.b     d7,d7                  # [24 CC CS EQ X]
	movb     d7_b,%al
	addb     %al,d7_b
	setcb    xflag
IAMATM3:
#;;   dbcs      d5,iamatm2             # [25]
	jc       _PA_15_
	movw     d5_w,%cx
	lea      -1(%ecx),%cx
	movw     %cx,d5_w
	lea      1(%ecx),%cx
	jcxz     _PA_15_
	jmp      IAMATM2
_PA_15_:         
#;;   beq.b     iamatm6                # [26]
	je       IAMATM6
#;;   bcc.b     iamatm4                # [27]
	jnc      IAMATM4
#;;   sub.w     d3,d5                  # [28]
	movw     d3_w,%ax
	subw     %ax,d5_w
#;;   neg.w     d5                     # [29]
	negw     d5_w
#;;   bra.b     iamatm9                # [30]
	jmp      IAMATM9
IAMATM4:
#;;   moveq     #3,d6                  # [32]
	movl     $3,d6_l
#;;   bsr.b     iamat15                # [33 (EQ)]
	call     IAMAT15
#;;   beq.b     iamatm5                # [34]
	je       IAMATM5
#;;   bra.b     iamatm8                # [35]
	jmp      IAMATM8
IAMATM5:
#;;   moveq     #7,d6                  # [37]
	movl     $7,d6_l
#;;   bsr.b     iamat15                # [38 (EQ)]
	call     IAMAT15
#;;   beq.b     iamatm7                # [39]
	je       IAMATM7
#;;   add.w     #15,d5                 # [40]
	addw     $15,d5_w
#;;   bra.b     iamatm8                # [41]
	jmp      IAMATM8
IAMATM6:
#;;   move.b    -(a6),d7               # [43 EQ]
	lea      -1(%ebp),%ebp
	movb     (%ebp),%al
	movb     %al,d7_b
	testb    %al,%al
#;;   addx.b    d7,d7                  # [44 CC CS EQ X]
	movb     d7_b,%cl
	setnzb   %al
	shlb     $6,%al
	notb     %al
	btw      $0,xflag
	adcb     %cl,d7_b
	pushf    
	andb     %al,(%esp)
	popf     
	setcb    xflag
#;;   bra.b     iamatm3                # [45]
	jmp      IAMATM3
IAMATM7:
#;;   moveq     #13,d6                 # [47]
	movl     $13,d6_l
#;;   bsr.b     iamat15                # [48]
	call     IAMAT15
#;;   add.w     #$10e,d5               # [49]
	addw     $0x10e,d5_w
IAMATM8:
#;;   add.w     d3,d5                  # [50]
	movw     d3_w,%ax
	addw     %ax,d5_w
IAMATM9:
#;;   lea       iamatt1(pc),a4         # [51]
	movl     $IAMATT1,a4_l
#;;   move.w    d5,d2                  # [52 NE]
	movw     d5_w,%ax
	movw     %ax,d2_w
	testw    %ax,%ax
#;;   bne.b     iamat19                # [53]
	jne      IAMAT19
#;;   add.b     d7,d7                  # [54 CS NE X]
	movb     d7_b,%al
	addb     %al,d7_b
	setcb    xflag
#;;   bne.b     iamat10                # [55]
	jne      IAMAT10
#;;   move.b    -(a6),d7               # [56]
	lea      -1(%ebp),%ebp
	movb     (%ebp),%al
	movb     %al,d7_b
#;;   addx.b    d7,d7                  # [57 CS]
	movb     d7_b,%al
	btw      $0,xflag
	adcb     %al,d7_b
IAMAT10:
#;;   bcs.b     iamat11                # [58]
	jc       IAMAT11
#;;   moveq     #1,d6                  # [59]
	movl     $1,d6_l
#;;   bra.b     iamat20                # [60]
	jmp      IAMAT20
IAMAT11:
#;;   moveq     #3,d6                  # [62]
	movl     $3,d6_l
#;;   bsr.b     iamat15                # [63]
	call     IAMAT15
#;;   tst.b     -$1c(a2)               # [64 EQ]
	movl     a2_l,%ecx
	testb    $0xff,-0x1c(%ecx)
#;;   beq.b     iamat12                # [65]
	je       IAMAT12
#;;   move.b    -$12(a2,d5.w),-(a5)    # [66]
	subl     $1,a5_l
	movswl   d5_w,%ecx
	addl     a2_l,%ecx
	movb     -0x12(%ecx),%al
	movl     a5_l,%ecx
	movb     %al,(%ecx)
#;;   bra.w     iamat31                # [67]
	jmp      IAMAT31
IAMAT12:
#;;   move.b    (a5),d0                # [69]
	movl     a5_l,%ecx
	movb     (%ecx),%bl
#;;   btst      #3,d5                  # [70 NE]
	testl    $1<<((3)&31),d5_l
#;;   bne.b     iamat13                # [71]
	jne      IAMAT13
#;;   bra.b     iamat14                # [72]
	jmp      IAMAT14
IAMAT13:
#;;   add.b     #$f0,d5                # [74]
	addb     $0xf0,d5_b
IAMAT14:
#;;   sub.b     d5,d0                  # [75]
	subb     d5_b,%bl
#;;   move.b    d0,-(a5)               # [76]
	subl     $1,a5_l
	movl     a5_l,%ecx
	movb     %bl,(%ecx)
#;;   bra.w     iamat31                # [77]
	jmp      IAMAT31
IAMAT15:
#;;   clr.w     d5                     # [79]
	movw     $0,d5_w
IAMAT16:
#;;   add.b     d7,d7                  # [80 EQ X]
	movb     d7_b,%al
	addb     %al,d7_b
	setcb    xflag
#;;   beq.b     iamat18                # [81]
	je       IAMAT18
IAMAT17:
#;;   addx.w    d5,d5                  # [82]
	movw     d5_w,%ax
	btw      $0,xflag
	adcw     %ax,d5_w
#;;   dbra      d6,iamat16             # [83]
	decw     d6_w
	cmpw     $-1,d6_w
	jne      IAMAT16
#;;   tst.w     d5                     # [84 EQ]
	testw    $0xffff,d5_w
#;;   rts                              # [85]
	ret      
IAMAT18:
#;;   move.b    -(a6),d7               # [87]
	lea      -1(%ebp),%ebp
	movb     (%ebp),%al
	movb     %al,d7_b
#;;   addx.b    d7,d7                  # [88 X]
	movb     d7_b,%al
	btw      $0,xflag
	adcb     %al,d7_b
	setcb    xflag
#;;   bra.b     iamat17                # [89]
	jmp      IAMAT17
IAMAT19:
#;;   moveq     #2,d6                  # [91]
	movl     $2,d6_l
IAMAT20:
#;;   bsr.b     iamat15                # [92]
	call     IAMAT15
#;;   move.w    d5,d4                  # [93]
	movw     d5_w,%ax
	movw     %ax,d4_w
#;;   move.b    14(a4,d4.w),d6         # [94]
	movswl   d4_w,%ecx
	addl     a4_l,%ecx
	movb     14(%ecx),%al
	movb     %al,d6_b
#;;   ext.w     d6                     # [95]
	movsbl   d6_b,%eax
	movw     %ax,d6_w
#;;   tst.b     -$1b(a2)               # [96 NE]
	movl     a2_l,%ecx
	testb    $0xff,-0x1b(%ecx)
#;;   bne.b     iamat21                # [97]
	jne      IAMAT21
#;;   addq.w    #4,d6                  # [98]
	addw     $4,d6_w
#;;   bra.b     iamat25                # [99]
	jmp      IAMAT25
IAMAT21:
#;;   bsr.b     iamat15                # [101]
	call     IAMAT15
#;;   move.w    d5,d1                  # [102]
	movw     d5_w,%dx
#;;   lsl.w     #4,d1                  # [103]
	shlw     $4,%dx
#;;   moveq     #2,d6                  # [104]
	movl     $2,d6_l
#;;   bsr.b     iamat15                # [105]
	call     IAMAT15
#;;   cmp.b     #7,d5                  # [106 LT]
	cmpb     $7,d5_b
#;;   blt.b     iamat23                # [107]
	jl       IAMAT23
#;;   moveq     #0,d6                  # [108]
	movl     $0,d6_l
#;;   bsr.b     iamat15                # [109 (EQ)]
	call     IAMAT15
#;;   beq.b     iamat22                # [110]
	je       IAMAT22
#;;   moveq     #2,d6                  # [111]
	movl     $2,d6_l
#;;   bsr.b     iamat15                # [112]
	call     IAMAT15
#;;   add.w     d5,d5                  # [113]
	movw     d5_w,%ax
	addw     %ax,d5_w
#;;   or.w      d1,d5                  # [114]
	orw      %dx,d5_w
#;;   bra.b     iamat26                # [115]
	jmp      IAMAT26
IAMAT22:
#;;   or.b      -$1a(a2),d1            # [117]
	movl     a2_l,%ecx
	orb      -0x1a(%ecx),%dl
#;;   bra.b     iamat24                # [118]
	jmp      IAMAT24
IAMAT23:
#;;   or.b      -$19(a2,d5.w),d1       # [120]
	movswl   d5_w,%ecx
	addl     a2_l,%ecx
	orb      -0x19(%ecx),%dl
IAMAT24:
#;;   move.w    d1,d5                  # [121]
	movw     %dx,d5_w
#;;   bra.b     iamat26                # [122]
	jmp      IAMAT26
IAMAT25:
#;;   bsr.b     iamat15                # [124]
	call     IAMAT15
IAMAT26:
#;;   add.w     d4,d4                  # [125 EQ]
	movw     d4_w,%ax
	addw     %ax,d4_w
#;;   beq.b     iamat27                # [126]
	je       IAMAT27
#;;   add.w     -2(a4,d4.w),d5         # [127]
	movswl   d4_w,%ecx
	addl     a4_l,%ecx
	movw     -2(%ecx),%ax
	addw     %ax,d5_w
IAMAT27:
#;;   lea       1(a5,d5.w),a4          # [128]
	movswl   d5_w,%ecx
	addl     a5_l,%ecx
	lea      1(%ecx),%eax
	movl     %eax,a4_l
#;;   move.b    -(a4),-(a5)            # [129]
	subl     $1,a4_l
	subl     $1,a5_l
	movl     a4_l,%ecx
	movb     (%ecx),%al
	movl     a5_l,%ecx
	movb     %al,(%ecx)
IAMAT28:
#;;   move.b    -(a4),-(a5)            # [130]
	subl     $1,a4_l
	subl     $1,a5_l
	movl     a4_l,%ecx
	movb     (%ecx),%al
	movl     a5_l,%ecx
	movb     %al,(%ecx)
#;;   dbra      d2,iamat28             # [131]
	decw     d2_w
	cmpw     $-1,d2_w
	jne      IAMAT28
#;;   bra.b     iamat31                # [132]
	jmp      IAMAT31
IAMAT29:
#;;   add.b     d7,d7                  # [134 CS NE X]
	movb     d7_b,%al
	addb     %al,d7_b
	setcb    xflag
#;;   bne.b     iamat30                # [135]
	jne      IAMAT30
#;;   move.b    -(a6),d7               # [136]
	lea      -1(%ebp),%ebp
	movb     (%ebp),%al
	movb     %al,d7_b
#;;   addx.b    d7,d7                  # [137 CS]
	movb     d7_b,%al
	btw      $0,xflag
	adcb     %al,d7_b
IAMAT30:
#;;   bcs.w     iamatm1                # [138]
	jc       IAMATM1
#;;   move.b    -(a6),-(a5)            # [139]
	lea      -1(%ebp),%ebp
	subl     $1,a5_l
	movb     (%ebp),%al
	movl     a5_l,%ecx
	movb     %al,(%ecx)
IAMAT31:
#;;   cmp.l     a5,a3                  # [140 NE]
	movl     a5_l,%eax
	cmpl     %eax,a3_l
#;;   bne.b     iamat29                # [141]
	jne      IAMAT29
#;;   cmp.l     a6,a0                  # [142 EQ]
	cmpl     %ebp,%esi
#;;   beq.b     iamat40                # [143]
	je       IAMAT40
IAMAT32:
#;;   moveq     #0,d6                  # [144]
	movl     $0,d6_l
#;;   bsr.w     iamat15                # [145 (EQ)]
	call     IAMAT15
#;;   beq.b     iamat35                # [146]
	je       IAMAT35
#;;   move.b    -(a6),d0               # [147]
	lea      -1(%ebp),%ebp
	movb     (%ebp),%bl
#;;   lea       -$1a(a2),a1            # [148]
	movl     a2_l,%ecx
	lea      -0x1a(%ecx),%edi
#;;   move.b    d0,(a1)+               # [149]
	movb     %bl,(%edi)
	lea      1(%edi),%edi
#;;   moveq     #1,d1                  # [150]
	movl     $1,%edx
#;;   moveq     #6,d2                  # [151]
	movl     $6,d2_l
IAMAT33:
#;;   cmp.b     d0,d1                  # [152 NE]
	cmpb     %bl,%dl
#;;   bne.b     iamat34                # [153]
	jne      IAMAT34
#;;   addq.w    #2,d1                  # [154]
	addw     $2,%dx
IAMAT34:
#;;   move.b    d1,(a1)+               # [155]
	movb     %dl,(%edi)
	lea      1(%edi),%edi
#;;   addq.w    #2,d1                  # [156]
	addw     $2,%dx
#;;   dbra      d2,iamat33             # [157]
	decw     d2_w
	cmpw     $-1,d2_w
	jne      IAMAT33
#;;   st        -$1b(a2)               # [158]
	movl     a2_l,%ecx
	movb     $0xff,-0x1b(%ecx)
#;;   bra.b     iamat36                # [159]
	jmp      IAMAT36
IAMAT35:
#;;   sf        -$1b(a2)               # [161]
	movl     a2_l,%ecx
	movb     $0,-0x1b(%ecx)
IAMAT36:
#;;   moveq     #0,d6                  # [162]
	movl     $0,d6_l
#;;   bsr.w     iamat15                # [163 (EQ)]
	call     IAMAT15
#;;   beq.b     iamat38                # [164]
	je       IAMAT38
#;;   lea       -$12(a2),a1            # [165]
	movl     a2_l,%ecx
	lea      -0x12(%ecx),%edi
#;;   moveq     #15,d0                 # [166]
	movl     $15,%ebx
IAMAT37:
#;;   move.b    -(a6),(a1)+            # [167]
	lea      -1(%ebp),%ebp
	movb     (%ebp),%al
	stosb    
#;;   dbra      d0,iamat37             # [168]
	decw     %bx
	cmpw     $-1,%bx
	jne      IAMAT37
#;;   st        -$1c(a2)               # [169]
	movl     a2_l,%ecx
	movb     $0xff,-0x1c(%ecx)
#;;   bra.b     iamat39                # [170]
	jmp      IAMAT39
IAMAT38:
#;;   sf        -$1c(a2)               # [172]
	movl     a2_l,%ecx
	movb     $0,-0x1c(%ecx)
IAMAT39:
#;;   clr.w     d3                     # [173]
	movw     $0,d3_w
#;;   move.b    -(a6),d3               # [174]
	lea      -3(%ebp),%ebp
	movb     2(%ebp),%al
	movb     %al,d3_b
#;;   move.b    -(a6),d0               # [175]
	movb     1(%ebp),%bl
#;;   lsl.w     #8,d0                  # [176]
	shlw     $8,%bx
#;;   move.b    -(a6),d0               # [177]
	movb     0(%ebp),%bl
#;;   move.l    a5,a3                  # [178]
	movl     a5_l,%eax
	movl     %eax,a3_l
#;;   sub.w     d0,a3                  # [179]
	movswl   %bx,%eax
	subl     %eax,a3_l
#;;   bra.b     iamat29                # [180]
	jmp      IAMAT29
IAMAT40:
#;;   move.l    (sp)+,d0               # [182]
	popl     %ebx
#;;   lsr.l     #4,d0                  # [183]
	shrl     $4,%ebx
#;;   lea       -12(a6),a6             # [184]
	lea      -12(%ebp),%ebp
IAMAT41:
#;;   move.l    (a5)+,(a6)+            # [185]
	movl     a5_l,%ecx
	movl     0(%ecx),%eax
	movl     %eax,0(%ebp)
#;;   move.l    (a5)+,(a6)+            # [186]
	movl     4(%ecx),%eax
	movl     %eax,4(%ebp)
#;;   move.l    (a5)+,(a6)+            # [187]
	movl     8(%ecx),%eax
	movl     %eax,8(%ebp)
#;;   move.l    (a5)+,(a6)+            # [188]
	movl     12(%ecx),%eax
	addl     $16,a5_l
	movl     %eax,12(%ebp)
	lea      16(%ebp),%ebp
#;;   dbra      d0,iamat41             # [189]
	decw     %bx
	cmpw     $-1,%bx
	jne      IAMAT41
#;;   unlk      a2                     # [190]
	movl     a2_l,%esp
	popl     a2_l
#;;   rts                              # [191]
	ret      
	.data
	.p2align	2
#;;   IAMATT1 dc.w    $0020,$0060,$0160,$0360,$0760,$0f60,$1f60 # [193]
IAMATT1:
	.short	0x20,0x60,0x160,0x360,0x760,0xf60,0x1f60
#;;           dc.b    0,1,3,4,5,6,7,8  # [194]
	.byte	0,1,3,4,5,6,7,8
	.text
	.p2align	2
    
    
    
    
    
#;------------------------------------------------------------------------------
#; IAM/ICE Decruncher
#;
#; IN :	A0 = Pointer To (De)crunched Data
#;
#; OUT:	Nothing
#;
    .globl  IAMICE
    .globl  _IAMICE
IAMICE:
_IAMICE:
    movl 4(%esp),%esi
    

#;;   link      a3,#-$78               # [203]
	pushl    a3_l
	movl     %esp,a3_l
	lea      (-0x78)(%esp),%esp
#;;   movem.l   d0-d7/a0-a6,-(sp)      # [204]
	pushl    %ebp
	pushl    a5_l
	pushl    a4_l
	pushl    a3_l
	pushl    a2_l
	pushl    %edi
	pushl    %esi
	pushl    d7_l
	pushl    d6_l
	pushl    d5_l
	pushl    d4_l
	pushl    d3_l
	pushl    d2_l
	pushl    %edx
	pushl    %ebx
#;;   lea       $78(a0),a4             # [205]
	lea      0x78(%esi),%eax
#;;   move.l    a4,a6                  # [206]
	movl     %eax,a4_l
	movl     a4_l,%ebp
#;;   addq.l    #4,a0                  # [207]
	lea      4(%esi),%esi
#;;   move.l    (a0)+,d0               # [208]
	movl     (%esi),%ebx
	lea      4(%esi),%esi
#;;   lea       -8(a0,d0.l),a5         # [209]
	lea      -8(%esi,%ebx),%eax
#;;   move.l    (a0)+,d0               # [210]
	movl     (%esi),%ebx
	lea      4(%esi),%esi
#;;   move.l    d0,(sp)                # [211]
	movl     %ebx,(%esp)
#;;   add.l     d0,a6                  # [212]
	lea      (%ebp,%ebx),%ebp
#;;   move.l    a6,a1                  # [213]
	movl     %ebp,%edi
#;;   moveq     #$78-1,d0              # [214]
	movl     $0x78-1,%ebx
	movl     %eax,a5_l
IAMICE1:
#;;   move.b    -(a1),-(a3)            # [215]
	lea      -1(%edi),%edi
	subl     $1,a3_l
	movb     (%edi),%al
	movl     a3_l,%ecx
	movb     %al,(%ecx)
#;;   dbra      d0,iamice1             # [216]
	decw     %bx
	cmpw     $-1,%bx
	jne      IAMICE1
#;;   move.l    a6,a3                  # [218]
	movl     %ebp,a3_l
#;;   move.b    -(a5),d7               # [219]
	subl     $1,a5_l
	movl     a5_l,%ecx
	movb     (%ecx),%al
	movb     %al,d7_b
#;;   bsr.b     iamic10                # [220]
	call     IAMIC10
#;;   move.l    a3,a5                  # [221]
	movl     a3_l,%eax
	movl     %eax,a5_l
#;;   bsr.b     iamic14                # [222 (CC)]
	call     IAMIC14
#;;   bcc.b     iamic4a                # [223]
	jnc      IAMIC4a
#;;   move.w    #$0f9f,d7              # [224]
	movw     $0xf9f,d7_w
#;;   bsr.b     iamic14                # [225 (CC)]
	call     IAMIC14
#;;   bcc.b     iamice2                # [226]
	jnc      IAMICE2
#;;   moveq     #15,d0                 # [227]
	movl     $15,%ebx
#;;   bsr.b     iamic16                # [228]
	call     IAMIC16
#;;   move.w    d1,d7                  # [229]
	movw     %dx,d7_w
IAMICE2:
#;;   moveq     #3,d6                  # [230]
	movl     $3,d6_l
IAMICE3:
#;;   move.w    -(a3),d4               # [231]
	subl     $2,a3_l
	movl     a3_l,%ecx
	movw     (%ecx),%ax
	movw     %ax,d4_w
#;;   moveq     #3,d5                  # [232]
	movl     $3,d5_l
IAMICE4:
#;;   add.w     d4,d4                  # [233 X]
	movw     d4_w,%ax
	addw     %ax,d4_w
#;;   addx.w    d0,d0                  # [234]
	adcw     %bx,%bx
#;;   add.w     d4,d4                  # [235 X]
	movw     d4_w,%ax
	addw     %ax,d4_w
#;;   addx.w    d1,d1                  # [236]
	adcw     %dx,%dx
#;;   add.w     d4,d4                  # [237 X]
	movw     d4_w,%ax
	addw     %ax,d4_w
#;;   addx.w    d2,d2                  # [238]
	movw     d2_w,%ax
	adcw     %ax,d2_w
#;;   add.w     d4,d4                  # [239 X]
	movw     d4_w,%ax
	addw     %ax,d4_w
#;;   addx.w    d3,d3                  # [240]
	movw     d3_w,%ax
	adcw     %ax,d3_w
#;;   dbra      d5,iamice4             # [241]
	decw     d5_w
	cmpw     $-1,d5_w
	jne      IAMICE4
#;;   dbra      d6,iamice3             # [242]
	decw     d6_w
	cmpw     $-1,d6_w
	jne      IAMICE3
#;;   movem.w   d0-d3,(a3)             # [243]
	movl     a3_l,%ecx
	movw     %bx,0(%ecx)
	movw     %dx,2(%ecx)
	movw     d2_w,%ax
	movw     %ax,4(%ecx)
	movw     d3_w,%ax
	movw     %ax,6(%ecx)
#;;   dbra      d7,iamice2             # [244]
	decw     d7_w
	cmpw     $-1,d7_w
	jne      IAMICE2
IAMIC4a:
#;;   movem.l   (sp),d0-d7/a0-a3       # [246]
	movl     0(%esp),%ebx
	movl     4(%esp),%edx
	movl     8(%esp),%eax
	movl     %eax,d2_l
	movl     12(%esp),%eax
	movl     %eax,d3_l
	movl     16(%esp),%eax
	movl     %eax,d4_l
	movl     20(%esp),%eax
	movl     %eax,d5_l
	movl     24(%esp),%eax
	movl     %eax,d6_l
	movl     28(%esp),%eax
	movl     %eax,d7_l
	movl     32(%esp),%esi
	movl     36(%esp),%edi
	movl     40(%esp),%eax
	movl     %eax,a2_l
	movl     44(%esp),%eax
	movl     %eax,a3_l
IAMICE5:
#;;   move.b    (a4)+,(a0)+            # [247]
	movl     a4_l,%ecx
	movb     (%ecx),%al
	addl     $1,a4_l
	movb     %al,(%esi)
	lea      1(%esi),%esi
#;;   subq.l    #1,d0                  # [248 NE]
	subl     $1,%ebx
#;;   bne.b     iamice5                # [249]
	jne      IAMICE5
#;;   moveq     #$78-1,d0              # [251]
	movl     $0x78-1,%ebx
IAMICE6:
#;;   move.b    -(a3),-(a5)            # [252]
	subl     $1,a3_l
	subl     $1,a5_l
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movl     a5_l,%ecx
	movb     %al,(%ecx)
#;;   dbra      d0,iamice6             # [253]
	decw     %bx
	cmpw     $-1,%bx
	jne      IAMICE6
IAMICE7:
#;;   movem.l   (sp)+,d0-d7/a0-a6      # [255]
	popl     %ebx
	popl     %edx
	popl     d2_l
	popl     d3_l
	popl     d4_l
	popl     d5_l
	popl     d6_l
	popl     d7_l
	popl     %esi
	popl     %edi
	popl     a2_l
	popl     a3_l
	popl     a4_l
	popl     a5_l
	popl     %ebp
#;;   unlk      a3                     # [256]
	movl     a3_l,%esp
	popl     a3_l
#;;   rts                              # [257]
	ret      
IAMIC10:
#;;   bsr.b     iamic14                # [259 (CC)]
	call     IAMIC14
#;;   bcc.b     iamic13                # [260]
	jnc      IAMIC13
#;;   moveq     #0,d1                  # [261]
	movl     $0,%edx
#;;   bsr.b     iamic14                # [262 (CC)]
	call     IAMIC14
#;;   bcc.b     iamic12                # [263]
	jnc      IAMIC12
#;;   lea       iamict1(pc),a1         # [264]
	movl     $IAMICT1,%edi
#;;   moveq     #4,d3                  # [265]
	movl     $4,d3_l
IAMIC11:
#;;   move.l    -(a1),d0               # [266]
	lea      -4(%edi),%edi
	movl     (%edi),%ebx
#;;   bsr.b     iamic16                # [267]
	call     IAMIC16
#;;   swap      d0                     # [268]
	roll     $16,%ebx
#;;   cmp.w     d0,d1                  # [269 NE]
	cmpw     %bx,%dx
#;;   dbne      d3,iamic11             # [270]
	jne      _PA_228_
	decw     d3_w
	cmpw     $-1,d3_w
	jne      IAMIC11
_PA_228_:         
#;;   add.l     $14(a1),d1             # [271]
	addl     0x14(%edi),%edx
IAMIC12:
#;;   move.b    -(a5),-(a6)            # [272]
	subl     $1,a5_l
	lea      -1(%ebp),%ebp
	movl     a5_l,%ecx
	movb     (%ecx),%al
	movb     %al,(%ebp)
#;;   dbra      d1,iamic12             # [273]
	decw     %dx
	cmpw     $-1,%dx
	jne      IAMIC12
IAMIC13:
#;;   cmp.l     a4,a6                  # [274 GT]
	cmpl     a4_l,%ebp
#;;   bgt.b     iamic19                # [275]
	jg       IAMIC19
#;;   rts                              # [276]
	ret      
IAMIC14:
#;;   add.b     d7,d7                  # [278 CC NE X]
	movb     d7_b,%al
	addb     %al,d7_b
	setcb    xflag
#;;   bne.b     iamic15                # [279]
	jne      IAMIC15
#;;   move.b    -(a5),d7               # [280]
	subl     $1,a5_l
	movl     a5_l,%ecx
	movb     (%ecx),%al
	movb     %al,d7_b
#;;   addx.b    d7,d7                  # [281 CC]
	movb     d7_b,%al
	btw      $0,xflag
	adcb     %al,d7_b
IAMIC15:
#;;   rts                              # [282]
	ret      
IAMIC16:
#;;   moveq     #0,d1                  # [284]
	movl     $0,%edx
IAMIC17:
#;;   add.b     d7,d7                  # [285 NE X]
	movb     d7_b,%al
	addb     %al,d7_b
	setcb    xflag
#;;   bne.b     iamic18                # [286]
	jne      IAMIC18
#;;   move.b    -(a5),d7               # [287]
	subl     $1,a5_l
	movl     a5_l,%ecx
	movb     (%ecx),%al
	movb     %al,d7_b
#;;   addx.b    d7,d7                  # [288 X]
	movb     d7_b,%al
	btw      $0,xflag
	adcb     %al,d7_b
	setcb    xflag
IAMIC18:
#;;   addx.w    d1,d1                  # [289]
	btw      $0,xflag
	adcw     %dx,%dx
#;;   dbra      d0,iamic17             # [290]
	decw     %bx
	cmpw     $-1,%bx
	jne      IAMIC17
#;;   rts                              # [291]
	ret      
IAMIC19:
#;;   lea       iamict2(pc),a1         # [293]
	movl     $IAMICT2,%edi
#;;   moveq     #3,d2                  # [294]
	movl     $3,d2_l
IAMIC20:
#;;   bsr.b     iamic14                # [295 (CC)]
	call     IAMIC14
#;;   dbcc      d2,iamic20             # [296]
	jnc      _PA_251_
	decw     d2_w
	cmpw     $-1,d2_w
	jne      IAMIC20
_PA_251_:         
#;;   moveq     #0,d4                  # [297]
	movl     $0,d4_l
#;;   moveq     #0,d1                  # [298]
	movl     $0,%edx
#;;   move.b    1(a1,d2.w),d0          # [299]
	movswl   d2_w,%ecx
	movb     1(%edi,%ecx),%bl
#;;   ext.w     d0                     # [300 MI]
	movsbw   %bl,%bx
	testw    $-1,%bx
#;;   bmi.b     iamic21                # [301]
	js       IAMIC21
#;;   bsr.b     iamic16                # [302]
	call     IAMIC16
IAMIC21:
#;;   move.b    6(a1,d2.w),d4          # [303]
	movswl   d2_w,%ecx
	movb     6(%edi,%ecx),%al
	movb     %al,d4_b
#;;   add.w     d1,d4                  # [304 EQ]
	addw     %dx,d4_w
#;;   beq.b     iamic23                # [305]
	je       IAMIC23
#;;   lea       iamict3(pc),a1         # [306]
	movl     $IAMICT3,%edi
#;;   moveq     #1,d2                  # [307]
	movl     $1,d2_l
IAMIC22:
#;;   bsr.b     iamic14                # [308 (CC)]
	call     IAMIC14
#;;   dbcc      d2,iamic22             # [309]
	jnc      _PA_264_
	decw     d2_w
	cmpw     $-1,d2_w
	jne      IAMIC22
_PA_264_:         
#;;   moveq     #0,d1                  # [310]
	movl     $0,%edx
#;;   move.b    1(a1,d2.w),d0          # [311]
	movswl   d2_w,%ecx
	movb     1(%edi,%ecx),%bl
#;;   ext.w     d0                     # [312]
	movsbw   %bl,%bx
#;;   bsr.b     iamic16                # [313]
	call     IAMIC16
#;;   add.w     d2,d2                  # [314]
	movw     d2_w,%ax
	addw     %ax,d2_w
#;;   add.w     6(a1,d2.w),d1          # [315 PL]
	movswl   d2_w,%ecx
	addw     6(%edi,%ecx),%dx
#;;   bpl.b     iamic25                # [316]
	jns      IAMIC25
#;;   sub.w     d4,d1                  # [317]
	subw     d4_w,%dx
#;;   bra.b     iamic25                # [318]
	jmp      IAMIC25
IAMIC23:
#;;   moveq     #0,d1                  # [320]
	movl     $0,%edx
#;;   moveq     #5,d0                  # [321]
	movl     $5,%ebx
#;;   moveq     #-1,d2                 # [322]
	movl     $-1,d2_l
#;;   bsr.b     iamic14                # [323 (CC)]
	call     IAMIC14
#;;   bcc.b     iamic24                # [324]
	jnc      IAMIC24
#;;   moveq     #8,d0                  # [325]
	movl     $8,%ebx
#;;   moveq     #$3f,d2                # [326]
	movl     $0x3f,d2_l
IAMIC24:
#;;   bsr.b     iamic16                # [327]
	call     IAMIC16
#;;   add.w     d2,d1                  # [328]
	addw     d2_w,%dx
IAMIC25:
#;;   lea       2(a6,d4.w),a1          # [329]
	movswl   d4_w,%ecx
	lea      2(%ebp,%ecx),%edi
#;;   add.w     d1,a1                  # [330]
	movswl   %dx,%eax
	lea      (%edi,%eax),%edi
#;;   move.b    -(a1),-(a6)            # [331]
	lea      -1(%edi),%edi
	lea      -1(%ebp),%ebp
	movb     (%edi),%al
	movb     %al,(%ebp)
IAMIC26:
#;;   move.b    -(a1),-(a6)            # [332]
	lea      -1(%edi),%edi
	lea      -1(%ebp),%ebp
	movb     (%edi),%al
	movb     %al,(%ebp)
#;;   dbra      d4,iamic26             # [333]
	decw     d4_w
	cmpw     $-1,d4_w
	jne      IAMIC26
#;;   bra.w     iamic10                # [334]
	jmp      IAMIC10
	.data
	.p2align	2
#;;           dc.l    $7fff000e,$00ff0007,$00070002,$00030001,$00030001 # [336]
	.long	0x7fff000e,0xff0007,0x70002,0x30001,0x30001
#;;   IAMICT1 dc.l    $0000010d,$0000000e,$00000007,$00000004,$00000001 # [337]
IAMICT1:
	.long	0x10d,0xe,0x7,0x4,0x1
#;;   IAMICT2 dc.b    9,1,0,-1,-1,8,4,2,1,0 # [338]
IAMICT2:
	.byte	9,1,0,-1,-1,8,4,2,1,0
#;;   IAMICT3 dc.b    11,4,7,0,1,31,-1,-1,0,31,0,0,0,0,0,0,0,0 # [339]
IAMICT3:
	.byte	11,4,7,0,1,31,-1,-1,0,31,0,0,0,0,0,0,0,0
