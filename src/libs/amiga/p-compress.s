    .text
    .globl UPCOMP
    .globl _UPCOMP
UPCOMP:
_UPCOMP:
movl    4(%esp),%ebx    # output
movl    8(%esp),%esi    # input
movl    12(%esp),%edx   # input size
movl    16(%esp),%eax
movl    %eax,UI_DecrunchLen

#;;   move.l    d0,-(sp)               # [2]
	pushl    %ebx
#;;   move.l    a0,-(sp)               # [3]
	pushl    %esi
#;;   bsr.b     upcom                  # [4]
	call     UPCOM
#;;   addq.l    #8,sp                  # [5]
	lea      8(%esp),%esp
#;;   move.w    d0,d4                  # [6]
	movw     %bx,d4_w
UPCOMPO:
#;;   moveq     #-1,d0                 # [8]
	movl     $-1,%ebx
#;;   cmp.w     #1,d4                  # [9 NE]
	cmpw     $1,d4_w
#;;   bne.b     upcomo1                # [10]
	jne      UPCOMO1
#;;   move.w    #UERR_Memory,UI_ErrorNum(a4) # [11]
	movw     $-1,UI_ErrorNum
#;;   moveq     #0,d0                  # [12]
	movl     $0,%ebx
#;;   rts                              # [13]
	movl %ebx,%eax
	ret      
UPCOMO1:
#;;   cmp.w     #2,d4                  # [15 NE]
	cmpw     $2,d4_w
#;;   bne.b     upcomo2                # [16]
	jne      UPCOMO2
#;;   move.w    #UERR_Corrupt,UI_ErrorNum(a4) # [17]
	movw     $-1,UI_ErrorNum
#;;   moveq     #0,d0                  # [18]
	movl     $0,%ebx
UPCOMO2:
#;;   rts                              # [19]
	movl %ebx,%eax
	ret      
GETLONG:
#;;   move.b    (a1),d0                # [21]
	movb     (%edi),%bl
#;;   lsl.l     #8,d0                  # [22]
	shll     $8,%ebx
#;;   move.b    1(a1),d0               # [23]
	movb     1(%edi),%bl
#;;   lsl.l     #8,d0                  # [24]
	shll     $8,%ebx
#;;   move.b    2(a1),d0               # [25]
	movb     2(%edi),%bl
#;;   lsl.l     #8,d0                  # [26]
	shll     $8,%ebx
#;;   move.b    3(a1),d0               # [27]
	movb     3(%edi),%bl
#;;   rts                              # [28]
	ret      
UPCOM:
#;;   link      a5,#-$3c               # [35]
	pushl    a5_l
	movl     %esp,a5_l
	lea      (-0x3c)(%esp),%esp
#;;   movem.l   d2-d7/a1-a6,-(sp)      # [36]
	pushl    %ebp
	pushl    a5_l
	pushl    a4_l
	pushl    a3_l
	pushl    a2_l
	pushl    %edi
	pushl    d7_l
	pushl    d6_l
	pushl    d5_l
	pushl    d4_l
	pushl    d3_l
	pushl    d2_l
#;;   move.l    8(a5),a0               # [37]
	movl     a5_l,%ecx
	movl     8(%ecx),%esi
#;;   move.l    12(a5),-$18(a5)        # [38]
	movl     12(%ecx),%eax
	movl     %eax,-0x18(%ecx)
#;;   move.l    12(a5),-$20(a5)        # [39]
	movl     12(%ecx),%eax
	movl     %eax,-0x20(%ecx)
#;;   move.l    a0,d0                  # [40]
	movl     %esi,%ebx
#;;   add.l     #12,d0                 # [41]
	addl     $12,%ebx
#;;   move.l    d0,-$1c(a5)            # [42]
	movl     %ebx,-0x1c(%ecx)
#;;   sub.l     #14,d1                 # [43]
	subl     $14,%edx
#;;   move.l    d1,-$10(a5)            # [44]
	movl     %edx,-0x10(%ecx)
#;;   lea       8(a0),a1               # [46]
	lea      8(%esi),%edi
#;;   bsr.b     getlong                # [47]
	call     GETLONG
#;;   move.l    d0,-$30(a5)            # [48]
	movl     a5_l,%ecx
	movl     %ebx,-0x30(%ecx)
#;;   move.l    UI_DecrunchLen(a4),d0  # [50]
	movl     UI_DecrunchLen,%ebx
#;;   move.l    d0,-4(a5)              # [51]
	movl     a5_l,%ecx
	movl     %ebx,-4(%ecx)
#;;   add.l     -$20(a5),d0            # [52]
	addl     -0x20(%ecx),%ebx
#;;   move.l    d0,-$34(a5)            # [53]
	movl     %ebx,-0x34(%ecx)
#;;   moveq     #2,d4                  # [55]
	movl     $2,d4_l
#;;   clr.l     -$28(a5)               # [56]
	movl     $0,-0x28(%ecx)
#;;   move.l    #$400,d0               # [57]
	movl     $0x400,%ebx
#;;   bsr.w     alcmem                 # [58]
    push %ebx
	call     _malloc
