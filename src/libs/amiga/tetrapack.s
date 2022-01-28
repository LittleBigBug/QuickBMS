#;------------------------------------------------------------------------------
#; Tetra Decruncher Routine
#;   
#;a1  in
#;a0  in + insz
#;d0 & UI_DecrunchAdr out
#;UI_DecrunchLen
    .text
    .globl UTETR
    .globl _UTETR
UTETR:
_UTETR:
    movl 4(%esp),%edi   # a1
    
    movl 8(%esp),%esi   # a0
    
    movl 12(%esp),%ebx  # d0
    movl %ebx,UI_DecrunchAdr
    
    movl 16(%esp),%eax
    movl %eax,UI_DecrunchLen
    
    
    


#;;   moveq     #0,d6                  # [11]
	movl     $0,d6_l
#;;   cmp.w     #$4e71,(a1)            # [12 NE]
	cmpw     $0x4e71,(%edi)
#;;   bne.b     utetr0                 # [13]
	jne      UTETR0
#;;   moveq     #1,d6                  # [14]
	movl     $1,d6_l
UTETR0:
#;;   move.l    d0,a1                  # [16]
	movl     %ebx,%edi
#;;   add.l     d1,a1                  # [17]
	lea      (%edi,%edx),%edi
#;;   move.l    -(a0),a2               # [19]
	lea      -8(%esi),%esi
	movl     4(%esi),%eax
	movl     %eax,a2_l
#;;   add.l     a1,a2                  # [20]
	addl     %edi,a2_l
#;;   move.l    -(a0),d0               # [21]
	movl     0(%esi),%ebx
UTETR1:
#;;   bsr.b     utetr6                 # [22 (CS)]
	call     UTETR6
#;;   bcs.b     utetr8                 # [23]
	jc       UTETR8
#;;   move.w    a5,d1                  # [24]
	movw     a5_w,%dx
#;;   moveq     #1,d3                  # [25]
	movl     $1,d3_l
#;;   bsr.b     utetr6                 # [26 (CS)]
	call     UTETR6
#;;   bcs.b     utetr11                # [27]
	jc       UTETR11
#;;   moveq     #3,d1                  # [28]
	movl     $3,%edx
#;;   moveq     #0,d4                  # [29]
	movl     $0,d4_l
UTETR2:
#;;   bsr.b     utetr14                # [30]
	call     UTETR14
#;;   move.w    d2,d3                  # [31]
	movw     d2_w,%ax
	movw     %ax,d3_w
#;;   add.w     d4,d3                  # [32]
	movw     d4_w,%ax
	addw     %ax,d3_w
UTETR3:
#;;   moveq     #7,d1                  # [33]
	movl     $7,%edx
UTETR4:
#;;   bsr.b     utetr6                 # [34 (X)]
	call     UTETR6
	setcb    xflag
#;;   roxl.l    #1,d2                  # [35]
	btw      $0,xflag
	rcll     $1,d2_l
#;;   dbra      d1,utetr4              # [36]
	decw     %dx
	cmpw     $-1,%dx
	jne      UTETR4
#;;   move.b    d2,-(a2)               # [37]
	movb     d2_b,%al
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     %al,(%ecx)
#;;   cmp.l     a1,a2                  # [38 EQ]
	cmpl     %edi,a2_l
#;;   dbeq      d3,utetr3              # [39]
	je       _PA_26_
	decw     d3_w
	cmpw     $-1,d3_w
	jne      UTETR3
_PA_26_:         
#;;   bra.b     utetr13                # [40]
	jmp      UTETR13
UTETR5:
#;;   moveq     #7,d1                  # [42]
	movl     $7,%edx
#;;   moveq     #8,d4                  # [43]
	movl     $8,d4_l
#;;   bra.b     utetr2                 # [44]
	jmp      UTETR2
UTETR6:
#;;   lsr.l     #1,d0                  # [46 CS NE X]
	shrl     $1,%ebx
	setcb    xflag
#;;   bne.b     utetr7                 # [47]
	jne      UTETR7
#;;   move.l    -(a0),d0               # [48]
	lea      -4(%esi),%esi
	movl     (%esi),%ebx
#;;   move.w    #$0010,ccr             # [49 X]
	movb     $((0x10)>>4)&1,xflag
#;;   roxr.l    #1,d0                  # [50 CS X]
	btw      $0,xflag
	rcrl     $1,%ebx
	setcb    xflag
UTETR7:
#;;   rts                              # [51]
	ret      
UTETR8:
#;;   moveq     #2,d1                  # [53]
	movl     $2,%edx
#;;   bsr.b     utetr14                # [54]
	call     UTETR14
#;;   cmp.b     #2,d2                  # [55 LT]
	cmpb     $2,d2_b
#;;   blt.b     utetr9                 # [56]
	jl       UTETR9
#;;   cmp.b     #3,d2                  # [57 EQ]
	cmpb     $3,d2_b
#;;   beq.b     utetr5                 # [58]
	je       UTETR5
