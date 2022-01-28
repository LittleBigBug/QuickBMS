    .text
    .globl MAX12
    .globl _MAX12
MAX12:
_MAX12:
    movl 4(%esp), %ebx  # d0 out
    movl 8(%esp), %esi  # a0 in



#;;   move.l    a0,a2                  # [3]
	movl     %esi,a2_l
#;;   move.l    d0,a3                  # [4]
	movl     %ebx,a3_l
#;;   add.l     $1a4(a2),a3            # [5]
	movl     a2_l,%ecx
	movl     0x1a4(%ecx),%eax
	addl     %eax,a3_l
#;;   lea       $1a8(a2),a0            # [6]
	lea      0x1a8(%ecx),%esi
#;;   add.l     $1a0(a2),a0            # [7]
	addl     0x1a0(%ecx),%esi
#;;   move.l    $22(a2),d4             # [8]
	movl     0x22(%ecx),%eax
	movl     %eax,d4_l
#;;   move.w    $198(a2),d2            # [9]
	movw     0x198(%ecx),%ax
	movw     %ax,d2_w
#;;   move.b    $19a(a2),d1            # [10]
	movb     0x19a(%ecx),%dl
#;;   move.b    $19b(a2),d0            # [11]
	movb     0x19b(%ecx),%bl
#;;   lea       -52(sp),sp             # [13]
	lea      -52(%esp),%esp
#;;   move.l    sp,a5                  # [14]
	movl     %esp,a5_l
#;;   move.l    $19c(a2),(a5)          # [15]
	movl     0x19c(%ecx),%eax
	movl     a5_l,%ecx
	movl     %eax,(%ecx)
#;;   move.l    $44(a2),4(a5)          # [16]
	movl     a2_l,%ecx
	movl     0x44(%ecx),%eax
	movl     a5_l,%ecx
	movl     %eax,4(%ecx)
#;;   move.l    $e2(a2),8(a5)          # [17]
	movl     a2_l,%ecx
	movl     0xe2(%ecx),%eax
	movl     a5_l,%ecx
	movl     %eax,8(%ecx)
#;;   move.l    $e6(a2),12(a5)         # [18]
	movl     a2_l,%ecx
	movl     0xe6(%ecx),%eax
	movl     a5_l,%ecx
	movl     %eax,12(%ecx)
#;;   move.l    $ea(a2),16(a5)         # [19]
	movl     a2_l,%ecx
	movl     0xea(%ecx),%eax
	movl     a5_l,%ecx
	movl     %eax,16(%ecx)
#;;   move.l    $13c(a2),20(a5)        # [20]
	movl     a2_l,%ecx
	movl     0x13c(%ecx),%eax
	movl     a5_l,%ecx
	movl     %eax,20(%ecx)
#;;   move.l    $140(a2),24(a5)        # [21]
	movl     a2_l,%ecx
	movl     0x140(%ecx),%eax
	movl     a5_l,%ecx
	movl     %eax,24(%ecx)
#;;   move.l    $134(a2),28(a5)        # [22]
	movl     a2_l,%ecx
	movl     0x134(%ecx),%eax
	movl     a5_l,%ecx
	movl     %eax,28(%ecx)
#;;   move.l    $138(a2),32(a5)        # [23]
	movl     a2_l,%ecx
	movl     0x138(%ecx),%eax
	movl     a5_l,%ecx
	movl     %eax,32(%ecx)
#;;   move.l    $144(a2),36(a5)        # [24]
	movl     a2_l,%ecx
	movl     0x144(%ecx),%eax
	movl     a5_l,%ecx
	movl     %eax,36(%ecx)
#;;   move.l    $148(a2),40(a5)        # [25]
	movl     a2_l,%ecx
	movl     0x148(%ecx),%eax
	movl     a5_l,%ecx
	movl     %eax,40(%ecx)