#;;   move.l    d0,-$2c(a5)            # [59 EQ]
	movl     a5_l,%ecx
	movl     %ebx,-0x2c(%ecx)
	testl    %ebx,%ebx
#;;   beq.w     upcom4                 # [60]
	je       UPCOM4
#;;   move.l    #$13ec,d0              # [62]
	movl     $0x13ec,%ebx
#;;   bsr.w     alcmem                 # [63]
    push %ebx
	call     _malloc
#;;   move.l    d0,-$14(a5)            # [64 EQ]
	movl     a5_l,%ecx
	movl     %ebx,-0x14(%ecx)
	testl    %ebx,%ebx
#;;   beq.b     upcom3                 # [65]
	je       UPCOM3
#;;   move.l    #$1000,d0              # [67]
	movl     $0x1000,%ebx
#;;   bsr.w     alcmem                 # [68]
    push %ebx
	call     _malloc
#;;   move.l    d0,-12(a5)             # [69 EQ]
	movl     a5_l,%ecx
	movl     %ebx,-12(%ecx)
	testl    %ebx,%ebx
#;;   beq.b     upcom2                 # [70]
	je       UPCOM2
#;;   add.l     #$1000,d0              # [71]
	addl     $0x1000,%ebx
#;;   move.l    d0,-$38(a5)            # [72]
	movl     %ebx,-0x38(%ecx)
#;;   clr.l     -8(a5)                 # [74]
	movl     $0,-8(%ecx)
#;;   bsr.w     upcom37                # [75]
	call     UPCOM37
#;;   move.l    -$1c(a5),a0            # [76]
	movl     a5_l,%ecx
	movl     -0x1c(%ecx),%esi
#;;   move.l    -$10(a5),d0            # [77]
	movl     -0x10(%ecx),%ebx
#;;   bsr.w     upcom44                # [78]
	call     UPCOM44
#;;   moveq     #2,d4                  # [79]
	movl     $2,d4_l
#;;   cmp.l     -$30(a5),d0            # [80 NE]
	movl     a5_l,%ecx
	cmpl     -0x30(%ecx),%ebx
#;;   bne.b     upcom1                 # [81]
	jne      UPCOM1
#;;   bsr.b     upcom5                 # [82]
	call     UPCOM5
#;;   move.l    a6,a5                  # [83]
	movl     %ebp,a5_l
UPCOM1:
#;;   move.l    -12(a5),a1             # [85]
	movl     a5_l,%ecx
	movl     -12(%ecx),%edi
#;;   bsr.w     fremem                 # [86]
    push %edi
	call     _free
UPCOM2:
#;;   move.l    -$14(a5),a1            # [88]
	movl     a5_l,%ecx
	movl     -0x14(%ecx),%edi
#;;   bsr.w     fremem                 # [89]
    push %edi
	call     _free
UPCOM3:
#;;   move.l    -$2c(a5),a1            # [91]
	movl     a5_l,%ecx
	movl     -0x2c(%ecx),%edi
#;;   bsr.w     fremem                 # [92]
    push %edi
	call     _free
UPCOM4:
#;;   move.l    d4,d0                  # [94]
	movl     d4_l,%ebx
#;;   movem.l   (sp)+,d2-d7/a1-a6      # [95]
	popl     d2_l
	popl     d3_l
	popl     d4_l
	popl     d5_l
	popl     d6_l
	popl     d7_l
	popl     %edi
	popl     a2_l
	popl     a3_l
	popl     a4_l
	popl     a5_l
	popl     %ebp
#;;   unlk      a5                     # [96]
	movl     a5_l,%esp
	popl     a5_l
#;;   rts                              # [97]
	ret      
UPCOM5:
#;;   move.l    sp,-$3c(a5)            # [99]
	movl     a5_l,%ecx
	movl     %esp,-0x3c(%ecx)
#;;   move.l    d2,-(sp)               # [100]
	pushl    d2_l
#;;   move.l    -$14(a5),a4            # [101]
	movl     -0x14(%ecx),%eax
	movl     %eax,a4_l
#;;   move.l    a4,a0                  # [102]
	movl     a4_l,%esi
#;;   moveq     #0,d1                  # [103]
	movl     $0,%edx
#;;   lea       $11d4(a4),a1           # [104]
	movl     a4_l,%ecx
	lea      0x11d4(%ecx),%edi
#;;   moveq     #2,d3                  # [105]
	movl     $2,d3_l
#;;   move.w    #15,d4                 # [106]
	movw     $15,d4_w
#;;   move.w    #1,d2                  # [107]
	movw     $1,d2_w
#;;   bsr.w     upcom31                # [108]
	call     UPCOM31
#;;   move.w    #7,d3                  # [109]
	movw     $7,d3_w
#;;   move.w    #7,d4                  # [110]
	movw     $7,d4_w
#;;   moveq     #2,d2                  # [111]
	movl     $2,d2_l
#;;   bsr.w     upcom31                # [112]
	call     UPCOM31
#;;   move.w    #11,d3                 # [113]
	movw     $11,d3_w
#;;   moveq     #3,d4                  # [114]
	movl     $3,d4_l
#;;   moveq     #3,d2                  # [115]
	movl     $3,d2_l
#;;   bsr.w     upcom31                # [116]
	call     UPCOM31
