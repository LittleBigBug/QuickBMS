#    a0 = in + insz
#    UI_DecrunchLen = outsz
#    d0 = out
 
    .text
    .globl UMEGA
    .globl _UMEGA
UMEGA:
_UMEGA:
    movl 4(%esp),%esi   # a0
    movl 8(%esp),%eax
    movl %eax,UI_DecrunchLen
    movl 12(%esp),%ebx




#;;   move.l    d0,UI_DecrunchAdr(a4)  # [2]
	movl     a4_l,%ecx
	movl     %ebx,UI_DecrunchAdr
#;;   move.l    d0,a1                  # [3]
	movl     %ebx,%edi
#;;   move.l    a1,a2                  # [4]
	movl     %edi,a2_l
#;;   add.l     UI_DecrunchLen(a4),a2  # [5]
	movl     UI_DecrunchLen,%eax
	addl     %eax,a2_l
UMEGA0:
#;;   moveq     #-1,d7                 # [7]
	movl     $-1,d7_l
#;;   moveq     #1,d6                  # [8]
	movl     $1,d6_l
#;;   moveq     #2,d5                  # [9]
	movl     $2,d5_l
#;;   moveq     #7,d4                  # [10]
	movl     $7,d4_l
#;;   move.l    -(a0),d0               # [11]
	lea      -4(%esi),%esi
	movl     (%esi),%ebx
UMEGA1:
#;;   moveq     #3,d1                  # [13]
	movl     $3,%edx
#;;   bsr.b     umega10                # [14]
	call     UMEGA10
#;;   tst.b     d2                     # [15 EQ]
	testb    $0xff,d2_b
#;;   beq.b     umega8                 # [16]
	je       UMEGA8
#;;   cmp.w     d4,d2                  # [17 NE]
	movw     d4_w,%ax
	cmpw     %ax,d2_w
#;;   bne.b     umega4                 # [18]
	jne      UMEGA4
#;;   lsr.l     d6,d0                  # [19 CC NE]
	movl     d6_l,%ecx
	testl    $-1,%ebx
	shrl     %cl,%ebx
#;;   bne.b     umega2                 # [20]
	jne      UMEGA2
#;;   bsr.b     umega9                 # [21 (CC)]
	call     UMEGA9
UMEGA2:
#;;   bcc.b     umega3                 # [22]
	jnc      UMEGA3
#;;   moveq     #10,d1                 # [23]
	movl     $10,%edx
#;;   bsr.b     umega10                # [24]
	call     UMEGA10
#;;   tst.w     d2                     # [25 NE]
	testw    $0xffff,d2_w
#;;   bne.b     umega4                 # [26]
	jne      UMEGA4
#;;   moveq     #$12,d1                # [27]
	movl     $0x12,%edx
#;;   bsr.b     umega10                # [28]
	call     UMEGA10
#;;   bra.b     umega4                 # [29]
	jmp      UMEGA4
UMEGA3:
#;;   moveq     #4,d1                  # [31]
	movl     $4,%edx
#;;   bsr.b     umega10                # [32]
	call     UMEGA10
#;;   addq.w    #7,d2                  # [33]
	addw     $7,d2_w
UMEGA4:
#;;   subq.w    #1,d2                  # [34 X]
	subw     $1,d2_w
	setcb    xflag
UMEGA5:
#;;   moveq     #7,d1                  # [35]
	movl     $7,%edx
UMEGA6:
#;;   lsr.l     d6,d0                  # [36 NE X]
	movl     d6_l,%ecx
	testl    $-1,%ebx
	btw      $0,xflag
	shrl     %cl,%ebx
	setcb    xflag
#;;   bne.b     umega7                 # [37]
	jne      UMEGA7
#;;   bsr.b     umega9                 # [38 (X)]
	call     UMEGA9
	setcb    xflag
UMEGA7:
#;;   roxl.l    d6,d3                  # [39 X]
	movb     d6_b,%al
	btw      $0,xflag
	movb     %al,%cl
	rcll     %cl,d3_l
	setcb    xflag
