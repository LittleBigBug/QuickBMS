#;------------------------------------------------------------------------------
#; Discovery File Imploder Unpacking Routine
#;
#; IN :	A0 = Pointer To Destination Buffer
#;
#; OUT:	D0 = Success (0=Error)
#;
    .text
    .globl UNDIMP
    .globl _UNDIMP
UNDIMP:
_UNDIMP:
    movl 4(%esp), %esi


#;;   move.l    a0,a3                  # [9]
	movl     %esi,a3_l
#;;   move.l    a0,a4                  # [10]
	movl     %esi,a4_l
#;;   addq.l    #4,a0                  # [11]
	lea      4(%esi),%esi
#;;   add.l     (a0)+,a4               # [12]
	movl     0(%esi),%eax
	addl     %eax,a4_l
#;;   add.l     (a0)+,a3               # [13]
	movl     4(%esi),%eax
	addl     %eax,a3_l
	lea      8(%esi),%esi
#;;   move.l    a3,a2                  # [14]
	movl     a3_l,%eax
	movl     %eax,a2_l
#;;   move.l    (a2)+,-(a0)            # [15]
	lea      -12(%esi),%esi
	movl     a2_l,%ecx
	movl     0(%ecx),%eax
	movl     %eax,8(%esi)
#;;   move.l    (a2)+,-(a0)            # [16]
	movl     4(%ecx),%eax
	movl     %eax,4(%esi)
#;;   move.l    (a2)+,-(a0)            # [17]
	movl     8(%ecx),%eax
	movl     %eax,0(%esi)
#;;   move.l    (a2)+,d2               # [18]
	movl     12(%ecx),%eax
	movl     %eax,d2_l
#;;   move.w    (a2)+,d3               # [19 MI]
	movw     16(%ecx),%ax
	addl     $18,a2_l
	movw     %ax,d3_w
	testw    %ax,%ax
#;;   bmi.b     unimp1                 # [20]
	js       UNIMP1
#;;   subq.l    #1,a3                  # [21]
	subl     $1,a3_l
UNIMP1:
#;;   lea       -$1c(sp),sp            # [23]
	lea      -0x1c(%esp),%esp
#;;   move.l    sp,a1                  # [24]
	movl     %esp,%edi
#;;   moveq     #7-1,d0                # [25]
	movl     $7-1,%ebx
UNIMP2:
#;;   move.l    (a2)+,(a1)+            # [26]
	movl     a2_l,%ecx
	movl     (%ecx),%eax
	addl     $4,a2_l
	stosl    
#;;   dbra      d0,unimp2              # [27]
	decw     %bx
	cmpw     $-1,%bx
	jne      UNIMP2
#;;   move.l    sp,a1                  # [29]
	movl     %esp,%edi
#;;   moveq     #0,d4                  # [30]
	movl     $0,d4_l
UNIMP3:
#;;   tst.l     d2                     # [31 EQ]
	testl    $0xffffffff,d2_l
#;;   beq.b     unimp5                 # [32]
	je       UNIMP5
UNIMP4:
#;;   move.b    -(a3),-(a4)            # [33]
	subl     $1,a3_l
	subl     $1,a4_l
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movl     a4_l,%ecx
	movb     %al,(%ecx)
#;;   subq.l    #1,d2                  # [34 NE]
	subl     $1,d2_l
#;;   bne.b     unimp4                 # [35]
	jne      UNIMP4
UNIMP5:
#;;   cmp.l     a4,a0                  # [37 CS]
	cmpl     a4_l,%esi
#;;   bcs.b     unimp8                 # [38]
	jc       UNIMP8
#;;   lea       $1c(sp),sp             # [39]
	lea      0x1c(%esp),%esp
#;;   moveq     #-1,d0                 # [40]
	movl     $-1,%ebx
#;;   cmp.l     a3,a0                  # [41 EQ]
	cmpl     a3_l,%esi
#;;   beq.b     unimp7                 # [42]
	je       UNIMP7
UNIMP6:
#;;   moveq     #0,d0                  # [43]
	movl     $0,%ebx
UNIMP7:
#;;   rts                              # [44]
	movl %ebx,%eax
	ret      
#;;   UNIMP8  unimpm  unimp13          # [66 (Macro)]
UNIMP8:
#;;   add.b     d3,d3                  # [66 CC NE X]
	movb     d3_b,%al
	addb     %al,d3_b
	setcb    xflag
#;;   bne.b     .unimp__000            # [66]
	jne      .UNIMP__000
#;;   move.b    -(a3),d3               # [66]
	subl     $1,a3_l
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movb     %al,d3_b
#;;   addx.b    d3,d3                  # [66 CC]
	movb     d3_b,%al
	btw      $0,xflag
	adcb     %al,d3_b
