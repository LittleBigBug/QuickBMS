#;------------------------------------------------------------------------------
#; TUC Decruncher Routine
#;
#; a5 in
#; UI_DecrunchLen
#; UI_DecrunchAdr
    .text
    .globl UTUC
    .globl _UTUC
UTUC:
_UTUC:
    movl 4(%esp),%eax
    movl %eax,a5_l
    
    movl 8(%esp),%eax
    movl %eax,UI_DecrunchAdr
    
    movl 12(%esp),%eax
    movl %eax,UI_DecrunchLen


#;;   moveq     #0,d7                  # [8]
	movl     $0,d7_l
#;;   move.b    (a5)+,d7               # [9]
	movl     a5_l,%ecx
	movb     0(%ecx),%al
	movb     %al,d7_b
#;;   swap      d7                     # [10]
	roll     $16,d7_l
#;;   move.b    (a5)+,d7               # [11]
	movb     1(%ecx),%al
	addl     $2,a5_l
	movb     %al,d7_b
#;;   move.l    #"rich",d3             # [12]
	#movl     $<Bad expression>,d3_l
    movl $0,d3_l
#;;   move.w    d7,d0                  # [13]
	movw     d7_w,%bx
#;;   addq.w    #1,d0                  # [14]
	addw     $1,%bx
#;;   and.w     #$fe,d0                # [15]
	andw     $0xfe,%bx
#;;   lea       (a5,d0.w),a1           # [16]
	movswl   %bx,%ecx
	addl     a5_l,%ecx
	lea      0(%ecx),%edi
#;;   move.w    d7,d0                  # [17]
	movw     d7_w,%bx
#;;   lsl.w     #2,d0                  # [18]
	shlw     $2,%bx
#;;   lea       $10(a1,d0.w),a2        # [19]
	movswl   %bx,%ecx
	lea      0x10(%edi,%ecx),%eax
	movl     %eax,a2_l
#;;   move.l    (a2)+,d6               # [20]
	movl     a2_l,%ecx
	movl     (%ecx),%eax
	addl     $4,a2_l
	movl     %eax,d6_l
#;;   eor.l     d6,d3                  # [21]
	movl     d6_l,%eax
	xorl     %eax,d3_l
#;;   swap      d6                     # [22]
	roll     $16,d6_l
#;;   neg.l     d6                     # [23]
	negl     d6_l
#;;   eor.l     #$9c978d97,d6          # [24]
	xorl     $0x9c978d97,d6_l
#;;   movem.l   d3/d6-d7/a1-a2/a5,-(sp) # [26]
	pushl    a5_l
	pushl    a2_l
	pushl    %edi
	pushl    d7_l
	pushl    d6_l
	pushl    d3_l
#;;   sub.l     a0,a0                  # [27]
	subl     %esi,%esi
#;;   st        UI_Temp(a4)            # [28]
	movl     a4_l,%ecx
	movb     $0xff,UI_Temp
#;;   bsr.b     utuc1                  # [29]
	call     UTUC1
#;;   clr.l     UI_Temp(a4)            # [30]
	movl     a4_l,%ecx
	movl     $0,UI_Temp
#;;   movem.l   (sp)+,d3/d6-d7/a1-a2/a5 # [31]
	popl     d3_l
	popl     d6_l
	popl     d7_l
	popl     %edi
	popl     a2_l
	popl     a5_l
#;;   move.l    a0,d0                  # [33]
	movl     %esi,%ebx
#;;   move.l    d0,UI_DecrunchLen(a4)  # [34]
	movl     %ebx,UI_DecrunchLen
#;;   bsr.w     alcdmemj               # [35]
    push %ebx
	call     _malloc
#;;   move.l    d0,UI_DecrunchAdr(a4)  # [36 EQ]
	movl     a4_l,%ecx
	movl     %ebx,UI_DecrunchAdr
	testl    %ebx,%ebx
#;;   beq.b     utuco                  # [37]
	je       UTUCO
#;;   move.l    d0,a3                  # [38]
	movl     %ebx,a3_l
#;;   bsr.b     utuc1                  # [39]
	call     UTUC1
UTUCO:
#;;   rts                              # [40]
	ret      