#;;   moveq     #8,d1                  # [59]
	movl     $8,%edx
#;;   bsr.b     utetr14                # [60]
	call     UTETR14
#;;   move.w    d2,d3                  # [61]
	movw     d2_w,%ax
	movw     %ax,d3_w
#;;   addq.w    #4,d3                  # [62]
	addw     $4,d3_w
#;;   move.w    a6,d1                  # [63]
	movw     %bp,%dx
#;;   bra.b     utetr11                # [64]
	jmp      UTETR11
UTETR9:
#;;   move.w    d5,d1                  # [66]
	movw     d5_w,%dx
#;;   move.w    d2,d3                  # [67]
	movw     d2_w,%ax
	movw     %ax,d3_w
#;;   tst.w     d6                     # [68 NE]
	testw    $0xffff,d6_w
#;;   bne.b     utetr10                # [69]
	jne      UTETR10
#;;   add.w     d2,d1                  # [70]
	addw     d2_w,%dx
UTETR10:
#;;   addq.w    #2,d3                  # [71]
	addw     $2,d3_w
UTETR11:
#;;   bsr.b     utetr14                # [72]
	call     UTETR14
UTETR12:
#;;   move.b    -1(a2,d2.w),-(a2)      # [73]
	subl     $1,a2_l
	movswl   d2_w,%ecx
	addl     a2_l,%ecx
	movb     -1(%ecx),%al
	movl     a2_l,%ecx
	movb     %al,(%ecx)
#;;   cmp.l     a1,a2                  # [74 EQ]
	cmpl     %edi,a2_l
#;;   dbeq      d3,utetr12             # [75]
	je       _PA_58_2
	decw     d3_w
	cmpw     $-1,d3_w
	jne      UTETR12
_PA_58_2:         
UTETR13:
#;;   cmp.w     #-1,d3                 # [76 EQ]
	cmpw     $-1,d3_w
#;;   beq.b     utetr1                 # [77]
	je       UTETR1
#;;   bra.b     utetr16                # [78]
	jmp      UTETR16
UTETR14:
#;;   subq.w    #1,d1                  # [80]
	subw     $1,%dx
#;;   moveq     #0,d2                  # [81]
	movl     $0,d2_l
UTETR15:
#;;   bsr.b     utetr6                 # [82 (X)]
	call     UTETR6
	setcb    xflag
#;;   roxl.l    #1,d2                  # [83]
	btw      $0,xflag
	rcll     $1,d2_l
#;;   dbra      d1,utetr15             # [84]
	decw     %dx
	cmpw     $-1,%dx
	jne      UTETR15
#;;   rts                              # [85]
	ret      
UTETR16:
#;;   move.l    UI_DecrunchAdr(a4),a0  # [87]
	movl     a4_l,%ecx
	movl     UI_DecrunchAdr,%esi
#;;   move.l    a0,a2                  # [88]
	movl     %esi,a2_l
#;;   add.l     UI_DecrunchLen(a4),a2  # [89]
	movl     UI_DecrunchLen,%eax
	addl     %eax,a2_l
UTETR17:
#;;   move.b    (a1)+,d0               # [90]
	movb     (%edi),%bl
	lea      1(%edi),%edi
#;;   cmp.b     d7,d0                  # [91 NE]
	cmpb     d7_b,%bl
#;;   bne.b     utetr19                # [92]
	jne      UTETR19
#;;   moveq     #0,d1                  # [93]
	movl     $0,%edx
#;;   move.b    (a1)+,d1               # [94 EQ]
	movb     (%edi),%dl
	testb    %dl,%dl
	lea      1(%edi),%edi
#;;   beq.b     utetr19                # [95]
	je       UTETR19
#;;   move.b    (a1)+,d0               # [96]
	movb     (%edi),%bl
	lea      1(%edi),%edi
#;;   addq.w    #1,d1                  # [97]
	addw     $1,%dx
UTETR18:
#;;   move.b    d0,(a0)+               # [98]
	movb     %bl,(%esi)
	lea      1(%esi),%esi
#;;   cmp.l     a2,a0                  # [99 EQ]
	cmpl     a2_l,%esi
#;;   dbeq      d1,utetr18             # [100]
	je       _PA_81_
	decw     %dx
	cmpw     $-1,%dx
	jne      UTETR18
_PA_81_:         
#;;   cmp.w     #-1,d1                 # [101 NE]
	cmpw     $-1,%dx
#;;   bne.b     utetr20                # [102]
	jne      UTETR20
UTETR19:
#;;   move.b    d0,(a0)+               # [103]
	movb     %bl,(%esi)
	lea      1(%esi),%esi
#;;   cmp.l     a2,a0                  # [104 NE]
	cmpl     a2_l,%esi
#;;   bne.b     utetr17                # [105]
	jne      UTETR17
UTETR20:
#;;   moveq     #-1,d0                 # [107]
	movl     $-1,%ebx
UTETRAO:
#;;   rts                              # [108]
	movl %ebx,%eax
	ret      