#;;   move.w    #$17,d3                # [117]
	movw     $0x17,d3_w
#;;   moveq     #1,d4                  # [118]
	movl     $1,d4_l
#;;   moveq     #4,d2                  # [119]
	movl     $4,d2_l
#;;   bsr.w     upcom31                # [120]
	call     UPCOM31
#;;   move.w    #15,d3                 # [121]
	movw     $15,d3_w
#;;   move.w    #0,d4                  # [122]
	movw     $0,d4_w
#;;   moveq     #5,d2                  # [123]
	movl     $5,d2_l
#;;   bsr.w     upcom31                # [124]
	call     UPCOM31
#;;   lea       $c60(a4),a1            # [126]
	movl     a4_l,%ecx
	lea      0xc60(%ecx),%edi
#;;   lea       $76e(a4),a2            # [127]
	lea      0x76e(%ecx),%eax
#;;   moveq     #-2,d0                 # [128]
	movl     $-2,%ebx
#;;   moveq     #0,d3                  # [129]
	movl     $0,d3_l
#;;   moveq     #1,d1                  # [130]
	movl     $1,%edx
#;;   moveq     #2,d2                  # [131]
	movl     $2,d2_l
#;;   move.w    #$13c,d7               # [132]
	movw     $0x13c,d7_w
	movl     %eax,a2_l
UPCOM6:
#;;   move.w    d1,(a0)+               # [133]
	movw     %dx,(%esi)
	lea      2(%esi),%esi
#;;   move.w    d0,(a1)+               # [134]
	movw     %bx,(%edi)
	lea      2(%edi),%edi
#;;   move.w    d3,-(a2)               # [135]
	movw     d3_w,%ax
	subl     $2,a2_l
	movl     a2_l,%ecx
	movw     %ax,(%ecx)
#;;   sub.w     d2,d0                  # [136]
	subw     d2_w,%bx
#;;   add.w     d2,d3                  # [137]
	movw     d2_w,%ax
	addw     %ax,d3_w
#;;   dbra      d7,upcom6              # [138]
	decw     d7_w
	cmpw     $-1,d7_w
	jne      UPCOM6
#;;   moveq     #0,d6                  # [140]
	movl     $0,d6_l
#;;   move.l    #$27a,d7               # [141]
	movl     $0x27a,d7_l
#;;   move.l    a4,a0                  # [142]
	movl     a4_l,%esi
#;;   lea       $27a(a4),a1            # [143]
	movl     a4_l,%ecx
	lea      0x27a(%ecx),%edi
#;;   lea       $eda(a4),a2            # [144]
	lea      0xeda(%ecx),%eax
#;;   lea       $76e(a4),a3            # [145]
	movl     %eax,a2_l
	lea      0x76e(%ecx),%eax
#;;   moveq     #4,d4                  # [146]
	movl     $4,d4_l
#;;   moveq     #2,d2                  # [147]
	movl     $2,d2_l
#;;   move.w    #$13b,d1               # [148]
	movw     $0x13b,%dx
	movl     %eax,a3_l
UPCOM7:
#;;   move.w    (a0)+,d0               # [149]
	movw     0(%esi),%bx
#;;   add.w     (a0)+,d0               # [150]
	addw     2(%esi),%bx
	lea      4(%esi),%esi
#;;   move.w    d0,(a1)+               # [151]
	movw     %bx,(%edi)
	lea      2(%edi),%edi
#;;   move.w    d6,(a2)+               # [152]
	movw     d6_w,%ax
	movl     a2_l,%ecx
	addl     $2,a2_l
	movw     %ax,(%ecx)
#;;   move.w    d7,(a3)+               # [153]
	movw     d7_w,%ax
	movl     a3_l,%ecx
	movw     %ax,0(%ecx)
#;;   move.w    d7,(a3)+               # [154]
	addl     $4,a3_l
	movw     %ax,2(%ecx)
#;;   add.w     d4,d6                  # [155]
	movw     d4_w,%ax
	addw     %ax,d6_w
#;;   add.w     d2,d7                  # [156]
	movw     d2_w,%ax
	addw     %ax,d7_w
#;;   dbra      d1,upcom7              # [157]
	decw     %dx
	cmpw     $-1,%dx
	jne      UPCOM7
#;;   move.w    d1,(a1)                # [158]
	movw     %dx,(%edi)
#;;   clr.w     $c5e(a4)               # [159]
	movl     a4_l,%ecx
	movw     $0,0xc5e(%ecx)
#;;   move.l    a5,a6                  # [161]
	movl     a5_l,%ebp
#;;   move.l    -12(a6),a0             # [162]
	movl     -12(%ebp),%esi
#;;   lea       $1000(a0),a0           # [163]
	lea      0x1000(%esi),%esi
#;;   move.l    -12(a6),a1             # [164]
	movl     -12(%ebp),%edi
#;;   move.l    -$14(a6),a5            # [165]
	movl     -0x14(%ebp),%eax
	movl     %eax,a5_l
#;;   lea       $76e(a5),a4            # [166]
	movl     a5_l,%ecx
	lea      0x76e(%ecx),%eax
#;;   lea       $c60(a5),a3            # [167]
	movl     %eax,a4_l
	lea      0xc60(%ecx),%eax