#;;   move.l    $14c(a2),44(a5)        # [26]
	movl     a2_l,%ecx
	movl     0x14c(%ecx),%eax
	movl     a5_l,%ecx
	movl     %eax,44(%ecx)
#;;   move.w    $72(a2),d7             # [27]
	movl     a2_l,%ecx
	movw     0x72(%ecx),%ax
	movw     %ax,d7_w
#;;   sub.w     #$3a,d7                # [28]
	subw     $0x3a,d7_w
#;;   ext.l     d7                     # [29]
	movswl   d7_w,%eax
	movl     %eax,d7_l
#;;   move.l    d7,48(a5)              # [30]
	movl     d7_l,%eax
	movl     a5_l,%ecx
	movl     %eax,48(%ecx)
#;;   bsr.b     umaxp1                 # [31]
	call     UMAXP1
#;;   lea       52(sp),sp              # [32]
	lea      52(%esp),%esp
#;;   moveq     #-1,d0                 # [33]
	movl     $-1,%ebx
#.OUT:
#;;   rts                              # [34]
	movl %ebx,%eax
	ret      
UMAXP:
#;;   move.l    a3,a6                  # [39]
	movl     a3_l,%ebp
#;;   sub.l     4(a5),a6               # [40]
	movl     a5_l,%ecx
	subl     4(%ecx),%ebp
UMAXP1:
#;;   move.l    d4,d5                  # [41]
	movl     d4_l,%eax
	movl     %eax,d5_l
#;;   move.l    a3,a2                  # [42]
	movl     a3_l,%eax
	movl     %eax,a2_l
#;;   move.b    d0,d0                  # [43 NE]
	movb     %bl,%bl
	testb    %bl,%bl
#;;   bne.w     umaxp32                # [44]
	jne      UMAXP32
UMAXP2:
#;;   tst.w     d2                     # [46 EQ]
	testw    $0xffff,d2_w
#;;   beq.b     umaxp4                 # [47]
	je       UMAXP4
UMAXP3:
#;;   subq.w    #1,d5                  # [48 MI]
	subw     $1,d5_w
#;;   bmi.b     umaxp                  # [49]
	js       UMAXP
#;;   move.b    -(a0),-(a3)            # [50]
	lea      -1(%esi),%esi
	subl     $1,a3_l
	movb     (%esi),%al
	movl     a3_l,%ecx
	movb     %al,(%ecx)
#;;   subq.w    #1,d2                  # [51 NE]
	subw     $1,d2_w
#;;   bne.b     umaxp3                 # [52]
	jne      UMAXP3
UMAXP4:
#;;   subq.l    #1,(a5)                # [53 EQ]
	movl     a5_l,%ecx
	subl     $1,(%ecx)
#;;   beq.b     umaxp7                 # [54]
	je       UMAXP7
#;;   moveq     #3,d7                  # [56]
	movl     $3,d7_l
#;;   lea       umaxp13(pc),a1         # [57]
	movl     $UMAXP13,%edi
#;;   add.l     48(a5),a1              # [58]
	addl     48(%ecx),%edi
UMAXP5:
#;;   add.b     d1,d1                  # [59 CS NE X]
	addb     %dl,%dl
	setcb    xflag
#;;   bne.b     umaxp6                 # [60]
	jne      UMAXP6
#;;   move.b    -(a0),d1               # [61]
	lea      -1(%esi),%esi
	movb     (%esi),%dl
#;;   addx.b    d1,d1                  # [62 CS]
	btw      $0,xflag
	adcb     %dl,%dl
UMAXP6:
#;;   bcs.b     umaxp8                 # [63]
	jc       UMAXP8
#;;   jmp       (a1)                   # [64]
	jmp      *%edi
UMAXP7:
#;;   rts                              # [66]
	ret      
UMAXP8:
#;;   subq.l    #6,a1                  # [68]
	lea      -6(%edi),%edi
