#;------------------------------------------------------------------------------
#; Unpacking Routine
#;
#; IN :	A0 = Crunch Address
#;	A5 = Length Address
#;
#; OUT:	D0 = Success (0=Error)
#;
    .text
    .globl UCMAT
    .globl _UCMAT
UCMAT:
_UCMAT:
    movl 4(%esp),%eax
    movl %eax, UI_DecrunchAdr

    movl 8(%esp),%eax
    movl %eax,UI_DecrunchLen
    
    movl 12(%esp),%eax
    movl %eax,a5_l



#;;   move.l    UI_DecrunchAdr,d0      # [15]
	movl     UI_DecrunchAdr,%ebx
#;;   move.l    d0,a1                  # [17]
	movl     %ebx,%edi
#;;   move.l    d0,a2                  # [18]
	movl     %ebx,a2_l
#;;   add.l     UI_DecrunchLen,a2      # [19]
	movl     UI_DecrunchLen,%eax
	addl     %eax,a2_l
#;;   move.l    d0,a6                  # [20]
	movl     %ebx,%ebp
#;;   add.l     UI_DecrunchLen,a6      # [21]
	addl     UI_DecrunchLen,%ebp
#;;   pea       ucmat13(pc)            # [23]
	lea      UCMAT13,%eax
	pushl    %eax
#;;   moveq     #-$80,d3               # [24]
	movl     $-0x80,d3_l
UCMAT1:
#;;   bsr.w     ucmat4                 # [25 (CC)]
	call     UCMAT4
#;;   bcc.b     ucmat8                 # [26]
	jnc      UCMAT8
#;;   moveq     #0,d2                  # [27]
	movl     $0,d2_l
#;;   bsr.w     ucmat4                 # [28 (CC)]
	call     UCMAT4
#;;   bcc.b     ucmat3                 # [29]
	jnc      UCMAT3
#;;   moveq     #3,d0                  # [30]
	movl     $3,%ebx
#;;   moveq     #8,d2                  # [31]
	movl     $8,d2_l
UCMAT2:
#;;   subq.w    #2,d2                  # [32]
	subw     $2,d2_w
#;;   move.b    ucmatt1(pc,d0.w),d4    # [33]
	movswl   %bx,%ecx
	movb     UCMATT1(%ecx),%al
	movb     %al,d4_b
#;;   bsr.w     ucmat6                 # [34]
	call     UCMAT6
#;;   cmp.w     ucmatt2(pc,d2.w),d1    # [35 NE]
	movswl   d2_w,%ecx
	cmpw     UCMATT2(%ecx),%dx
#;;   dbne      d0,ucmat2              # [36]
	jne      _PA_20_
	decw     %bx
	cmpw     $-1,%bx
	jne      UCMAT2
_PA_20_:         
#;;   move.b    ucmatt3(pc,d0.w),d2    # [37]
	movswl   %bx,%ecx
	movb     UCMATT3(%ecx),%al
	movb     %al,d2_b
#;;   add.w     d1,d2                  # [38]
	addw     %dx,d2_w
UCMAT3:
#;;   cmp.l     a6,a1                  # [39 EQ]
	cmpl     %ebp,%edi
#;;   beq.w     ucmate                 # [40]
	je       UCMATE
#;;   move.b    (a0)+,(a1)+            # [41]
	movsb    
#;;   dbra      d2,ucmat3              # [42]
	decw     d2_w
	cmpw     $-1,d2_w
	jne      UCMAT3
#;;   cmp.l     a1,a2                  # [43 LE]
	cmpl     %edi,a2_l
#;;   ble.b     ucmato                 # [44]
	jle      UCMATO
UCMAT8:
#;;   moveq     #4,d0                  # [46]
	movl     $4,%ebx
UCMAT9:
#;;   bsr.b     ucmat4                 # [47 (CC)]
	call     UCMAT4
#;;   bcc.b     ucmat10                # [48]
	jnc      UCMAT10
#;;   subq.w    #1,d0                  # [49 NE]
	subw     $1,%bx
#;;   bne.b     ucmat9                 # [50]
	jne      UCMAT9
UCMAT10:
#;;   clr.w     d1                     # [51]
	xorw     %dx,%dx