#;;   move.l    (sp)+,d0               # [169 EQ]
	popl     %ebx
	testl    %ebx,%ebx
	movl     %eax,a3_l
#;;   beq.b     upcom7a                # [170]
	je       UPCOM7a
#;;   move.l    d0,(a1)+               # [171]
	movl     %ebx,(%edi)
	lea      4(%edi),%edi
#;;   addq.l    #4,-8(a6)              # [172]
	addl     $4,-8(%ebp)
#;;   subq.l    #4,-4(a6)              # [173]
	subl     $4,-4(%ebp)
UPCOM7a:
#;;   moveq     #1,d4                  # [175]
	movl     $1,d4_l
#;;   moveq     #15,d5                 # [176]
	movl     $15,d5_l
#;;   bsr.w     upcom33                # [177]
	call     UPCOM33
#;;   bra.b     upcom10                # [178]
	jmp      UPCOM10
UPCOM8:
#;;   neg.w     d7                     # [180]
	negw     d7_w
#;;   lsr.w     d4,d7                  # [181]
	movb     d4_b,%al
	movw     %ax,%cx
	shrw     %cl,d7_w
#;;   sub.w     d4,d7                  # [182]
	movw     d4_w,%ax
	subw     %ax,d7_w
#;;   cmp.l     -$38(a6),a1            # [183 EQ]
	cmpl     -0x38(%ebp),%edi
#;;   beq.w     upcom36                # [184]
	je       UPCOM36
#;;   move.b    d7,(a1)+               # [185]
	movb     d7_b,%al
	stosb    
#;;   addq.l    #1,-8(a6)              # [186]
	addl     $1,-8(%ebp)
#;;   cmp.l     #$1000,-8(a6)          # [187 NE]
	cmpl     $0x1000,-8(%ebp)
#;;   bne.b     upcom9                 # [188]
	jne      UPCOM9
#;;   bsr.w     upcom34                # [189]
	call     UPCOM34
UPCOM9:
#;;   subq.l    #1,-4(a6)              # [190]
	subl     $1,-4(%ebp)
#;;   tst.l     -4(a6)                 # [191 EQ]
	testl    $0xffffffff,-4(%ebp)
#;;   beq.w     upcom22                # [192]
	je       UPCOM22
UPCOM10:
#;;   move.w    $4f0(a3),d7            # [193]
	movl     a3_l,%ecx
	movw     0x4f0(%ecx),%ax
	movw     %ax,d7_w
UPCOM11:
#;;   add.w     d6,d6                  # [194 CC]
	movw     d6_w,%ax
	addw     %ax,d6_w
#;;   bcc.b     upcom13                # [195]
	jnc      UPCOM13
#;;   move.w    2(a3,d7.w),d7          # [196 MI]
	movswl   d7_w,%ecx
	addl     a3_l,%ecx
	movw     2(%ecx),%ax
	movw     %ax,d7_w
	testw    %ax,%ax
#;;   dbmi      d5,upcom11             # [197]
	js       _PA_170_
	movw     d5_w,%cx
	lea      -1(%ecx),%cx
	movw     %cx,d5_w
	lea      1(%ecx),%cx
	jcxz     _PA_170_
	jmp      UPCOM11
_PA_170_:         
#;;   bmi.b     upcom14                # [198]
	js       UPCOM14
UPCOM12:
#;;   moveq     #15,d5                 # [199]
	movl     $15,d5_l
#;;   bsr.w     upcom33                # [200]
	call     UPCOM33
#;;   bra.b     upcom11                # [201]
	jmp      UPCOM11
UPCOM13:
#;;   move.w    (a3,d7.w),d7           # [203 MI PL]
	movswl   d7_w,%ecx
	addl     a3_l,%ecx
	movw     0(%ecx),%ax
	movw     %ax,d7_w
	testw    %ax,%ax
#;;   dbmi      d5,upcom11             # [204]
	js       _PA_176_
	movw     d5_w,%cx
	lea      -1(%ecx),%cx
	movw     %cx,d5_w
	lea      1(%ecx),%cx
	jcxz     _PA_176_
	jmp      UPCOM11
_PA_176_:         
#;;   bpl.b     upcom12                # [205]
	jns      UPCOM12
UPCOM14:
#;;   dbra      d5,upcom15             # [206]
	decw     d5_w
	cmpw     $-1,d5_w
	jne      UPCOM15
#;;   bsr.w     upcom33                # [207]
	call     UPCOM33
#;;   moveq     #15,d5                 # [208]
	movl     $15,d5_l
UPCOM15:
#;;   cmp.w     #$8000,$4f0(a5)        # [209 EQ]
	movl     a5_l,%ecx
	cmpw     $0x8000,0x4f0(%ecx)
#;;   beq.b     upcom21                # [210]
	je       UPCOM21
#;;   move.w    (a4,d7.w),d0           # [211]
	movswl   d7_w,%ecx
	addl     a4_l,%ecx
	movw     0(%ecx),%bx
UPCOM16:
#;;   lea       (a5,d0.w),a2           # [212]
	movswl   %bx,%ecx
	addl     a5_l,%ecx
	lea      0(%ecx),%eax
	movl     %eax,a2_l