.UNIMP__000:
#;;   bcc.b     unimp13                # [66]
	jnc      UNIMP13
#;;           unimpm  unimp12          # [67 (Macro)]
#;;   add.b     d3,d3                  # [67 CC NE X]
	movb     d3_b,%al
	addb     %al,d3_b
	setcb    xflag
#;;   bne.b     .unimp__001            # [67]
	jne      .UNIMP__001
#;;   move.b    -(a3),d3               # [67]
	subl     $1,a3_l
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movb     %al,d3_b
#;;   addx.b    d3,d3                  # [67 CC]
	movb     d3_b,%al
	btw      $0,xflag
	adcb     %al,d3_b
.UNIMP__001:
#;;   bcc.b     unimp12                # [67]
	jnc      UNIMP12
#;;           unimpm  unimp11          # [68 (Macro)]
#;;   add.b     d3,d3                  # [68 CC NE X]
	movb     d3_b,%al
	addb     %al,d3_b
	setcb    xflag
#;;   bne.b     .unimp__002            # [68]
	jne      .UNIMP__002
#;;   move.b    -(a3),d3               # [68]
	subl     $1,a3_l
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movb     %al,d3_b
#;;   addx.b    d3,d3                  # [68 CC]
	movb     d3_b,%al
	btw      $0,xflag
	adcb     %al,d3_b
.UNIMP__002:
#;;   bcc.b     unimp11                # [68]
	jnc      UNIMP11
#;;           unimpm  unimp10          # [69 (Macro)]
#;;   add.b     d3,d3                  # [69 CC NE X]
	movb     d3_b,%al
	addb     %al,d3_b
	setcb    xflag
#;;   bne.b     .unimp__003            # [69]
	jne      .UNIMP__003
#;;   move.b    -(a3),d3               # [69]
	subl     $1,a3_l
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movb     %al,d3_b
#;;   addx.b    d3,d3                  # [69 CC]
	movb     d3_b,%al
	btw      $0,xflag
	adcb     %al,d3_b
.UNIMP__003:
#;;   bcc.b     unimp10                # [69]
	jnc      UNIMP10
#;;           unimpm  unimp9           # [70 (Macro)]
#;;   add.b     d3,d3                  # [70 CC NE X]
	movb     d3_b,%al
	addb     %al,d3_b
	setcb    xflag
#;;   bne.b     .unimp__004            # [70]
	jne      .UNIMP__004
#;;   move.b    -(a3),d3               # [70]
	subl     $1,a3_l
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movb     %al,d3_b
#;;   addx.b    d3,d3                  # [70 CC]
	movb     d3_b,%al
	btw      $0,xflag
	adcb     %al,d3_b
.UNIMP__004:
#;;   bcc.b     unimp9                 # [70]
	jnc      UNIMP9
#;;   move.b    -(a3),d4               # [71]
	subl     $1,a3_l
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movb     %al,d4_b
#;;   moveq     #3,d0                  # [72]
	movl     $3,%ebx
#;;   bra.b     unimp14                # [73]
	jmp      UNIMP14
UNIMP9:
#;;           unimpm1                  # [76 (Macro)]
#;;   add.b     d3,d3                  # [76 NE X]
	movb     d3_b,%al
	addb     %al,d3_b
	setcb    xflag
#;;   bne.b     .unimp__005            # [76]
	jne      .UNIMP__005
#;;   move.b    -(a3),d3               # [76]
	subl     $1,a3_l
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movb     %al,d3_b
#;;   addx.b    d3,d3                  # [76 X]
	movb     d3_b,%al
	btw      $0,xflag
	adcb     %al,d3_b
	setcb    xflag
.UNIMP__005:
#;;   addx.b    d4,d4                  # [76]
	movb     d4_b,%al
	btw      $0,xflag
	adcb     %al,d4_b
#;;           unimpm1                  # [76 (Macro)]
#;;   add.b     d3,d3                  # [76 NE X]
	movb     d3_b,%al
	addb     %al,d3_b
	setcb    xflag
#;;   bne.b     .unimp__006            # [76]
	jne      .UNIMP__006
#;;   move.b    -(a3),d3               # [76]
	subl     $1,a3_l
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movb     %al,d3_b
#;;   addx.b    d3,d3                  # [76 X]
	movb     d3_b,%al
	btw      $0,xflag
	adcb     %al,d3_b
	setcb    xflag
.UNIMP__006:
#;;   addx.b    d4,d4                  # [76]
	movb     d4_b,%al
	btw      $0,xflag
	adcb     %al,d4_b
#;;           unimpm1                  # [76 (Macro)]
#;;   add.b     d3,d3                  # [76 NE X]
	movb     d3_b,%al
	addb     %al,d3_b
	setcb    xflag