#;;   dbra      d1,umega6              # [40]
	decw     %dx
	cmpw     $-1,%dx
	jne      UMEGA6
#;;   move.b    d3,-(a2)               # [41]
	movb     d3_b,%al
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     %al,(%ecx)
#;;   cmp.l     a1,a2                  # [42 EQ]
	cmpl     %edi,a2_l
#;;   dbeq      d2,umega5              # [43]
	je       _PA_39_
	decw     d2_w
	cmpw     $-1,d2_w
	jne      UMEGA5
_PA_39_:         
#;;   cmp.w     d7,d2                  # [44 EQ]
	movw     d7_w,%ax
	cmpw     %ax,d2_w
#;;   beq.b     umega8                 # [45]
	je       UMEGA8
#;;   bra.w     umega22                # [46]
	jmp      UMEGA22
UMEGA8:
#;;   moveq     #2,d1                  # [48]
	movl     $2,%edx
#;;   bsr.b     umega10                # [49]
	call     UMEGA10
#;;   moveq     #2,d3                  # [50]
	movl     $2,d3_l
#;;   moveq     #8,d1                  # [51]
	movl     $8,%edx
#;;   tst.w     d2                     # [52 EQ]
	testw    $0xffff,d2_w
#;;   beq.b     umega20                # [53]
	je       UMEGA20
#;;   moveq     #4,d3                  # [54]
	movl     $4,d3_l
#;;   cmp.w     d5,d2                  # [55 EQ]
	movw     d5_w,%ax
	cmpw     %ax,d2_w
#;;   beq.b     umega17                # [56]
	je       UMEGA17
#;;   moveq     #3,d3                  # [57]
	movl     $3,d3_l
#;;   cmp.w     d6,d2                  # [58 EQ]
	movw     d6_w,%ax
	cmpw     %ax,d2_w
#;;   beq.b     umega15                # [59]
	je       UMEGA15
#;;   moveq     #2,d1                  # [60]
	movl     $2,%edx
#;;   bsr.b     umega10                # [61]
	call     UMEGA10
#;;   cmp.w     #3,d2                  # [62 EQ]
	cmpw     $3,d2_w
#;;   beq.b     umega14                # [63]
	je       UMEGA14
#;;   cmp.w     d5,d2                  # [64 EQ]
	movw     d5_w,%ax
	cmpw     %ax,d2_w
#;;   beq.b     umega13                # [65]
	je       UMEGA13
#;;   addq.w    #5,d2                  # [66]
	addw     $5,d2_w
#;;   move.w    d2,d3                  # [67]
	movw     d2_w,%ax
	movw     %ax,d3_w
#;;   bra.b     umega17                # [68]
	jmp      UMEGA17
UMEGA9:
#;;   move.l    -(a0),d0               # [70]
	lea      -4(%esi),%esi
	movl     (%esi),%ebx
#;;   move.w    #$0010,ccr             # [71 X]
	movb     $((0x10)>>4)&1,xflag
#;;   roxr.l    d6,d0                  # [72 CC CS X]
	btw      $0,xflag
	movb     d6_b,%cl
	rcrl     %cl,%ebx
	setcb    xflag
#;;   rts                              # [73]
	ret      
UMEGA10:
#;;   subq.w    #1,d1                  # [75 X]
	subw     $1,%dx
	setcb    xflag
#;;   moveq     #0,d2                  # [76]
	movl     $0,d2_l
UMEGA11:
#;;   lsr.l     d6,d0                  # [77 NE X]
	movl     d6_l,%ecx
	testl    $-1,%ebx
	btw      $0,xflag
	shrl     %cl,%ebx
	setcb    xflag
#;;   bne.b     umega12                # [78]
	jne      UMEGA12
#;;   bsr.b     umega9                 # [79 (X)]
	call     UMEGA9
	setcb    xflag
UMEGA12:
#;;   roxl.l    d6,d2                  # [80 X]
	movb     d6_b,%al
	btw      $0,xflag
	movb     %al,%cl
	rcll     %cl,d2_l
	setcb    xflag
