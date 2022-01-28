#; a3 in
#; UI_DecrunchAdr out
    .text
    .globl TRY101
    .globl _TRY101
TRY101:
_TRY101:
    movl 4(%esp),%eax
    movl %eax,a3_l
    
    movl 8(%esp),%eax
    movl %eax,UI_DecrunchAdr


#;;   move.l    #1056,d0               # [5]
	movl     $1056,%ebx
#;;   bsr.w     alcmemj                # [6]
    push %ebx
	call     _malloc
#;;   move.l    d0,UI_Temp(a4)         # [7 EQ]
	movl     a4_l,%ecx
	movl     %ebx,UI_Temp
	testl    %ebx,%ebx
#;;   beq.b     utryo                  # [8]
	je       UTRYO
#;;   move.l    d0,a5                  # [10]
	movl     %ebx,a5_l
#;;   move.l    d0,a1                  # [11]
	movl     %ebx,%edi
#;;   move.l    a3,a0                  # [12]
	movl     a3_l,%esi
#;;   move.w    #1056/4-1,d7           # [13]
	movw     $1056/4-1,d7_w
TRYIT1:
#;;   move.l    (a0)+,(a1)+            # [14]
	movsl    
#;;   dbra      d7,tryit1              # [15]
	decw     d7_w
	cmpw     $-1,d7_w
	jne      TRYIT1
#;;   move.l    UI_DecrunchAdr(a4),$18(a5) # [17]
	movl     a4_l,%ecx
	movl     UI_DecrunchAdr,%eax
	movl     a5_l,%ecx
	movl     %eax,0x18(%ecx)
#;;   lea       $5ba-$196(a3),a1       # [18]
	movl     a3_l,%ecx
	lea      0x5ba-0x196(%ecx),%edi
#;;   bsr.b     utryit                 # [19]
	call     UTRYIT
#;;   move.l    UI_Temp(a4),a1         # [21]
	movl     a4_l,%ecx
	movl     UI_Temp,%edi
#;;   bsr.w     frememj                # [22]
    push %edi
	call     _free
#;;   clr.l     UI_Temp(a4)            # [23]
	movl     a4_l,%ecx
	movl     $0,UI_Temp
#;;   moveq     #-1,d0                 # [25]
	movl     $-1,%ebx
UTRYO:
#;;   rts                              # [26]
	movl %ebx,%eax
	ret      
UTRYIT:
#;;   move.l    $18(a5),a2             # [31]
	movl     a5_l,%ecx
	movl     0x18(%ecx),%eax
	movl     %eax,a2_l
#;;   move.l    $14(a5),d0             # [32]
	movl     0x14(%ecx),%ebx
#;;   move.l    $0c(a5),d3             # [33]
	movl     0xc(%ecx),%eax
	movl     %eax,d3_l
#;;   add.l     d3,a2                  # [34]
	movl     d3_l,%eax
	addl     %eax,a2_l
#;;   lea       $24(a5),a0             # [35]
	lea      0x24(%ecx),%esi
#;;   add.l     $10(a5),a1             # [36]
	addl     0x10(%ecx),%edi
#;;   move.l    a0,a6                  # [37]
	movl     %esi,%ebp
#;;   moveq     #$1f,d7                # [39]
	movl     $0x1f,d7_l
#;;   move.l    -(a1),d5               # [40]
	lea      -8(%edi),%edi
	movl     4(%edi),%eax
	movl     %eax,d5_l
#;;   move.l    -(a1),d4               # [41]
	movl     0(%edi),%eax
	movl     %eax,d4_l
#;;   lsr.l     #1,d0                  # [42]
	shrl     $1,%ebx
#;;   add.l     d0,a0                  # [43]
	lea      (%esi,%ebx),%esi
UTRY1:
#;;   move.l    a0,a3                  # [44]
	movl     %esi,a3_l
UTRY2:
#;;   moveq     #1,d0                  # [45]
	movl     $1,%ebx
#;;   moveq     #0,d6                  # [46]
	movl     $0,d6_l
UTRY3:
#;;   lsr.l     #1,d5                  # [47 X]
	shrl     $1,d5_l
#;;   roxl.l    #1,d6                  # [48]
	rcll     $1,d6_l
#;;   subq.w    #1,d7                  # [49 PL]
	subw     $1,d7_w
#;;   bpl.b     utry4                  # [50]
	jns      UTRY4
#;;   moveq     #$1f,d7                # [51]
	movl     $0x1f,d7_l
#;;   move.l    d4,d5                  # [52]
	movl     d4_l,%eax
	movl     %eax,d5_l
#;;   move.l    -(a1),d4               # [53]
	lea      -4(%edi),%edi
	movl     (%edi),%eax
	movl     %eax,d4_l
UTRY4:
#;;   subq.w    #1,d0                  # [54 NE]
	subw     $1,%bx
#;;   bne.b     utry3                  # [55]
	jne      UTRY3
#;;   eor.b     #$01,d6                # [56]
	xorb     $0x1,d6_b
#;;   add.l     d6,d6                  # [57]
	movl     d6_l,%eax
	addl     %eax,d6_l