#;;   bne.b     .unimp__007            # [76]
	jne      .UNIMP__007
#;;   move.b    -(a3),d3               # [76]
	subl     $1,a3_l
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movb     %al,d3_b
#;;   addx.b    d3,d3                  # [76 X]
	movb     d3_b,%al
	btw      $0,xflag
	adcb     %al,d3_b
	setcb    xflag
.UNIMP__007:
#;;   addx.b    d4,d4                  # [76]
	movb     d4_b,%al
	btw      $0,xflag
	adcb     %al,d4_b
#;;   addq.b    #6,d4                  # [78]
	addb     $6,d4_b
#;;   moveq     #3,d0                  # [79]
	movl     $3,%ebx
#;;   bra.b     unimp14                # [80]
	jmp      UNIMP14
UNIMP10:
#;;   moveq     #5,d4                  # [82]
	movl     $5,d4_l
#;;   moveq     #3,d0                  # [83]
	movl     $3,%ebx
#;;   bra.b     unimp14                # [84]
	jmp      UNIMP14
UNIMP11:
#;;   moveq     #4,d4                  # [86]
	movl     $4,d4_l
#;;   moveq     #2,d0                  # [87]
	movl     $2,%ebx
#;;   bra.b     unimp14                # [88]
	jmp      UNIMP14
UNIMP12:
#;;   moveq     #3,d4                  # [90]
	movl     $3,d4_l
#;;   moveq     #1,d0                  # [91]
	movl     $1,%ebx
#;;   bra.b     unimp14                # [92]
	jmp      UNIMP14
UNIMP13:
#;;   moveq     #2,d4                  # [94]
	movl     $2,d4_l
#;;   moveq     #0,d0                  # [95]
	movl     $0,%ebx
UNIMP14:
#;;   moveq     #0,d5                  # [96]
	movl     $0,d5_l
#;;   move.w    d0,d1                  # [97]
	movw     %bx,%dx
#;;           unimpm  unimp16          # [98 (Macro)]
#;;   add.b     d3,d3                  # [98 CC NE X]
	movb     d3_b,%al
	addb     %al,d3_b
	setcb    xflag
#;;   bne.b     .unimp__008            # [98]
	jne      .UNIMP__008
#;;   move.b    -(a3),d3               # [98]
	subl     $1,a3_l
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movb     %al,d3_b
#;;   addx.b    d3,d3                  # [98 CC]
	movb     d3_b,%al
	btw      $0,xflag
	adcb     %al,d3_b
.UNIMP__008:
#;;   bcc.b     unimp16                # [98]
	jnc      UNIMP16
#;;           unimpm  unimp15          # [99 (Macro)]
#;;   add.b     d3,d3                  # [99 CC NE X]
	movb     d3_b,%al
	addb     %al,d3_b
	setcb    xflag
#;;   bne.b     .unimp__009            # [99]
	jne      .UNIMP__009
#;;   move.b    -(a3),d3               # [99]
	subl     $1,a3_l
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movb     %al,d3_b
#;;   addx.b    d3,d3                  # [99 CC]
	movb     d3_b,%al
	btw      $0,xflag
	adcb     %al,d3_b
.UNIMP__009:
#;;   bcc.b     unimp15                # [99]
	jnc      UNIMP15
#;;   move.b    unimp24(pc,d0.w),d5    # [100]
	movswl   %bx,%ecx
	movb     UNIMP24(%ecx),%al
	movb     %al,d5_b
#;;   addq.b    #8,d0                  # [101]
	addb     $8,%bl
#;;   bra.b     unimp16                # [102]
	jmp      UNIMP16
UNIMP15:
#;;   moveq     #2,d5                  # [104]
	movl     $2,d5_l
#;;   addq.b    #4,d0                  # [105]
	addb     $4,%bl
UNIMP16:
#;;   move.b    unimp25(pc,d0.w),d0    # [106]
	movswl   %bx,%ecx
	movb     UNIMP25(%ecx),%bl
UNIMP17:
#;;   add.b     d3,d3                  # [107 NE X]
	movb     d3_b,%al
	addb     %al,d3_b
	setcb    xflag
#;;   bne.b     unimp18                # [108]
	jne      UNIMP18
#;;   move.b    -(a3),d3               # [109]
	subl     $1,a3_l
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movb     %al,d3_b
#;;   addx.b    d3,d3                  # [110 X]
	movb     d3_b,%al
	btw      $0,xflag
	adcb     %al,d3_b
	setcb    xflag
UNIMP18:
#;;   addx.w    d2,d2                  # [111]
	movw     d2_w,%ax
	btw      $0,xflag
	adcw     %ax,d2_w
#;;   subq.b    #1,d0                  # [112 NE]
	subb     $1,%bl