#;;   move.w    (a2),d1                # [213]
	movl     a2_l,%ecx
	movw     (%ecx),%dx
#;;   add.w     d4,d1                  # [214]
	addw     d4_w,%dx
#;;   move.w    d1,(a2)+               # [215]
	movw     %dx,0(%ecx)
#;;   cmp.w     (a2)+,d1               # [216 LS]
	addl     $4,a2_l
	cmpw     2(%ecx),%dx
#;;   bls.b     upcom20                # [217]
	jbe      UPCOM20
UPCOM17:
#;;   cmp.w     (a2)+,d1               # [218 HI]
	movl     a2_l,%ecx
	addl     $2,a2_l
	cmpw     (%ecx),%dx
#;;   bhi.b     upcom17                # [219]
	ja       UPCOM17
#;;   subq.l    #4,a2                  # [220]
	subl     $4,a2_l
#;;   move.l    a2,d2                  # [221]
	movl     a2_l,%eax
	movl     %eax,d2_l
#;;   sub.l     a5,d2                  # [222]
	movl     a5_l,%eax
	subl     %eax,d2_l
#;;   move.w    (a2),(a5,d0.w)         # [223]
	movl     a2_l,%ecx
	movw     (%ecx),%ax
	movswl   %bx,%ecx
	addl     a5_l,%ecx
	movw     %ax,0(%ecx)
#;;   move.w    d1,(a2)                # [224]
	movl     a2_l,%ecx
	movw     %dx,(%ecx)
#;;   move.w    (a3,d0.w),d1           # [225 MI]
	movswl   %bx,%ecx
	addl     a3_l,%ecx
	movw     0(%ecx),%dx
	testw    %dx,%dx
#;;   bmi.b     upcom18                # [226]
	js       UPCOM18
#;;   move.w    d2,2(a4,d1.w)          # [227]
	movw     d2_w,%ax
	movswl   %dx,%ecx
	addl     a4_l,%ecx
	movw     %ax,2(%ecx)
UPCOM18:
#;;   move.w    d2,(a4,d1.w)           # [228]
	movw     d2_w,%ax
	movswl   %dx,%ecx
	addl     a4_l,%ecx
	movw     %ax,0(%ecx)
#;;   move.w    (a3,d2.w),d3           # [229 MI]
	movswl   d2_w,%ecx
	addl     a3_l,%ecx
	movw     0(%ecx),%ax
	movw     %ax,d3_w
	testw    %ax,%ax
#;;   bmi.b     upcom19                # [230]
	js       UPCOM19
#;;   move.w    d0,2(a4,d3.w)          # [231]
	movswl   d3_w,%ecx
	addl     a4_l,%ecx
	movw     %bx,2(%ecx)
UPCOM19:
#;;   move.w    d1,(a3,d2.w)           # [232]
	movswl   d2_w,%ecx
	addl     a3_l,%ecx
	movw     %dx,0(%ecx)
#;;   move.w    d0,(a4,d3.w)           # [233]
	movswl   d3_w,%ecx
	addl     a4_l,%ecx
	movw     %bx,0(%ecx)
#;;   move.w    d3,(a3,d0.w)           # [234]
	movw     d3_w,%ax
	movswl   %bx,%ecx
	addl     a3_l,%ecx
	movw     %ax,0(%ecx)
#;;   move.w    (a4,d2.w),d0           # [235 NE]
	movswl   d2_w,%ecx
	addl     a4_l,%ecx
	movw     0(%ecx),%bx
	testw    %bx,%bx
#;;   bne.b     upcom16                # [236]
	jne      UPCOM16
#;;   bra.b     upcom21                # [237]
	jmp      UPCOM21
UPCOM20:
#;;   move.w    (a4,d0.w),d0           # [239 NE]
	movswl   %bx,%ecx
	addl     a4_l,%ecx
	movw     0(%ecx),%bx
	testw    %bx,%bx
#;;   bne.b     upcom16                # [240]
	jne      UPCOM16
UPCOM21:
#;;   cmp.w     #$fe00,d7              # [241 GE]
	cmpw     $0xfe00,d7_w
#;;   bge.w     upcom8                 # [242]
	jge      UPCOM8
#;;   cmp.w     #$fd86,d7              # [243 GT]
	cmpw     $0xfd86,d7_w
#;;   bgt.b     upcom23                # [244]
	jg       UPCOM23
UPCOM22:
#;;   bsr.w     upcom34                # [245]
	call     UPCOM34
#;;   moveq     #0,d4                  # [246]
	movl     $0,d4_l
#;;   rts                              # [247]
	ret      
UPCOM23:
#;;   rol.w     #8,d6                  # [249]
	rolw     $8,d6_w
#;;   subq.w    #7,d5                  # [250 CC]
	subw     $7,d5_w
#;;   bcc.b     upcom24                # [251]
	jnc      UPCOM24
#;;   moveq     #0,d3                  # [252]
	movl     $0,d3_l
#;;   move.l    d6,-(sp)               # [253]
	pushl    d6_l
#;;   bsr.w     upcom33                # [254]
	call     UPCOM33
#;;   move.w    d6,d3                  # [255]
	movw     d6_w,%ax
	movw     %ax,d3_w