#;;   add.l     d6,a3                  # [58]
	movl     d6_l,%eax
	addl     %eax,a3_l
#;;   moveq     #0,d6                  # [59]
	movl     $0,d6_l
#;;   move.w    (a3),d6                # [60]
	movl     a3_l,%ecx
	movw     (%ecx),%ax
	movw     %ax,d6_w
#;;   cmp.w     #1024,d6               # [61 CC]
	cmpw     $1024,d6_w
#;;   bcc.b     utry5                  # [62]
	jnc      UTRY5
#;;   add.l     a6,d6                  # [63]
	addl     %ebp,d6_l
#;;   move.l    d6,a3                  # [64]
	movl     d6_l,%eax
	movl     %eax,a3_l
#;;   bra.b     utry2                  # [65]
	jmp      UTRY2
UTRY5:
#;;   move.b    d6,-(a2)               # [67]
	movb     d6_b,%al
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     %al,(%ecx)
#;;   subq.l    #1,d3                  # [68 NE]
	subl     $1,d3_l
#;;   bne.b     utry1                  # [69]
	jne      UTRY1
#;;   move.l    $18(a5),a1             # [70]
	movl     a5_l,%ecx
	movl     0x18(%ecx),%edi
#;;   move.l    $04(a5),d0             # [71]
	movl     0x4(%ecx),%ebx
#;;   move.l    a1,a0                  # [72]
	movl     %edi,%esi
#;;   add.l     d0,a0                  # [73]
	lea      (%esi,%ebx),%esi
#;;   move.l    a1,a2                  # [74]
	movl     %edi,a2_l
#;;   add.l     $0c(a5),a2             # [75]
	movl     0xc(%ecx),%eax
	addl     %eax,a2_l
UTRY6:
#;;   moveq     #0,d0                  # [76]
	movl     $0,%ebx
#;;   move.b    -(a2),d0               # [77]
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     (%ecx),%bl
#;;   cmp.b     $22(a5),d0             # [78 NE]
	movl     a5_l,%ecx
	cmpb     0x22(%ecx),%bl
#;;   bne.b     utry7                  # [79]
	jne      UTRY7
#;;   move.b    -(a2),d1               # [80 EQ]
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     (%ecx),%dl
	testb    %dl,%dl
#;;   beq.b     utry15                 # [81]
	je       UTRY15
#;;   move.b    -(a2),d0               # [82]
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     (%ecx),%bl
#;;   move.b    d0,-(a0)               # [83]
	lea      -4(%esi),%esi
	movb     %bl,3(%esi)
#;;   move.b    d1,-(a0)               # [84]
	movb     %dl,2(%esi)
#;;   move.b    d0,-(a0)               # [85]
	movb     %bl,1(%esi)
#;;   move.b    d1,-(a0)               # [86]
	movb     %dl,0(%esi)
#;;   bra.b     utry16                 # [87]
	jmp      UTRY16
UTRY7:
#;;   cmp.b     $21(a5),d0             # [89 NE]
	cmpb     0x21(%ecx),%bl
#;;   bne.b     utry8                  # [90]
	jne      UTRY8
#;;   moveq     #2,d4                  # [91]
	movl     $2,d4_l
#;;   bra.b     utry9                  # [92]
	jmp      UTRY9
UTRY8:
#;;   cmp.b     $23(a5),d0             # [94 NE]
	cmpb     0x23(%ecx),%bl
#;;   bne.b     utry10                 # [95]
	jne      UTRY10
#;;   moveq     #3,d4                  # [96]
	movl     $3,d4_l
UTRY9:
#;;   moveq     #0,d1                  # [97]
	movl     $0,%edx
#;;   move.b    -(a2),d1               # [98 EQ]
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     (%ecx),%dl
	testb    %dl,%dl
#;;   beq.b     utry15                 # [99]
	je       UTRY15
#;;   bra.b     utry13                 # [100]
	jmp      UTRY13
UTRY10:
#;;   cmp.b     $20(a5),d0             # [102 NE]
	cmpb     0x20(%ecx),%bl
#;;   bne.b     utry11                 # [103]
	jne      UTRY11
#;;   moveq     #3,d4                  # [104]
	movl     $3,d4_l
#;;   moveq     #0,d1                  # [105]
	movl     $0,%edx
#;;   move.b    -(a2),d1               # [106 EQ]
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     (%ecx),%dl
	testb    %dl,%dl
#;;   beq.b     utry15                 # [107]
	je       UTRY15
#;;   bra.b     utry12                 # [108]
	jmp      UTRY12
UTRY11:
#;;   cmp.b     $0b(a5),d0             # [110 NE]
	cmpb     0xb(%ecx),%bl
#;;   bne.b     utry15                 # [111]
	jne      UTRY15
#;;   moveq     #0,d4                  # [112]
	movl     $0,d4_l
#;;   move.b    -(a2),d4               # [113 EQ]
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     (%ecx),%al
	movb     %al,d4_b
	testb    %al,%al
#;;   beq.b     utry15                 # [114]
	je       UTRY15