#;;   dbra      d7,umaxp5              # [69]
	decw     d7_w
	cmpw     $-1,d7_w
	jne      UMAXP5
#;;   add.b     d1,d1                  # [70 CC NE X]
	addb     %dl,%dl
	setcb    xflag
#;;   bne.b     umaxp9                 # [71]
	jne      UMAXP9
#;;   move.b    -(a0),d1               # [72]
	lea      -1(%esi),%esi
	movb     (%esi),%dl
#;;   addx.b    d1,d1                  # [73 CC]
	btw      $0,xflag
	adcb     %dl,%dl
UMAXP9:
#;;   bcc.b     umaxp10                # [74]
	jnc      UMAXP10
#;;   move.b    -(a0),d0               # [75]
	lea      -1(%esi),%esi
	movb     (%esi),%bl
#;;   moveq     #3,d7                  # [76]
	movl     $3,d7_l
#;;   bra.b     umaxp14                # [77]
	jmp      UMAXP14
UMAXP10:
#;;   moveq     #2,d7                  # [79]
	movl     $2,d7_l
UMAXP11:
#;;   add.b     d1,d1                  # [80 NE X]
	addb     %dl,%dl
	setcb    xflag
#;;   bne.b     umaxp12                # [81]
	jne      UMAXP12
#;;   move.b    -(a0),d1               # [82]
	lea      -1(%esi),%esi
	movb     (%esi),%dl
#;;   addx.b    d1,d1                  # [83 X]
	btw      $0,xflag
	adcb     %dl,%dl
	setcb    xflag
UMAXP12:
#;;   addx.b    d0,d0                  # [84]
	btw      $0,xflag
	adcb     %bl,%bl
#;;   dbra      d7,umaxp11             # [85]
	decw     d7_w
	cmpw     $-1,d7_w
	jne      UMAXP11
#;;   addq.b    #6,d0                  # [86]
	addb     $6,%bl
#;;   moveq     #3,d7                  # [87]
	movl     $3,d7_l
#;;   bra.b     umaxp14                # [88]
	jmp      UMAXP14
UMAXP13:
#;;   moveq     #5,d0                  # [90]
	movl     $5,%ebx
#;;   moveq     #3,d7                  # [91]
	movl     $3,d7_l
#;;   bra.b     umaxp14                # [92]
	jmp      UMAXP14
#;;   moveq     #4,d0                  # [94]
	movl     $4,%ebx
#;;   moveq     #2,d7                  # [95]
	movl     $2,d7_l
#;;   bra.b     umaxp14                # [96]
	jmp      UMAXP14
#;;   moveq     #3,d0                  # [98]
	movl     $3,%ebx
#;;   moveq     #1,d7                  # [99]
	movl     $1,d7_l
#;;   bra.b     umaxp14                # [100]
	jmp      UMAXP14
#;;   moveq     #2,d0                  # [102]
	movl     $2,%ebx
#;;   moveq     #0,d7                  # [103]
	movl     $0,d7_l
UMAXP14:
#;;   move.w    d7,d3                  # [104]
	movw     d7_w,%ax
	movw     %ax,d3_w
#;;   add.b     d1,d1                  # [105 CC NE X]
	addb     %dl,%dl
	setcb    xflag
#;;   bne.b     umaxp15                # [106]
	jne      UMAXP15
#;;   move.b    -(a0),d1               # [107]
	lea      -1(%esi),%esi
	movb     (%esi),%dl
#;;   addx.b    d1,d1                  # [108 CC]
	btw      $0,xflag
	adcb     %dl,%dl
UMAXP15:
#;;   bcc.b     umaxp17                # [109]
	jnc      UMAXP17
#;;   add.b     d1,d1                  # [110 CC NE X]
	addb     %dl,%dl
	setcb    xflag
#;;   bne.b     umaxp16                # [111]
	jne      UMAXP16
#;;   move.b    -(a0),d1               # [112]
	lea      -1(%esi),%esi
	movb     (%esi),%dl