#;;   dbra      d1,umega11             # [81]
	decw     %dx
	cmpw     $-1,%dx
	jne      UMEGA11
#;;   rts                              # [82]
	ret      
UMEGA13:
#;;   moveq     #2,d1                  # [84]
	movl     $2,%edx
#;;   bsr.b     umega10                # [85]
	call     UMEGA10
#;;   addq.w    #7,d2                  # [86]
	addw     $7,d2_w
#;;   move.w    d2,d3                  # [87]
	movw     d2_w,%ax
	movw     %ax,d3_w
#;;   bra.b     umega17                # [88]
	jmp      UMEGA17
UMEGA14:
#;;   moveq     #8,d1                  # [90]
	movl     $8,%edx
#;;   bsr.b     umega10                # [91]
	call     UMEGA10
#;;   move.w    d2,d3                  # [92]
	movw     d2_w,%ax
	movw     %ax,d3_w
#;;   bra.b     umega17                # [93]
	jmp      UMEGA17
UMEGA15:
#;;   moveq     #8,d1                  # [95]
	movl     $8,%edx
#;;   lsr.l     d6,d0                  # [96 CS NE]
	movl     d6_l,%ecx
	testl    $-1,%ebx
	shrl     %cl,%ebx
#;;   bne.b     umega16                # [97]
	jne      UMEGA16
#;;   bsr.b     umega9                 # [98 (CS)]
	call     UMEGA9
UMEGA16:
#;;   bcs.b     umega20                # [99]
	jc       UMEGA20
#;;   moveq     #14,d1                 # [100]
	movl     $14,%edx
#;;   bra.b     umega20                # [101]
	jmp      UMEGA20
UMEGA17:
#;;   moveq     #16,d1                 # [103]
	movl     $16,%edx
#;;   lsr.l     d6,d0                  # [104 CC NE]
	movl     d6_l,%ecx
	testl    $-1,%ebx
	shrl     %cl,%ebx
#;;   bne.b     umega18                # [105]
	jne      UMEGA18
#;;   bsr.b     umega9                 # [106 (CC)]
	call     UMEGA9
UMEGA18:
#;;   bcc.b     umega20                # [107]
	jnc      UMEGA20
#;;   moveq     #8,d1                  # [108]
	movl     $8,%edx
#;;   lsr.l     d6,d0                  # [109 CS NE]
	movl     d6_l,%ecx
	testl    $-1,%ebx
	shrl     %cl,%ebx
#;;   bne.b     umega19                # [110]
	jne      UMEGA19
#;;   bsr.b     umega9                 # [111 (CS)]
	call     UMEGA9
UMEGA19:
#;;   bcs.b     umega20                # [112]
	jc       UMEGA20
#;;   moveq     #12,d1                 # [113]
	movl     $12,%edx
UMEGA20:
#;;   bsr.b     umega10                # [114]
	call     UMEGA10
#;;   subq.w    #1,d3                  # [115]
	subw     $1,d3_w
UMEGA21:
#;;   move.b    -1(a2,d2.l),-(a2)      # [116]
	subl     $1,a2_l
	movl     d2_l,%ecx
	addl     a2_l,%ecx
	movb     -1(%ecx),%al
	movl     a2_l,%ecx
	movb     %al,(%ecx)
#;;   cmp.l     a1,a2                  # [117 EQ]
	cmpl     %edi,a2_l
#;;   dbeq      d3,umega21             # [118]
	je       _PA_107_
	decw     d3_w
	cmpw     $-1,d3_w
	jne      UMEGA21
_PA_107_:         
#;;   cmp.w     d7,d3                  # [119 EQ]
	movw     d7_w,%ax
	cmpw     %ax,d3_w
#;;   beq.w     umega1                 # [120]
	je       UMEGA1
UMEGA22:
#;;   moveq     #-1,d0                 # [122]
	movl     $-1,%ebx
UMEGAO:
#;;   rts                              # [123]
	movl %ebx,%eax
	ret      
    
    
#;make object, not needed