UTUC1:
#;;   moveq     #0,d5                  # [42]
	movl     $0,d5_l
#;;   move.w    (a2)+,d0               # [43 EQ]
	movl     a2_l,%ecx
	addl     $2,a2_l
	movw     (%ecx),%bx
	testw    %bx,%bx
#;;   beq.w     utuc12                 # [44]
	je       UTUC12
#;;   move.l    a2,a6                  # [45]
	movl     a2_l,%ebp
#;;   lea       (a2,d0.w),a2           # [46]
	movswl   %bx,%ecx
	addl     a2_l,%ecx
	lea      0(%ecx),%eax
#;;   move.w    d6,d0                  # [47]
	movw     d6_w,%bx
#;;   moveq     #0,d1                  # [48]
	movl     $0,%edx
#;;   move.b    (a5,d0.w),d1           # [49]
	movswl   %bx,%ecx
	addl     a5_l,%ecx
	movb     0(%ecx),%dl
	movl     %eax,a2_l
#;;   bsr.w     utuc13                 # [50]
	call     UTUC13
#;;   move.l    d0,d4                  # [51]
	movl     %ebx,d4_l
#;;   add.l     a3,d4                  # [52]
	movl     a3_l,%eax
	addl     %eax,d4_l
UTUC2:
#;;   moveq     #0,d0                  # [54]
	movl     $0,%ebx
UTUC3:
#;;   asl.l     #1,d5                  # [55 CC NE]
	shll     $1,d5_l
#;;   bne.b     utuc4                  # [56]
	jne      UTUC4
#;;   move.l    (a2)+,d5               # [57]
	movl     a2_l,%ecx
	movl     (%ecx),%eax
	addl     $4,a2_l
	movl     %eax,d5_l
#;;   eor.l     d3,d5                  # [58]
	movl     d3_l,%eax
	xorl     %eax,d5_l
#;;   move.w    #$10,ccr               # [59 X]
	movb     $((0x10)>>4)&1,xflag
#;;   roxl.l    #1,d5                  # [60 CC]
	btw      $0,xflag
	rcll     $1,d5_l
UTUC4:
#;;   bcc.b     utuc5                  # [61]
	jnc      UTUC5
#;;   addq.w    #2,d0                  # [62]
	addw     $2,%bx
UTUC5:
#;;   move.w    (a6,d0.w),d0           # [63 PL]
	movswl   %bx,%ecx
	movw     0(%ebp,%ecx),%bx
	testw    %bx,%bx
#;;   bpl.b     utuc3                  # [64]
	jns      UTUC3
#;;   not.w     d0                     # [65]
	notw     %bx
#;;   cmp.w     #$100,d0               # [66 CS NE]
	cmpw     $0x100,%bx
#;;   bcs.w     utuc10                 # [67]
	jc       UTUC10
#;;   bne.b     utuc6                  # [68]
	jne      UTUC6
#;;   move.l    d7,d1                  # [69]
	movl     d7_l,%edx
#;;   swap      d1                     # [70]
	roll     $16,%edx
#;;   bsr.w     utuc13                 # [71]
	call     UTUC13
#;;   moveq     #0,d1                  # [72]
	movl     $0,%edx
#;;   move.b    (a5,d0.w),d1           # [73]
	movswl   %bx,%ecx
	addl     a5_l,%ecx
	movb     0(%ecx),%dl
#;;   asl.w     #2,d0                  # [74]
	shlw     $2,%bx
#;;   move.l    (a1,d0.w),d2           # [75]
	movswl   %bx,%ecx
	movl     0(%edi,%ecx),%eax
	movl     %eax,d2_l
#;;   bsr.w     utuc13                 # [76]
	call     UTUC13
#;;   add.l     d2,d0                  # [77]
	addl     d2_l,%ebx
#;;   tst.b     UI_Temp(a4)            # [78 NE]
	movl     a4_l,%ecx
	testb    $0xff,UI_Temp
#;;   bne.b     utuc5a                 # [79]
	jne      UTUC5a
#;;   move.l    d0,(a3)+               # [80]
	movl     a3_l,%ecx
	addl     $4,a3_l
	movl     %ebx,(%ecx)
#;;   bra.w     utuc11                 # [81]
	jmp      UTUC11