#;;   addx.b    d1,d1                  # [113 CC]
	btw      $0,xflag
	adcb     %dl,%dl
UMAXP16:
#;;   bcc.b     umaxp19                # [114]
	jnc      UMAXP19
#;;   moveq     #0,d6                  # [115]
	movl     $0,d6_l
#;;   move.b    8(a5,d7.w),d6          # [116]
	movswl   d7_w,%ecx
	addl     a5_l,%ecx
	movb     8(%ecx),%al
	movb     %al,d6_b
#;;   addq.b    #4,d7                  # [117]
	addb     $4,d7_b
#;;   bra.b     umaxp20                # [118]
	jmp      UMAXP20
UMAXP17:
#;;   add.b     d1,d1                  # [120 NE X]
	addb     %dl,%dl
	setcb    xflag
#;;   bne.b     umaxp18                # [121]
	jne      UMAXP18
#;;   move.b    -(a0),d1               # [122]
	lea      -1(%esi),%esi
	movb     (%esi),%dl
#;;   addx.b    d1,d1                  # [123 X]
	btw      $0,xflag
	adcb     %dl,%dl
	setcb    xflag
UMAXP18:
#;;   addx.w    d2,d2                  # [124]
	movw     d2_w,%ax
	btw      $0,xflag
	adcw     %ax,d2_w
#;;   bra.b     umaxp23                # [125]
	jmp      UMAXP23
UMAXP19:
#;;   moveq     #2,d6                  # [127]
	movl     $2,d6_l
UMAXP20:
#;;   move.b    12(a5,d7.w),d7         # [128]
	movswl   d7_w,%ecx
	addl     a5_l,%ecx
	movb     12(%ecx),%al
	movb     %al,d7_b
UMAXP21:
#;;   add.b     d1,d1                  # [129 NE X]
	addb     %dl,%dl
	setcb    xflag
#;;   bne.b     umaxp22                # [130]
	jne      UMAXP22
#;;   move.b    -(a0),d1               # [131]
	lea      -1(%esi),%esi
	movb     (%esi),%dl
#;;   addx.b    d1,d1                  # [132 X]
	btw      $0,xflag
	adcb     %dl,%dl
	setcb    xflag
UMAXP22:
#;;   addx.w    d2,d2                  # [133]
	movw     d2_w,%ax
	btw      $0,xflag
	adcw     %ax,d2_w
#;;   subq.b    #1,d7                  # [134 NE]
	subb     $1,d7_b
#;;   bne.b     umaxp21                # [135]
	jne      UMAXP21
#;;   add.w     d6,d2                  # [136]
	movw     d6_w,%ax
	addw     %ax,d2_w
UMAXP23:
#;;   moveq     #0,d6                  # [137]
	movl     $0,d6_l
#;;   move.w    d6,a4                  # [138]
	movswl   d6_w,%eax
#;;   move.w    d3,d7                  # [139]
	movl     %eax,a4_l
	movw     d3_w,%ax
	movw     %ax,d7_w
#;;   add.b     d1,d1                  # [140 CC NE X]
	addb     %dl,%dl
	setcb    xflag
#;;   bne.b     umaxp24                # [141]
	jne      UMAXP24
#;;   move.b    -(a0),d1               # [142]
	lea      -1(%esi),%esi
	movb     (%esi),%dl
#;;   addx.b    d1,d1                  # [143 CC]
	btw      $0,xflag
	adcb     %dl,%dl
UMAXP24:
#;;   bcc.b     umaxp27                # [144]
	jnc      UMAXP27
#;;   add.w     d3,d3                  # [145]
	movw     d3_w,%ax
	addw     %ax,d3_w
#;;   add.b     d1,d1                  # [146 CC NE X]
	addb     %dl,%dl
	setcb    xflag
#;;   bne.b     umaxp25                # [147]
	jne      UMAXP25