#;;   move.b    ucmatt4(pc,d0.w),d4    # [52 MI]
	movswl   %bx,%ecx
	movb     UCMATT4(%ecx),%al
	movb     %al,d4_b
	testb    %al,%al
#;;   bmi.b     ucmat11                # [53]
	js       UCMAT11
#;;   bsr.b     ucmat6                 # [54]
	call     UCMAT6
UCMAT11:
#;;   move.b    ucmatt5(pc,d0.w),d0    # [55]
	movswl   %bx,%ecx
	movb     UCMATT5(%ecx),%bl
#;;   add.w     d1,d0                  # [56]
	addw     %dx,%bx
#;;   moveq     #2,d4                  # [57]
	movl     $2,d4_l
#;;   bsr.b     ucmat6                 # [58]
	call     UCMAT6
#;;   move.l    d1,d2                  # [59]
	movl     %edx,d2_l
#;;   move.b    ucmatt6(pc,d2.w),d4    # [60]
	movswl   d2_w,%ecx
	movb     UCMATT6(%ecx),%al
	movb     %al,d4_b
#;;   add.w     d2,d2                  # [61]
	movw     d2_w,%ax
	addw     %ax,d2_w
#;;   bsr.b     ucmat6                 # [62]
	call     UCMAT6
#;;   add.w     ucmatt7(pc,d2.w),d1    # [63]
	movswl   d2_w,%ecx
	addw     UCMATT7(%ecx),%dx
#;;   move.l    a1,a3                  # [64]
	movl     %edi,a3_l
#;;   sub.l     d1,a3                  # [65]
	subl     %edx,a3_l
UCMAT12:
#;;   cmp.l     a6,a1                  # [66 EQ]
	cmpl     %ebp,%edi
#;;   beq.b     ucmate                 # [67]
	je       UCMATE
#;;   move.b    (a3)+,(a1)+            # [68]
	movl     a3_l,%ecx
	movb     (%ecx),%al
	addl     $1,a3_l
	stosb    
#;;   dbra      d0,ucmat12             # [69]
	decw     %bx
	cmpw     $-1,%bx
	jne      UCMAT12
#;;   cmp.l     a1,a2                  # [70 GT]
	cmpl     %edi,a2_l
#;;   bgt.b     ucmat1                 # [71]
	jg       UCMAT1
UCMATO:
#;;   rts                              # [72]
	ret      
	.data
	.p2align	2
#;;   UCMATT1 dc.b    9,2,1,0          # [74]
UCMATT1:
	.byte	9,2,1,0
#;;   UCMATT2 dc.w    $3ff,7,3,1       # [75]
UCMATT2:
	.short	0x3ff,7,3,1
#;;   UCMATT3 dc.b    12,5,2,1         # [76]
UCMATT3:
	.byte	12,5,2,1
#;;   UCMATT4 dc.b    9,1,0,-1,-1      # [77]
UCMATT4:
	.byte	9,1,0,-1,-1
#;;   UCMATT5 dc.b    9,5,3,2,1        # [78]
UCMATT5:
	.byte	9,5,3,2,1
#;;   UCMATT6 dc.b    3,5,5,6,7,8,9,10 # [79]
UCMATT6:
	.byte	3,5,5,6,7,8,9,10
#;;   UCMATT7 dc.w    $1,$11,$51,$91,$111,$211,$411,$811 # [80]
UCMATT7:
	.short	0x1,0x11,0x51,0x91,0x111,0x211,0x411,0x811
	.text
	.p2align	2
UCMAT4:
#;;   add.b     d3,d3                  # [82 CC NE X]
	movb     d3_b,%al
	addb     %al,d3_b
	setcb    xflag
#;;   bne.b     ucmat5                 # [83]
	jne      UCMAT5
#;;   move.b    (a0)+,d3               # [84]
	lodsb    
	movb     %al,d3_b
#;;   addx.b    d3,d3                  # [85 CC X]
	movb     d3_b,%al
	btw      $0,xflag
	adcb     %al,d3_b
	setcb    xflag
UCMAT5:
#;;   rts                              # [86]
	ret      
UCMAT6:
#;;   moveq     #0,d1                  # [88]
	movl     $0,%edx
UCMAT7:
#;;   bsr.b     ucmat4                 # [89 (X)]
	call     UCMAT4
	setcb    xflag
#;;   addx.w    d1,d1                  # [90]
	btw      $0,xflag
	adcw     %dx,%dx