#;;   bne.b     unimp17                # [113]
	jne      UNIMP17
#;;   add.w     d5,d2                  # [114]
	movw     d5_w,%ax
	addw     %ax,d2_w
#;;   moveq     #0,d5                  # [115]
	movl     $0,d5_l
#;;   move.l    d5,a2                  # [116]
	movl     d5_l,%eax
	movl     %eax,a2_l
#;;   move.w    d1,d0                  # [117]
	movw     %dx,%bx
#;;           unimpm  unimp20          # [118 (Macro)]
#;;   add.b     d3,d3                  # [118 CC NE X]
	movb     d3_b,%al
	addb     %al,d3_b
	setcb    xflag
#;;   bne.b     .unimp__010            # [118]
	jne      .UNIMP__010
#;;   move.b    -(a3),d3               # [118]
	subl     $1,a3_l
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movb     %al,d3_b
#;;   addx.b    d3,d3                  # [118 CC]
	movb     d3_b,%al
	btw      $0,xflag
	adcb     %al,d3_b
.UNIMP__010:
#;;   bcc.b     unimp20                # [118]
	jnc      UNIMP20
#;;   add.w     d1,d1                  # [119]
	addw     %dx,%dx
#;;           unimpm  unimp19          # [120 (Macro)]
#;;   add.b     d3,d3                  # [120 CC NE X]
	movb     d3_b,%al
	addb     %al,d3_b
	setcb    xflag
#;;   bne.b     .unimp__011            # [120]
	jne      .UNIMP__011
#;;   move.b    -(a3),d3               # [120]
	subl     $1,a3_l
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movb     %al,d3_b
#;;   addx.b    d3,d3                  # [120 CC]
	movb     d3_b,%al
	btw      $0,xflag
	adcb     %al,d3_b
.UNIMP__011:
#;;   bcc.b     unimp19                # [120]
	jnc      UNIMP19
#;;   move.w    8(a1,d1.w),a2          # [121]
	movswl   %dx,%ecx
	movw     8(%edi,%ecx),%cx
	movswl   %cx,%eax
#;;   addq.b    #8,d0                  # [122]
	addb     $8,%bl
	movl     %eax,a2_l
#;;   bra.b     unimp20                # [123]
	jmp      UNIMP20
UNIMP19:
#;;   move.w    (a1,d1.w),a2           # [125]
	movswl   %dx,%ecx
	movw     0(%edi,%ecx),%cx
	movswl   %cx,%eax
#;;   addq.b    #4,d0                  # [126]
	addb     $4,%bl
	movl     %eax,a2_l
UNIMP20:
#;;   move.b    16(a1,d0.w),d0         # [127]
	movswl   %bx,%ecx
	movb     16(%edi,%ecx),%bl
UNIMP21:
#;;   add.b     d3,d3                  # [128 NE X]
	movb     d3_b,%al
	addb     %al,d3_b
	setcb    xflag
#;;   bne.b     unimp22                # [129]
	jne      UNIMP22
#;;   move.b    -(a3),d3               # [130]
	subl     $1,a3_l
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movb     %al,d3_b
#;;   addx.b    d3,d3                  # [131 X]
	movb     d3_b,%al
	btw      $0,xflag
	adcb     %al,d3_b
	setcb    xflag
UNIMP22:
#;;   addx.l    d5,d5                  # [132]
	movl     d5_l,%eax
	btw      $0,xflag
	adcl     %eax,d5_l
#;;   subq.b    #1,d0                  # [133 NE]
	subb     $1,%bl
#;;   bne.b     unimp21                # [134]
	jne      UNIMP21
#;;   addq.w    #1,a2                  # [135]
	addl     $1,a2_l
#;;   add.l     d5,a2                  # [136]
	movl     d5_l,%eax
	addl     %eax,a2_l
#;;   add.l     a4,a2                  # [137]
	movl     a4_l,%eax
	addl     %eax,a2_l
UNIMP23:
#;;   move.b    -(a2),-(a4)            # [138]
	subl     $1,a2_l
	subl     $1,a4_l
	movl     a2_l,%ecx
	movb     (%ecx),%al
	movl     a4_l,%ecx
	movb     %al,(%ecx)
#;;   subq.b    #1,d4                  # [139 NE]
	subb     $1,d4_b
#;;   bne.b     unimp23                # [140]
	jne      UNIMP23
#;;   bra.w     unimp3                 # [141]
	jmp      UNIMP3
	.data
	.p2align	2
#;;   UNIMP24 dc.b    6,10,10,18       # [143]
UNIMP24:
	.byte	6,10,10,18
#;;   UNIMP25 dc.b    1,1,1,1,2,3,3,4,4,5,7,14 # [144]
UNIMP25:
	.byte	1,1,1,1,2,3,3,4,4,5,7,14