MECRUOB:
#;;   move.l    UI_DecrunchAdr(a4),a3  # [128]
	movl     a4_l,%ecx
	movl     UI_DecrunchAdr,%eax
	movl     %eax,a3_l
#;;   move.l    a3,UI_Temp(a4)         # [129]
	movl     a3_l,%eax
	movl     %eax,UI_Temp
#;;   move.l    a3,a6                  # [130]
	movl     a3_l,%ebp
#;;   add.l     UI_DecrunchLen(a4),a6  # [131]
	addl     UI_DecrunchLen,%ebp
#;;   lea       $104(a3),a0            # [133]
	movl     a3_l,%ecx
	lea      0x104(%ecx),%esi
#;;   move.l    a0,a1                  # [134]
	movl     %esi,%edi
#;;   add.l     $ec(a3),a1             # [135]
	addl     0xec(%ecx),%edi
#;;   move.l    a1,a5                  # [136]
	movl     %edi,a5_l
#;;   move.l    (a0),d0                # [137]
	movl     (%esi),%ebx
#;;   addq.l    #1,d0                  # [138]
	addl     $1,%ebx
#;;   move.l    d0,d2                  # [139]
	movl     %ebx,d2_l
#;;   moveq     #$14,d1                # [140]
	movl     $0x14,%edx
#;;   add.l     $ec(a3),d1             # [141]
	addl     0xec(%ecx),%edx
#;;   subq.l    #4,d1                  # [142]
	subl     $4,%edx
#;;   lsl.l     #2,d0                  # [143]
	shll     $2,%ebx
#;;   sub.l     d0,d1                  # [144]
	subl     %ebx,%edx
#;;   moveq     #-$14,d3               # [145]
	movl     $-0x14,d3_l
#;;   add.l     d1,d3                  # [146]
	addl     %edx,d3_l
#;;   lsl.l     #2,d0                  # [147]
	shll     $2,%ebx
#;;   add.l     d0,d1                  # [148]
	addl     %ebx,%edx
#;;   bsr.w     mecro14                # [149]
	call     MECRO14
#;;   move.l    d1,UI_DecrunchLen(a4)  # [150]
	movl     a4_l,%ecx
	movl     %edx,UI_DecrunchLen
#;;   move.l    d1,d0                  # [151]
	movl     %edx,%ebx
#;;   bsr.w     alcdmem                # [152]
    push %ebx
	call     _malloc
#;;   move.l    d0,UI_DecrunchAdr(a4)  # [153 EQ]
	movl     a4_l,%ecx
	movl     %ebx,UI_DecrunchAdr
	testl    %ebx,%ebx
#;;   beq.b     mecrob5                # [154]
	je       MECROB5
#;;   move.l    d0,a2                  # [155]
	movl     %ebx,a2_l
#;;   move.l    #$3f3,(a2)+            # [156]
	movl     a2_l,%ecx
	movl     $0x3f3,0(%ecx)
#;;   clr.l     (a2)+                  # [157]
	movl     $0,4(%ecx)
#;;   move.l    d2,(a2)+               # [158]
	movl     d2_l,%eax
	movl     %eax,8(%ecx)
#;;   clr.l     (a2)+                  # [159]
	movl     $0,12(%ecx)
#;;   move.l    $104(a3),(a2)+         # [160]
	movl     a3_l,%ecx
	movl     0x104(%ecx),%eax
	movl     a2_l,%ecx
	addl     $20,a2_l
	movl     %eax,16(%ecx)
#;;   move.l    a2,a1                  # [161]
	movl     a2_l,%edi
#;;   move.l    d2,d5                  # [162]
	movl     d2_l,%eax
	movl     %eax,d5_l
#;;   moveq     #0,d1                  # [163]
	movl     $0,%edx
#;;   lea       $108(a3),a0            # [164]
	movl     a3_l,%ecx
	lea      0x108(%ecx),%esi
MECROB1:
#;;   subq.l    #1,d2                  # [165 EQ]
	subl     $1,d2_l
#;;   beq.b     mecrob2                # [166]
	je       MECROB2
#;;   move.l    4(a0),d0               # [167]
	movl     4(%esi),%ebx