#;;   subq.b    #1,d4                  # [91 PL]
	subb     $1,d4_b
#;;   bpl.b     ucmat7                 # [92]
	jns      UCMAT7
#;;   rts                              # [93]
	ret      
UCMATE:
#;;   cmp.l     a1,a2                  # [95 EQ]
	cmpl     %edi,a2_l
#;;   beq.b     ucmato                 # [96]
	je       UCMATO
#;;   addq.l    #4,sp                  # [97]
	lea      4(%esp),%esp
UCMATE1:
#;;   move.w    #UERR_Corrupt,UI_ErrorNum # [98]
	movw     $-1,UI_ErrorNum
#;;   moveq     #0,d0                  # [99]
	movl     $0,%ebx
#;;   rts                              # [100]
	movl %ebx,%eax
	ret      
UCMAT13:
#;;   move.l    UI_DecrunchAdr,a0      # [104]
	movl     UI_DecrunchAdr,%esi
#;;   move.l    a0,a2                  # [105]
	movl     %esi,a2_l
#;;   add.l     (a5),a2                # [106]
	movl     a5_l,%ecx
	movl     (%ecx),%eax
	addl     %eax,a2_l
#;;   move.l    4(a5),d1               # [107]
	movl     4(%ecx),%edx
#;;   move.w    8(a5),d0               # [108]
	movw     8(%ecx),%bx
#;;   lea       $1000(a2),a1           # [109]
	movl     a2_l,%ecx
	lea      0x1000(%ecx),%edi
#;;   add.l     d1,a0                  # [110]
	lea      (%esi,%edx),%esi
UCMAT14:
#;;   move.b    -(a0),-(a1)            # [111]
	lea      -1(%esi),%esi
	lea      -1(%edi),%edi
	movb     (%esi),%al
	movb     %al,(%edi)
#;;   subq.l    #1,d1                  # [112 NE]
	subl     $1,%edx
#;;   bne.b     ucmat14                # [113]
	jne      UCMAT14
#;;   moveq     #0,d1                  # [115]
	movl     $0,%edx
UCMAT15:
#;;   move.b    (a1)+,d1               # [116]
	movb     (%edi),%dl
	lea      1(%edi),%edi
#;;   cmp.w     d0,d1                  # [117 NE]
	cmpw     %bx,%dx
#;;   bne.b     ucmat17                # [118]
	jne      UCMAT17
#;;   moveq     #0,d2                  # [119]
	movl     $0,d2_l
#;;   move.b    (a1)+,d2               # [120 EQ]
	movb     (%edi),%al
	movb     %al,d2_b
	testb    %al,%al
	lea      1(%edi),%edi
#;;   beq.b     ucmat17                # [121]
	je       UCMAT17
#;;   move.b    (a1)+,d1               # [122]
	movb     (%edi),%dl
	lea      1(%edi),%edi
UCMAT16:
#;;   cmp.l     a6,a0                  # [123 EQ]
	cmpl     %ebp,%esi
#;;   beq.b     ucmate1                # [124]
	je       UCMATE1
#;;   move.b    d1,(a0)+               # [125]
	movb     %dl,(%esi)
	lea      1(%esi),%esi
#;;   dbra      d2,ucmat16             # [126]
	decw     d2_w
	cmpw     $-1,d2_w
	jne      UCMAT16
#;;   cmp.l     a6,a0                  # [127 EQ]
	cmpl     %ebp,%esi
#;;   beq.b     ucmate1                # [128]
	je       UCMATE1
#;;   move.b    d1,(a0)+               # [129]
	movb     %dl,(%esi)
	lea      1(%esi),%esi
UCMAT17:
#;;   cmp.l     a6,a0                  # [130 EQ]
	cmpl     %ebp,%esi
#;;   beq.b     ucmate1                # [131]
	je       UCMATE1
#;;   move.b    d1,(a0)+               # [132]
	movb     %dl,(%esi)
	lea      1(%esi),%esi
#;;   cmp.l     a0,a2                  # [133 GT]
	cmpl     %esi,a2_l
#;;   bgt.b     ucmat15                # [134]
	jg       UCMAT15
#;;   moveq     #-1,d0                 # [135]
	movl     $-1,%ebx
#;;   rts                              # [136]
	movl %ebx,%eax
	ret      