#;;   move.l    (sp)+,d6               # [256]
	popl     d6_l
#;;   swap      d3                     # [257]
	roll     $16,d3_l
#;;   neg.w     d5                     # [258]
	negw     d5_w
#;;   rol.l     d5,d3                  # [259]
	movb     d5_b,%al
	movl     %eax,%ecx
	roll     %cl,d3_l
#;;   move.w    d3,d1                  # [260]
	movw     d3_w,%dx
#;;   or.b      d6,d1                  # [261]
	orb      d6_b,%dl
#;;   swap      d3                     # [262]
	roll     $16,d3_l
#;;   move.w    d3,d6                  # [263]
	movw     d3_w,%ax
	movw     %ax,d6_w
#;;   neg.w     d5                     # [264]
	negw     d5_w
#;;   moveq     #15,d2                 # [265]
	movl     $15,d2_l
#;;   add.w     d2,d5                  # [266]
	movw     d2_w,%ax
	addw     %ax,d5_w
#;;   bra.b     upcom25                # [267]
	jmp      UPCOM25
UPCOM24:
#;;   moveq     #0,d1                  # [269]
	movl     $0,%edx
#;;   move.b    d6,d1                  # [270]
	movb     d6_b,%dl
#;;   clr.b     d6                     # [271]
	movb     $0,d6_b
#;;   dbra      d5,upcom25             # [272]
	decw     d5_w
	cmpw     $-1,d5_w
	jne      UPCOM25
#;;   bsr.w     upcom33                # [273]
	call     UPCOM33
#;;   moveq     #15,d5                 # [274]
	movl     $15,d5_l
UPCOM25:
#;;   move.w    d1,d3                  # [275]
	movw     %dx,d3_w
#;;   add.w     d3,d3                  # [276]
	movw     d3_w,%ax
	addw     %ax,d3_w
#;;   pea       (a5)                   # [277]
	pushl    a5_l
#;;   move.l    -$14(a6),a5            # [278]
	movl     -0x14(%ebp),%eax
	movl     %eax,a5_l
#;;   lea       $1194(a5),a2           # [279]
	movl     a5_l,%ecx
	lea      0x1194(%ecx),%eax
#;;   move.l    (sp)+,a5               # [280]
	popl     a5_l
	movl     %eax,a2_l
#;;   move.w    (a2,d3.w),d3           # [281]
	movswl   d3_w,%ecx
	addl     a2_l,%ecx
	movw     0(%ecx),%ax
	movw     %ax,d3_w
#;;   clr.w     d2                     # [282]
	movw     $0,d2_w
#;;   move.b    d3,d2                  # [283]
	movb     d3_b,%al
	movb     %al,d2_b
#;;   clr.b     d3                     # [284]
	movb     $0,d3_b
#;;   lsr.w     #2,d3                  # [285]
	shrw     $2,d3_w
UPCOM26:
#;;   add.w     d6,d6                  # [286 X]
	movw     d6_w,%ax
	addw     %ax,d6_w
#;;   addx.w    d1,d1                  # [287]
	adcw     %dx,%dx
#;;   dbra      d5,upcom27             # [288]
	decw     d5_w
	cmpw     $-1,d5_w
	jne      UPCOM27
#;;   moveq     #15,d5                 # [289]
	movl     $15,d5_l
#;;   bsr.b     upcom33                # [290]
	call     UPCOM33
UPCOM27:
#;;   dbra      d2,upcom26             # [291]
	decw     d2_w
	cmpw     $-1,d2_w
	jne      UPCOM26
#;;   and.w     #$3f,d1                # [293]
	andw     $0x3f,%dx
#;;   or.w      d3,d1                  # [294]
	orw      d3_w,%dx
#;;   move.l    a1,d3                  # [295]
	movl     %edi,d3_l
#;;   sub.l     -12(a6),d3             # [296]
	movl     -12(%ebp),%eax
	subl     %eax,d3_l
#;;   add.l     #$1000,d3              # [297]
	addl     $0x1000,d3_l
#;;   sub.w     d1,d3                  # [298]
	subw     %dx,d3_w
#;;   and.l     #$fff,d3               # [299]
	andl     $0xfff,d3_l
#;;   add.l     -12(a6),d3             # [300]
	movl     -12(%ebp),%eax
	addl     %eax,d3_l
#;;   move.l    d3,a2                  # [301]
	movl     d3_l,%eax
	movl     %eax,a2_l
#;;   clr.l     d3                     # [302]
	movl     $0,d3_l
#;;   move.w    d7,d3                  # [303]
	movw     d7_w,%ax
	movw     %ax,d3_w
#;;   add.l     #$202,d3               # [304 EQ]
	addl     $0x202,d3_l
#;;   beq.w     upcom10                # [305]
	je       UPCOM10
#;;   neg.w     d3                     # [306]
	negw     d3_w
#;;   ror.w     #1,d3                  # [307]
	rorw     $1,d3_w
UPCOM28:
#;;   move.b    (a2)+,(a1)+            # [308]
	movl     a2_l,%ecx
	movb     (%ecx),%al
	addl     $1,a2_l
	stosb    
#;;   cmp.l     a2,a0                  # [309 GT]
	cmpl     a2_l,%esi
