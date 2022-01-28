#;------------------------------------------------------------------------------
#; Spike Decruncher
#;
#;a0 in
#;a1 out
#;a2 out + outsz

    .text
    .globl  USPIKE
    .globl  _USPIKE
USPIKE:
_USPIKE:
    movl 4(%esp),%esi
    movl 8(%esp),%edi
    movl 12(%esp),%eax
    movl %eax,a2_l




#;;   moveq     #-1,d5                 # [11]
	movl     $-1,d5_l
#;;   moveq     #-$80,d3               # [12]
	movl     $-0x80,d3_l
USPIKE1:
#;;   bsr.b     uspik10                # [13 (CC)]
	call     USPIK10
#;;   bcc.b     uspike4                # [14]
	jnc      USPIKE4
#;;   moveq     #0,d2                  # [15]
	movl     $0,d2_l
#;;   bsr.b     uspik10                # [16 (CC)]
	call     USPIK10
#;;   bcc.b     uspike3                # [17]
	jnc      USPIKE3
#;;   moveq     #3,d0                  # [18]
	movl     $3,%ebx
#;;   moveq     #8,d2                  # [19]
	movl     $8,d2_l
USPIKE2:
#;;   subq.w    #2,d2                  # [20]
	subw     $2,d2_w
#;;   move.b    (a5,d0.w),d4           # [21]
	movswl   %bx,%ecx
	addl     a5_l,%ecx
	movb     0(%ecx),%al
	movb     %al,d4_b
#;;   bsr.b     uspik12                # [22]
	call     USPIK12
#;;   cmp.w     4(a5,d2.w),d1          # [23 NE]
	movswl   d2_w,%ecx
	addl     a5_l,%ecx
	cmpw     4(%ecx),%dx
#;;   dbne      d0,uspike2             # [24]
	jne      _PA_13_
	decw     %bx
	cmpw     $-1,%bx
	jne      USPIKE2
_PA_13_:         
#;;   move.b    12(a5,d0.w),d2         # [25]
	movswl   %bx,%ecx
	addl     a5_l,%ecx
	movb     12(%ecx),%al
	movb     %al,d2_b
#;;   add.w     d1,d2                  # [26]
	addw     %dx,d2_w
USPIKE3:
#;;   move.b    (a0)+,(a1)+            # [27]
	movsb    
#;;   cmp.l     a2,a1                  # [28 EQ]
	cmpl     a2_l,%edi
#;;   dbeq      d2,uspike3             # [29]
	je       _PA_18_
	decw     d2_w
	cmpw     $-1,d2_w
	jne      USPIKE3
_PA_18_:         
#;;   cmp.w     d5,d2                  # [30 NE]
	movw     d5_w,%ax
	cmpw     %ax,d2_w
#;;   bne.b     uspike9                # [31]
	jne      USPIKE9
USPIKE4:
#;;   moveq     #4,d0                  # [33]
	movl     $4,%ebx
USPIKE5:
#;;   bsr.b     uspik10                # [34 (CC)]
	call     USPIK10
#;;   bcc.b     uspike6                # [35]
	jnc      USPIKE6
#;;   subq.w    #1,d0                  # [36 NE]
	subw     $1,%bx
#;;   bne.b     uspike5                # [37]
	jne      USPIKE5
USPIKE6:
#;;   clr.w     d1                     # [38]
	xorw     %dx,%dx
#;;   move.b    16(a5,d0.w),d4         # [39 MI]
	movswl   %bx,%ecx
	addl     a5_l,%ecx
	movb     16(%ecx),%al
	movb     %al,d4_b
	testb    %al,%al
#;;   bmi.b     uspike7                # [40]
	js       USPIKE7
#;;   bsr.b     uspik12                # [41]
	call     USPIK12
USPIKE7:
#;;   move.b    21(a5,d0.w),d0         # [42]
	movswl   %bx,%ecx
	addl     a5_l,%ecx
	movb     21(%ecx),%bl
#;;   add.w     d1,d0                  # [43]
	addw     %dx,%bx
#;;   moveq     #2,d4                  # [44]
	movl     $2,d4_l
#;;   bsr.b     uspik12                # [45]
	call     USPIK12
#;;   move.l    d1,d2                  # [46]
	movl     %edx,d2_l
#;;   move.b    26(a5,d2.w),d4         # [47]
	movswl   d2_w,%ecx
	addl     a5_l,%ecx
	movb     26(%ecx),%al
	movb     %al,d4_b