UTUC5a:
#;;   addq.l    #4,a0                  # [83]
	lea      4(%esi),%esi
#;;   addq.l    #4,a3                  # [84]
	addl     $4,a3_l
#;;   bra.w     utuc11                 # [85]
	jmp      UTUC11
UTUC6:
#;;   cmp.w     #$110,d0               # [87 HI]
	cmpw     $0x110,%bx
#;;   bhi.b     utuc8                  # [88]
	ja       UTUC8
#;;   sub.w     #$101,d0               # [89]
	subw     $0x101,%bx
#;;   move.w    d0,d1                  # [90]
	movw     %bx,%dx
#;;   bsr.w     utuc13                 # [91]
	call     UTUC13
#;;   move.w    d0,d2                  # [92]
	movw     %bx,d2_w
#;;   moveq     #7,d1                  # [93]
	movl     $7,%edx
#;;   bsr.w     utuc13                 # [94]
	call     UTUC13
#;;   tst.b     UI_Temp(a4)            # [95 NE]
	movl     a4_l,%ecx
	testb    $0xff,UI_Temp
#;;   bne.b     utuc7b                 # [96]
	jne      UTUC7b
UTUC7:
#;;   move.b    d0,(a3)+               # [97]
	movl     a3_l,%ecx
	addl     $1,a3_l
	movb     %bl,(%ecx)
#;;   cmp.l     d4,a3                  # [98 EQ]
	movl     d4_l,%eax
	cmpl     %eax,a3_l
#;;   dbeq      d2,utuc7               # [99]
	je       _PA_85_
	decw     d2_w
	cmpw     $-1,d2_w
	jne      UTUC7
_PA_85_:         
#;;   cmp.w     #-1,d2                 # [100 EQ]
	cmpw     $-1,d2_w
#;;   beq.b     utuc10                 # [101]
	je       UTUC10
UTUC7a:
#;;   move.w    #UERR_Corrupt,UI_ErrorNum(a4) # [102]
	movl     a4_l,%ecx
	movw     $-1,UI_ErrorNum
#;;   moveq     #0,d0                  # [103]
	movl     $0,%ebx
#;;   rts                              # [104]
	movl %ebx,%eax
	ret      
UTUC7b:
#;;   addq.l    #1,a0                  # [106]
	lea      1(%esi),%esi
#;;   addq.l    #1,a3                  # [107]
	addl     $1,a3_l
#;;   cmp.l     d4,a3                  # [108 EQ]
	movl     d4_l,%eax
	cmpl     %eax,a3_l
#;;   dbeq      d2,utuc7b              # [109]
	je       _PA_94_
	decw     d2_w
	cmpw     $-1,d2_w
	jne      UTUC7b
_PA_94_:         
#;;   cmp.w     #-1,d2                 # [110 EQ]
	cmpw     $-1,d2_w
#;;   beq.b     utuc10                 # [111]
	je       UTUC10
#;;   bra.b     utuc7a                 # [112]
	jmp      UTUC7a
UTUC8:
#;;   move.w    d0,d2                  # [114]
	movw     %bx,d2_w
#;;   move.w    d0,d1                  # [115]
	movw     %bx,%dx
#;;   and.w     #15,d1                 # [116]
	andw     $15,%dx
#;;   bsr.b     utuc13                 # [117]
	call     UTUC13
#;;   addq.w    #3,d0                  # [118]
	addw     $3,%bx
#;;   move.w    d2,d1                  # [119]
	movw     d2_w,%dx
#;;   move.w    d0,d2                  # [120]
	movw     %bx,d2_w
#;;   lsr.w     #4,d1                  # [121]
	shrw     $4,%dx
#;;   and.w     #15,d1                 # [122]
	andw     $15,%dx
#;;   addq.w    #2,d1                  # [123]
	addw     $2,%dx
#;;   bsr.b     utuc13                 # [124]
	call     UTUC13
#;;   neg.l     d0                     # [125]
	negl     %ebx
#;;   tst.b     UI_Temp(a4)            # [126 NE]
	movl     a4_l,%ecx
	testb    $0xff,UI_Temp
#;;   bne.b     utuc9a                 # [127]
	jne      UTUC9a