#;;   bgt.b     upcom29                # [310]
	jg       UPCOM29
#;;   move.l    -12(a6),a2             # [311]
	movl     -12(%ebp),%eax
	movl     %eax,a2_l
UPCOM29:
#;;   addq.l    #1,-8(a6)              # [312]
	addl     $1,-8(%ebp)
#;;   cmp.l     #$1000,-8(a6)          # [313 NE]
	cmpl     $0x1000,-8(%ebp)
#;;   bne.b     upcom30                # [314]
	jne      UPCOM30
#;;   bsr.b     upcom34                # [315]
	call     UPCOM34
UPCOM30:
#;;   subq.l    #1,-4(a6)              # [316]
	subl     $1,-4(%ebp)
#;;   tst.l     -4(a6)                 # [317 EQ]
	testl    $0xffffffff,-4(%ebp)
#;;   beq.w     upcom22                # [318]
	je       UPCOM22
#;;   dbra      d3,upcom28             # [319]
	decw     d3_w
	cmpw     $-1,d3_w
	jne      UPCOM28
#;;   bra.w     upcom10                # [320]
	jmp      UPCOM10
UPCOM31:
#;;   addq.w    #1,d1                  # [322]
	addw     $1,%dx
#;;   move.w    d4,d0                  # [323]
	movw     d4_w,%bx
UPCOM32:
#;;   move.b    d1,(a1)+               # [324]
	movb     %dl,0(%edi)
#;;   move.b    d2,(a1)+               # [325]
	movb     d2_b,%al
	movb     %al,1(%edi)
	lea      2(%edi),%edi
#;;   dbra      d0,upcom32             # [326]
	decw     %bx
	cmpw     $-1,%bx
	jne      UPCOM32
#;;   dbra      d3,upcom31             # [327]
	decw     d3_w
	cmpw     $-1,d3_w
	jne      UPCOM31
#;;   rts                              # [328]
	ret      
UPCOM33:
#;;   pea       (a0)                   # [330]
	pushl    %esi
#;;   move.l    -$1c(a6),a0            # [331]
	movl     -0x1c(%ebp),%esi
#;;   add.l     -$28(a6),a0            # [332]
	addl     -0x28(%ebp),%esi
#;;   addq.l    #2,-$28(a6)            # [333]
	addl     $2,-0x28(%ebp)
#;;   move.b    (a0),d6                # [334]
	movb     (%esi),%al
	movb     %al,d6_b
#;;   lsl.w     #8,d6                  # [335]
	shlw     $8,d6_w
#;;   move.b    1(a0),d6               # [336]
	movb     1(%esi),%al
	movb     %al,d6_b
#;;   move.l    (sp)+,a0               # [337]
	popl     %esi
#;;   rts                              # [338]
	ret      
UPCOM34:
#;;   pea       (a0)                   # [340]
	pushl    %esi
#;;   move.l    -8(a6),d0              # [341]
	movl     -8(%ebp),%ebx
#;;   sub.l     d0,a1                  # [342]
	subl     %ebx,%edi
#;;   subq.l    #1,d0                  # [343]
	subl     $1,%ebx
#;;   move.l    -$20(a6),a0            # [344]
	movl     -0x20(%ebp),%esi
UPCOM35:
#;;   cmp.l     -$34(a6),a0            # [345 EQ]
	cmpl     -0x34(%ebp),%esi
#;;   beq.b     upcom36                # [346]
	je       UPCOM36
#;;   move.b    (a1)+,(a0)+            # [347]
	movb     (%edi),%al
	movb     %al,(%esi)
	lea      1(%edi),%edi
	lea      1(%esi),%esi
#;;   dbra      d0,upcom35             # [348]
	decw     %bx
	cmpw     $-1,%bx
	jne      UPCOM35
#;;   move.l    a0,-$20(a6)            # [349]
	movl     %esi,-0x20(%ebp)
#;;   sub.l     -8(a6),a1              # [350]
	subl     -8(%ebp),%edi
#;;   clr.l     -8(a6)                 # [351]
	movl     $0,-8(%ebp)
#;;   move.l    (sp)+,a0               # [352]
	popl     %esi
#;;   rts                              # [353]
	ret      
UPCOM36:
#;;   move.l    -$3c(a6),sp            # [355]
	movl     -0x3c(%ebp),%esp
#;;   moveq     #2,d4                  # [356]
	movl     $2,d4_l
#;;   rts                              # [357]
	ret      
UPCOM37:
#;;   movem.l   d4-d7/a3,-(sp)         # [359]
	pushl    a3_l
	pushl    d7_l
	pushl    d6_l
	pushl    d5_l
	pushl    d4_l
#;;   move.l    -$2c(a5),a3            # [360]
	movl     a5_l,%ecx
	movl     -0x2c(%ecx),%eax
	movl     %eax,a3_l
#;;   moveq     #0,d6                  # [361]
	movl     $0,d6_l
UPCOM38:
#;;   cmp.l     #$100,d6               # [362 GE]
	cmpl     $0x100,d6_l
#;;   bge.b     upcom43                # [363]
	jge      UPCOM43
#;;   moveq     #0,d7                  # [364]
	movl     $0,d7_l