#;;   add.w     d2,d2                  # [48]
	movw     d2_w,%ax
	addw     %ax,d2_w
#;;   bsr.b     uspik12                # [49]
	call     USPIK12
#;;   add.w     34(a5,d2.w),d1         # [50]
	movswl   d2_w,%ecx
	addl     a5_l,%ecx
	addw     34(%ecx),%dx
#;;   move.l    a1,a3                  # [51]
	movl     %edi,a3_l
#;;   sub.l     d1,a3                  # [52]
	subl     %edx,a3_l
USPIKE8:
#;;   move.b    (a3)+,(a1)+            # [53]
	movl     a3_l,%ecx
	movb     (%ecx),%al
	addl     $1,a3_l
	stosb    
#;;   cmp.l     a2,a1                  # [54 EQ]
	cmpl     a2_l,%edi
#;;   dbeq      d0,uspike8             # [55]
	je       _PA_43_
	decw     %bx
	cmpw     $-1,%bx
	jne      USPIKE8
_PA_43_:         
#;;   cmp.w     d5,d0                  # [56 EQ]
	cmpw     d5_w,%bx
#;;   beq.b     uspike1                # [57]
	je       USPIKE1
USPIKE9:
#;;   rts                              # [58]
	ret      
USPIK10:
#;;   add.b     d3,d3                  # [60 CC NE X]
	movb     d3_b,%al
	addb     %al,d3_b
	setcb    xflag
#;;   bne.b     uspik11                # [61]
	jne      USPIK11
#;;   move.b    (a0)+,d3               # [62]
	lodsb    
	movb     %al,d3_b
#;;   addx.b    d3,d3                  # [63 CC X]
	movb     d3_b,%al
	btw      $0,xflag
	adcb     %al,d3_b
	setcb    xflag
USPIK11:
#;;   rts                              # [64]
	ret      
USPIK12:
#;;   moveq     #0,d1                  # [66]
	movl     $0,%edx
USPIK13:
#;;   bsr.b     uspik10                # [67 (X)]
	call     USPIK10
	setcb    xflag
#;;   addx.w    d1,d1                  # [68]
	btw      $0,xflag
	adcw     %dx,%dx
#;;   subq.b    #1,d4                  # [69 PL]
	subb     $1,d4_b
#;;   bpl.b     uspik13                # [70]
	jns      USPIK13
#;;   rts                              # [71]
	ret      
USPIK20:
#;;   moveq     #0,d1                  # [75]
	movl     $0,%edx
USPIK21:
#;;   move.b    (a1)+,d1               # [76]
	movb     (%edi),%dl
	lea      1(%edi),%edi
#;;   cmp.w     d7,d1                  # [77 NE]
	cmpw     d7_w,%dx
#;;   bne.b     uspik23                # [78]
	jne      USPIK23
#;;   moveq     #0,d2                  # [79]
	movl     $0,d2_l
#;;   move.b    (a1)+,d2               # [80 EQ]
	movb     (%edi),%al
	movb     %al,d2_b
	testb    %al,%al
	lea      1(%edi),%edi
#;;   beq.b     uspik23                # [81]
	je       USPIK23
#;;   addq.w    #1,d2                  # [82]
	addw     $1,d2_w
#;;   move.b    (a1)+,d1               # [83]
	movb     (%edi),%dl
	lea      1(%edi),%edi
USPIK22:
#;;   move.b    d1,(a0)+               # [84]
	movb     %dl,(%esi)
	lea      1(%esi),%esi
#;;   cmp.l     a2,a0                  # [85 EQ]
	cmpl     a2_l,%esi
#;;   dbeq      d2,uspik22             # [86]
	je       _PA_69_
	decw     d2_w
	cmpw     $-1,d2_w
	jne      USPIK22
_PA_69_:         
#;;   tst.w     d2                     # [87 PL]
	testw    $0xffff,d2_w
#;;   bpl.b     uspik24                # [88]
	jns      USPIK24
USPIK23:
#;;   move.b    d1,(a0)+               # [89]
	movb     %dl,(%esi)
	lea      1(%esi),%esi
#;;   cmp.l     a0,a2                  # [90 NE]
	cmpl     %esi,a2_l
#;;   bne.b     uspik21                # [91]
	jne      USPIK21
USPIK24:
#;;   rts                              # [92]
	ret      