#;;   move.b    -(a0),d1               # [148]
	lea      -1(%esi),%esi
	movb     (%esi),%dl
#;;   addx.b    d1,d1                  # [149 CC]
	btw      $0,xflag
	adcb     %dl,%dl
UMAXP25:
#;;   bcc.b     umaxp26                # [150]
	jnc      UMAXP26
#;;   move.w    20(a5,d3.w),a4         # [151]
	movswl   d3_w,%ecx
	addl     a5_l,%ecx
	movw     20(%ecx),%cx
	movswl   %cx,%eax
#;;   addq.b    #8,d7                  # [152]
	addb     $8,d7_b
	movl     %eax,a4_l
#;;   bra.b     umaxp27                # [153]
	jmp      UMAXP27
UMAXP26:
#;;   move.w    28(a5,d3.w),a4         # [155]
	movswl   d3_w,%ecx
	addl     a5_l,%ecx
	movw     28(%ecx),%cx
	movswl   %cx,%eax
#;;   addq.b    #4,d7                  # [156]
	addb     $4,d7_b
	movl     %eax,a4_l
UMAXP27:
#;;   move.b    36(a5,d7.w),d7         # [157 PL]
	movswl   d7_w,%ecx
	addl     a5_l,%ecx
	movb     36(%ecx),%al
	movb     %al,d7_b
	testb    %al,%al
#;;   bpl.b     umaxp28                # [158]
	jns      UMAXP28
#;;   move.b    -(a0),d6               # [159]
	lea      -1(%esi),%esi
	movb     (%esi),%al
	movb     %al,d6_b
#;;   and.b     #$0f,d7                # [160 EQ]
	andb     $0xf,d7_b
#;;   beq.b     umaxp30                # [161]
	je       UMAXP30
UMAXP28:
#;;   add.b     d1,d1                  # [162 NE X]
	addb     %dl,%dl
	setcb    xflag
#;;   bne.b     umaxp29                # [163]
	jne      UMAXP29
#;;   move.b    -(a0),d1               # [164]
	lea      -1(%esi),%esi
	movb     (%esi),%dl
#;;   addx.b    d1,d1                  # [165 X]
	btw      $0,xflag
	adcb     %dl,%dl
	setcb    xflag
UMAXP29:
#;;   addx.w    d6,d6                  # [166]
	movw     d6_w,%ax
	btw      $0,xflag
	adcw     %ax,d6_w
#;;   subq.b    #1,d7                  # [167 NE]
	subb     $1,d7_b
#;;   bne.b     umaxp28                # [168]
	jne      UMAXP28
UMAXP30:
#;;   addq.w    #1,d6                  # [170]
	addw     $1,d6_w
#;;   add.w     d6,a4                  # [171]
	movswl   d6_w,%eax
	addl     %eax,a4_l
#;;   add.l     a3,a4                  # [172]
	movl     a3_l,%eax
	addl     %eax,a4_l
UMAXP31:
#;;   cmp.l     a4,a6                  # [173 NE]
	cmpl     a4_l,%ebp
#;;   bne.b     umaxp32                # [174]
	jne      UMAXP32
#;;   move.l    a2,a4                  # [175]
	movl     a2_l,%eax
	movl     %eax,a4_l
UMAXP32:
#;;   subq.w    #1,d5                  # [176 MI]
	subw     $1,d5_w
#;;   bmi.w     umaxp1                 # [177]
	js       UMAXP1
#;;   move.b    -(a4),-(a3)            # [178]
	subl     $1,a4_l
	subl     $1,a3_l
	movl     a4_l,%ecx
	movb     (%ecx),%al
	movl     a3_l,%ecx
	movb     %al,(%ecx)
#;;   subq.b    #1,d0                  # [179 NE]
	subb     $1,%bl
#;;   bne.b     umaxp31                # [180]
	jne      UMAXP31
#;;   bra.w     umaxp2                 # [181]
	jmp      UMAXP2