#;;   sub.l     (a0)+,d0               # [168]
	subl     (%esi),%ebx
	lea      4(%esi),%esi
#;;   add.l     d0,d1                  # [169]
	addl     %ebx,%edx
#;;   lsr.l     #2,d0                  # [170]
	shrl     $2,%ebx
#;;   move.l    d0,(a2)+               # [171]
	movl     a2_l,%ecx
	addl     $4,a2_l
	movl     %ebx,(%ecx)
#;;   bra.b     mecrob1                # [172]
	jmp      MECROB1
MECROB2:
#;;   move.l    d3,d0                  # [174]
	movl     d3_l,%ebx
#;;   sub.l     d1,d0                  # [175]
	subl     %edx,%ebx
#;;   lsr.l     #2,d0                  # [176]
	shrl     $2,%ebx
#;;   move.l    d0,(a2)+               # [177]
	movl     a2_l,%ecx
	addl     $4,a2_l
	movl     %ebx,(%ecx)
#;;   addq.l    #4,a0                  # [178]
	lea      4(%esi),%esi
#;;   moveq     #0,d4                  # [179]
	movl     $0,d4_l
MECROB3:
#;;   move.l    #$3e9,(a2)+            # [180]
	movl     a2_l,%ecx
	movl     $0x3e9,0(%ecx)
#;;   move.l    (a1)+,d0               # [181]
	movl     (%edi),%ebx
	lea      4(%edi),%edi
#;;   move.l    d0,(a2)+               # [182 EQ]
	addl     $8,a2_l
	movl     %ebx,4(%ecx)
	testl    %ebx,%ebx
#;;   beq.b     mecro4a                # [183]
	je       MECRO4a
MECROB4:
#;;   move.l    (a0)+,(a2)+            # [184]
	lodsl    
	movl     a2_l,%ecx
	addl     $4,a2_l
	movl     %eax,(%ecx)
#;;   subq.l    #1,d0                  # [185 NE]
	subl     $1,%ebx
#;;   bne.b     mecrob4                # [186]
	jne      MECROB4
MECRO4a:
#;;   bsr.b     mecrob7                # [187]
	call     MECROB7
#;;   move.l    #$3f2,(a2)+            # [188]
	movl     a2_l,%ecx
	addl     $4,a2_l
	movl     $0x3f2,(%ecx)
#;;   addq.l    #4,d4                  # [189]
	addl     $4,d4_l
#;;   subq.l    #1,d5                  # [190 NE]
	subl     $1,d5_l
#;;   bne.b     mecrob3                # [191]
	jne      MECROB3
#;;   moveq     #-1,d0                 # [193]
	movl     $-1,%ebx
MECROB5:
#;;   move.l    UI_Temp(a4),a1         # [194]
	movl     a4_l,%ecx
	movl     UI_Temp,%edi
#;;   bsr.w     fremem                 # [195]
    push %edi
	call     _free
#;;   clr.l     UI_Temp(a4)            # [196]
	movl     a4_l,%ecx
	movl     $0,UI_Temp
MECROB6:
#;;   rts                              # [197]
	ret      
MECROB7:
#;;   move.l    a0,-(sp)               # [199]
	pushl    %esi
#;;   move.l    a5,a0                  # [200]
	movl     a5_l,%esi
MECROB8:
#;;   move.l    (a0)+,d1               # [201]
	movl     0(%esi),%edx
#;;   move.l    (a0)+,d0               # [202 EQ]
	movl     4(%esi),%ebx
	testl    %ebx,%ebx
	lea      8(%esi),%esi
#;;   beq.b     mecrob9                # [203]
	je       MECROB9
#;;   cmp.l     #$3ec,d0               # [204 EQ]
	cmpl     $0x3ec,%ebx
#;;   beq.b     mecro10                # [205]
	je       MECRO10
MECROB9:
#;;   move.l    (sp)+,a0               # [206]
	popl     %esi
#;;   rts                              # [207]
	ret      
MECRO10:
#;;   cmp.l     d1,d4                  # [209 NE]
	cmpl     %edx,d4_l