#;;   moveq     #0,d1                  # [115]
	movl     $0,%edx
#;;   move.b    -(a2),d1               # [116]
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     (%ecx),%dl
UTRY12:
#;;   rol.w     #8,d1                  # [117]
	rolw     $8,%dx
#;;   move.b    -(a2),d1               # [118]
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     (%ecx),%dl
#;;   rol.w     #8,d1                  # [119]
	rolw     $8,%dx
UTRY13:
#;;   move.l    a0,a3                  # [120]
	movl     %esi,a3_l
#;;   add.l     d1,a3                  # [121]
	addl     %edx,a3_l
UTRY14:
#;;   move.b    -(a3),-(a0)            # [122]
	subl     $1,a3_l
	lea      -1(%esi),%esi
	movl     a3_l,%ecx
	movb     (%ecx),%al
	movb     %al,(%esi)
#;;   dbra      d4,utry14              # [123]
	decw     d4_w
	cmpw     $-1,d4_w
	jne      UTRY14
#;;   bra.b     utry16                 # [124]
	jmp      UTRY16
UTRY15:
#;;   move.b    d0,-(a0)               # [126]
	lea      -1(%esi),%esi
	movb     %bl,(%esi)
UTRY16:
#;;   cmp.l     a2,a1                  # [127 LT]
	cmpl     a2_l,%edi
#;;   blt.b     utry6                  # [128]
	jl       UTRY6
#;;   move.l    $18(a5),a0             # [129]
	movl     a5_l,%ecx
	movl     0x18(%ecx),%esi
#;;   move.l    a0,a1                  # [130]
	movl     %esi,%edi
#;;   add.l     (a5),a0                # [131]
	addl     (%ecx),%esi
#;;   add.l     $04(a5),a1             # [132]
	addl     0x4(%ecx),%edi
#;;   move.b    -(a1),d1               # [133]
	lea      -1(%edi),%edi
	movb     (%edi),%dl
UTRY17:
#;;   cmp.b     $08(a5),d1             # [134 EQ]
	movl     a5_l,%ecx
	cmpb     0x8(%ecx),%dl
#;;   beq.b     utry19                 # [135]
	je       UTRY19
#;;   cmp.b     $09(a5),d1             # [136 EQ]
	cmpb     0x9(%ecx),%dl
#;;   beq.b     utry19                 # [137]
	je       UTRY19
#;;   move.b    d1,d2                  # [138]
	movb     %dl,d2_b
#;;   move.b    -(a1),d1               # [139]
	lea      -1(%edi),%edi
	movb     (%edi),%dl
#;;   move.b    d2,-(a0)               # [140]
	movb     d2_b,%al
	lea      -1(%esi),%esi
	movb     %al,(%esi)
UTRY18:
#;;   cmp.l     $18(a5),a0             # [141 GT]
	movl     a5_l,%ecx
	cmpl     0x18(%ecx),%esi
#;;   bgt.b     utry17                 # [142]
	jg       UTRY17
#;;   rts                              # [143]
	ret      
UTRY19:
#;;   move.b    d1,d2                  # [145]
	movb     %dl,d2_b
#;;   moveq     #0,d4                  # [146]
	movl     $0,d4_l
#;;   move.b    -(a1),d4               # [147]
	lea      -1(%edi),%edi
	movb     (%edi),%al
	movb     %al,d4_b
#;;   cmp.b     #3,d4                  # [148 CC]
	cmpb     $3,d4_b
#;;   bcc.b     utry20                 # [149]
	jnc      UTRY20
#;;   move.b    d2,d3                  # [150]
	movb     d2_b,%al
	movb     %al,d3_b
#;;   move.b    -(a1),d1               # [151]
	lea      -1(%edi),%edi
	movb     (%edi),%dl
#;;   bra.b     utry22                 # [152]
	jmp      UTRY22
UTRY20:
#;;   cmp.b     $09(a5),d2             # [154 NE]
	movl     a5_l,%ecx
	movb     0x9(%ecx),%al
	cmpb     %al,d2_b
#;;   bne.b     utry21                 # [155]
	jne      UTRY21
#;;   move.b    -(a1),d1               # [156]
	lea      -1(%edi),%edi
	movb     (%edi),%dl
#;;   move.b    $0a(a5),d3             # [157]
	movb     0xa(%ecx),%al
	movb     %al,d3_b
#;;   bra.b     utry22                 # [158]
	jmp      UTRY22
UTRY21:
#;;   move.b    -(a1),d3               # [160]
	lea      -2(%edi),%edi
	movb     1(%edi),%al
	movb     %al,d3_b
#;;   move.b    -(a1),d1               # [161]
	movb     0(%edi),%dl
UTRY22:
#;;   move.b    d3,-(a0)               # [162]
	movb     d3_b,%al
	lea      -1(%esi),%esi
	movb     %al,(%esi)
#;;   dbra      d4,utry22              # [163]
	decw     d4_w
	cmpw     $-1,d4_w
	jne      UTRY22
#;;   bra.b     utry18                 # [164]
	jmp      UTRY18