UTUC9:
#;;   move.b    (a3,d0.l),(a3)+        # [128]
	movl     a3_l,%ecx
	movb     0(%ecx,%ebx),%al
	addl     $1,a3_l
	movb     %al,(%ecx)
#;;   cmp.l     d4,a3                  # [129 EQ]
	movl     d4_l,%eax
	cmpl     %eax,a3_l
#;;   dbeq      d2,utuc9               # [130]
	je       _PA_114_
	decw     d2_w
	cmpw     $-1,d2_w
	jne      UTUC9
_PA_114_:         
#;;   cmp.w     #-1,d2                 # [131 EQ]
	cmpw     $-1,d2_w
#;;   beq.b     utuc11                 # [132]
	je       UTUC11
#;;   bra.b     utuc7a                 # [133]
	jmp      UTUC7a
UTUC9a:
#;;   addq.l    #1,a0                  # [135]
	lea      1(%esi),%esi
#;;   addq.l    #1,a3                  # [136]
	addl     $1,a3_l
#;;   cmp.l     d4,a3                  # [137 EQ]
	movl     d4_l,%eax
	cmpl     %eax,a3_l
#;;   dbeq      d2,utuc9a              # [138]
	je       _PA_121_
	decw     d2_w
	cmpw     $-1,d2_w
	jne      UTUC9a
_PA_121_:         
#;;   cmp.w     #-1,d2                 # [139 EQ]
	cmpw     $-1,d2_w
#;;   beq.b     utuc11                 # [140]
	je       UTUC11
#;;   bra.b     utuc7a                 # [141]
	jmp      UTUC7a
UTUC10:
#;;   tst.b     UI_Temp(a4)            # [143 NE]
	movl     a4_l,%ecx
	testb    $0xff,UI_Temp
#;;   bne.b     utuc10a                # [144]
	jne      UTUC10a
#;;   move.b    d0,(a3)+               # [145]
	movl     a3_l,%ecx
	addl     $1,a3_l
	movb     %bl,(%ecx)
#;;   bra.b     utuc11                 # [146]
	jmp      UTUC11
UTUC10a:
#;;   addq.l    #1,a0                  # [148]
	lea      1(%esi),%esi
#;;   addq.l    #1,a3                  # [149]
	addl     $1,a3_l
UTUC11:
#;;   cmp.l     d4,a3                  # [150 NE]
	movl     d4_l,%eax
	cmpl     %eax,a3_l
#;;   bne.w     utuc2                  # [151]
	jne      UTUC2
UTUC12:
#;;   addq.w    #1,d6                  # [152]
	addw     $1,d6_w
#;;   cmp.w     d7,d6                  # [153 CS]
	movw     d7_w,%ax
	cmpw     %ax,d6_w
#;;   bcs.w     utuc1                  # [154]
	jc       UTUC1
#;;   moveq     #-1,d0                 # [155]
	movl     $-1,%ebx
#;;   rts                              # [156]
	movl %ebx,%eax
	ret      
UTUC13:
#;;   moveq     #0,d0                  # [158]
	movl     $0,%ebx
UTUC14:
#;;   asl.l     #1,d5                  # [159 NE X]
	shll     $1,d5_l
	setcb    xflag
#;;   bne.b     utuc15                 # [160]
	jne      UTUC15
#;;   move.l    (a2)+,d5               # [161]
	movl     a2_l,%ecx
	movl     (%ecx),%eax
	addl     $4,a2_l
	movl     %eax,d5_l
#;;   eor.l     d3,d5                  # [162]
	movl     d3_l,%eax
	xorl     %eax,d5_l
#;;   move.w    #$10,ccr               # [163 X]
	movb     $((0x10)>>4)&1,xflag
#;;   roxl.l    #1,d5                  # [164 X]
	btw      $0,xflag
	rcll     $1,d5_l
	setcb    xflag
UTUC15:
#;;   roxl.l    #1,d0                  # [165]
	btw      $0,xflag
	rcll     $1,%ebx
#;;   dbra      d1,utuc14              # [166]
	decw     %dx
	cmpw     $-1,%dx
	jne      UTUC14
#;;   rts                              # [167]
	ret      