#;;   bne.b     mecro13                # [210]
	jne      MECRO13
#;;   move.l    #$3ec,(a2)+            # [211]
	movl     a2_l,%ecx
	addl     $4,a2_l
	movl     $0x3ec,(%ecx)
#;;   move.l    (a0)+,d0               # [212]
	movl     (%esi),%ebx
	lea      4(%esi),%esi
MECRO11:
#;;   move.l    d0,(a2)+               # [213]
	movl     a2_l,%ecx
	addl     $4,a2_l
	movl     %ebx,(%ecx)
#;;   addq.l    #1,d0                  # [214]
	addl     $1,%ebx
MECRO12:
#;;   move.l    (a0)+,(a2)+            # [215]
	lodsl    
	movl     a2_l,%ecx
	addl     $4,a2_l
	movl     %eax,(%ecx)
#;;   subq.l    #1,d0                  # [216 NE]
	subl     $1,%ebx
#;;   bne.b     mecro12                # [217]
	jne      MECRO12
#;;   move.l    (a0)+,d0               # [218 NE]
	movl     (%esi),%ebx
	testl    %ebx,%ebx
	lea      4(%esi),%esi
#;;   bne.b     mecro11                # [219]
	jne      MECRO11
#;;   clr.l     (a2)+                  # [220]
	movl     a2_l,%ecx
	addl     $4,a2_l
	movl     $0,(%ecx)
#;;   bra.b     mecrob9                # [221]
	jmp      MECROB9
MECRO13:
#;;   move.l    (a0)+,d0               # [223 EQ]
	movl     (%esi),%ebx
	testl    %ebx,%ebx
	lea      4(%esi),%esi
#;;   beq.b     mecrob8                # [224]
	je       MECROB8
#;;   lsl.l     #2,d0                  # [225]
	shll     $2,%ebx
#;;   lea       4(a0,d0.l),a0          # [226]
	lea      4(%esi,%ebx),%esi
#;;   bra.b     mecro13                # [227]
	jmp      MECRO13
MECRO14:
#;;   addq.l    #4,a1                  # [229]
	lea      4(%edi),%edi
#;;   move.l    (a1)+,d0               # [230 EQ]
	movl     (%edi),%ebx
	testl    %ebx,%ebx
	lea      4(%edi),%edi
#;;   beq.b     mecro15                # [231]
	je       MECRO15
#;;   cmp.l     #$3ec,d0               # [232 EQ]
	cmpl     $0x3ec,%ebx
#;;   beq.b     mecro16                # [233]
	je       MECRO16
MECRO15:
#;;   rts                              # [234]
	ret      
MECRO16:
#;;   move.l    (a1)+,d0               # [236 EQ]
	movl     (%edi),%ebx
	testl    %ebx,%ebx
	lea      4(%edi),%edi
#;;   beq.b     mecro14                # [237]
	je       MECRO14
#;;   addq.l    #2,d0                  # [238]
	addl     $2,%ebx
#;;   lsl.l     #2,d0                  # [239]
	shll     $2,%ebx
#;;   add.l     d0,d1                  # [240]
	addl     %ebx,%edx
#;;   addq.l    #8,d1                  # [241]
	addl     $8,%edx
#;;   lea       -4(a1,d0.l),a1         # [242]
	lea      -4(%edi,%ebx),%edi
#;;   cmp.l     a6,a1                  # [243 LT]
	cmpl     %ebp,%edi
#;;   blt.b     mecro16                # [244]
	jl       MECRO16
#;;   addq.l    #4,sp                  # [246]
	lea      4(%esp),%esp
#;;   move.w    #_Hunk,UI_ErrorNum(a4) # [247]
	movl     a4_l,%ecx
	movw     $-1,UI_ErrorNum
#;;   clr.l     UI_Temp(a4)            # [248]
	movl     $0,UI_Temp
#;;   moveq     #0,d0                  # [249]
	movl     $0,%ebx
#;;   rts                              # [250]
	movl %ebx,%eax
	ret      