#;;   move.l    d6,d5                  # [365]
	movl     d6_l,%eax
	movl     %eax,d5_l
#;;   add.l     d5,d5                  # [366]
	movl     d5_l,%eax
	addl     %eax,d5_l
#;;   moveq     #8,d4                  # [367]
	movl     $8,d4_l
UPCOM39:
#;;   tst.l     d4                     # [368 LE]
	testl    $0xffffffff,d4_l
#;;   ble.b     upcom42                # [369]
	jle      UPCOM42
#;;   asr.l     #1,d5                  # [370]
	sarl     $1,d5_l
#;;   move.l    d5,d0                  # [371]
	movl     d5_l,%ebx
#;;   eor.l     d7,d0                  # [372]
	xorl     d7_l,%ebx
#;;   btst      #0,d0                  # [373 EQ]
	testl    $1<<((0)&31),%ebx
#;;   beq.b     upcom40                # [374]
	je       UPCOM40
#;;   move.l    d7,d0                  # [375]
	movl     d7_l,%ebx
#;;   lsr.l     #1,d0                  # [376]
	shrl     $1,%ebx
#;;   move.l    d0,d7                  # [377]
	movl     %ebx,d7_l
#;;   eor.l     #$edb88320,d7          # [378]
	xorl     $0xedb88320,d7_l
#;;   bra.b     upcom41                # [379]
	jmp      UPCOM41
UPCOM40:
#;;   lsr.l     #1,d7                  # [381]
	shrl     $1,d7_l
UPCOM41:
#;;   subq.l    #1,d4                  # [382]
	subl     $1,d4_l
#;;   bra.b     upcom39                # [383]
	jmp      UPCOM39
UPCOM42:
#;;   move.l    d6,d0                  # [385]
	movl     d6_l,%ebx
#;;   asl.l     #2,d0                  # [386]
	shll     $2,%ebx
#;;   move.l    d7,(a3,d0.l)           # [387]
	movl     d7_l,%eax
	movl     a3_l,%ecx
	movl     %eax,0(%ecx,%ebx)
#;;   addq.w    #1,d6                  # [388]
	addw     $1,d6_w
#;;   bra.b     upcom38                # [389]
	jmp      UPCOM38
UPCOM43:
#;;   movem.l   (sp)+,d4-d7/a3         # [391]
	popl     d4_l
	popl     d5_l
	popl     d6_l
	popl     d7_l
	popl     a3_l
#;;   rts                              # [392]
	ret      
UPCOM44:
#;;   movem.l   d2/d4-d7/a2,-(sp)      # [394]
	pushl    a2_l
	pushl    d7_l
	pushl    d6_l
	pushl    d5_l
	pushl    d4_l
	pushl    d2_l
#;;   move.l    -$2c(a5),a2            # [395]
	movl     a5_l,%ecx
	movl     -0x2c(%ecx),%eax
	movl     %eax,a2_l
#;;   moveq     #0,d5                  # [396]
	movl     $0,d5_l
#;;   moveq     #0,d7                  # [397]
	movl     $0,d7_l
UPCOM45:
#;;   cmp.l     d0,d7                  # [398 GE]
	cmpl     %ebx,d7_l
#;;   bge.b     upcom46                # [399]
	jge      UPCOM46
#;;   moveq     #0,d2                  # [400]
	movl     $0,d2_l
#;;   move.b    (a0,d7.l),d2           # [401]
	movl     d7_l,%ecx
	movb     0(%esi,%ecx),%al
	movb     %al,d2_b
#;;   eor.l     d5,d2                  # [402]
	movl     d5_l,%eax
	xorl     %eax,d2_l
#;;   moveq     #0,d1                  # [403]
	movl     $0,%edx
#;;   not.b     d1                     # [404]
	notb     %dl
#;;   and.l     d1,d2                  # [405]
	andl     %edx,d2_l
#;;   asl.l     #2,d2                  # [406]
	shll     $2,d2_l
#;;   move.l    d5,d6                  # [407]
	movl     %eax,d6_l
#;;   lsr.l     #8,d6                  # [408]
	shrl     $8,d6_l
#;;   and.l     #$00ffffff,d6          # [409]
	andl     $0xffffff,d6_l
#;;   move.l    (a2,d2.l),d4           # [410]
	movl     d2_l,%ecx
	addl     a2_l,%ecx
	movl     0(%ecx),%eax
	movl     %eax,d4_l
#;;   eor.l     d6,d4                  # [411]
	movl     d6_l,%eax
	xorl     %eax,d4_l
#;;   move.l    d4,d5                  # [412]
	movl     d4_l,%eax
	movl     %eax,d5_l
#;;   addq.l    #1,d7                  # [413]
	addl     $1,d7_l
#;;   bra.b     upcom45                # [414]
	jmp      UPCOM45
UPCOM46:
#;;   move.l    d5,d0                  # [416]
	movl     d5_l,%ebx
#;;   movem.l   (sp)+,d2/d4-d7/a2      # [417]
	popl     d2_l
	popl     d4_l
	popl     d5_l
	popl     d6_l
	popl     d7_l
	popl     a2_l
#;;   rts                              # [418]
	movl %ebx,%eax
	ret      
