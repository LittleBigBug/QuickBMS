    .text
    .globl UMAST31
    .globl _UMAST31
UMAST31:
_UMAST31:
    movl 4(%esp),%ebx   # d0
    movl 8(%esp),%esi   # a0




#;;   move.w    #$1d4,d2               # [1]
	movw     $0x1d4,d2_w
#;;   move.w    #$52,d3                # [2]
	movw     $0x52,d3_w
#;;   move.l    d0,a1                  # [4]
	movl     %ebx,%edi
#;;   lea       (a0,d3.w),a0           # [5]
	movswl   d3_w,%ecx
	lea      0(%esi,%ecx),%esi
#;;   lea       -$1e(sp),sp            # [6]
	lea      -0x1e(%esp),%esp
#;;   move.l    sp,a3                  # [7]
	movl     %esp,a3_l
#;;   move.l    $6c(a0),(a3)+          # [8]
	movl     0x6c(%esi),%eax
	movl     a3_l,%ecx
	movl     %eax,0(%ecx)
#;;   move.l    $70(a0),(a3)+          # [9]
	movl     0x70(%esi),%eax
	movl     %eax,4(%ecx)
#;;   move.l    $cc(a0),(a3)+          # [10]
	movl     0xcc(%esi),%eax
	movl     %eax,8(%ecx)
#;;   move.l    $d0(a0),(a3)+          # [11]
	movl     0xd0(%esi),%eax
	movl     %eax,12(%ecx)
#;;   move.w    $d4(a0),(a3)+          # [12]
	movw     0xd4(%esi),%ax
	movw     %ax,16(%ecx)
#;;   move.l    $112(a0),(a3)+         # [13]
	movl     0x112(%esi),%eax
	movl     %eax,18(%ecx)
#;;   move.l    $116(a0),(a3)+         # [14]
	movl     0x116(%esi),%eax
	movl     %eax,22(%ecx)
#;;   move.l    $11a(a0),(a3)+         # [15]
	movl     0x11a(%esi),%eax
	addl     $30,a3_l
	movl     %eax,26(%ecx)
#;;   move.l    sp,a3                  # [16]
	movl     %esp,a3_l
#;;   move.l    a2,a0                  # [17]
	movl     a2_l,%esi
#;;   bsr.b     mascr                  # [18]
	call     MASCR
#;;   lea       $1e(sp),sp             # [19]
	lea      0x1e(%esp),%esp
#;;   moveq     #-1,d0                 # [21]
	movl     $-1,%ebx
.OUT:
#;;   rts                              # [22]
	movl %ebx,%eax
	ret      
MASCR:
#;;   moveq     #-1,d7                 # [31]
	movl     $-1,d7_l
#;;   move.l    a1,a5                  # [32]
	movl     %edi,a5_l
#;;   move.l    (a0)+,d5               # [33]
	lodsl    
	movl     %eax,d5_l
#;;   add.l     d5,a1                  # [34]
	addl     d5_l,%edi
#;;   add.l     (a0),a0                # [35]
	addl     (%esi),%esi
#;;   subq.l    #4,a0                  # [36]
	lea      -4(%esi),%esi
#;;   tst.w     -(a0)                  # [37 PL]
	lea      -2(%esi),%esi
	testw    $0xffff,(%esi)
#;;   bpl.b     mascr1                 # [38]
	jns      MASCR1
#;;   subq.l    #1,a0                  # [39]
	lea      -1(%esi),%esi
MASCR1:
#;;   move.b    -(a0),d0               # [41]
	lea      -1(%esi),%esi
	movb     (%esi),%bl
MASCR2:
#;;   lsl.b     #1,d0                  # [42 CC NE X]
	shlb     $1,%bl
	setcb    xflag
#;;   bne.b     mascr3                 # [43]
	jne      MASCR3
#;;   move.b    -(a0),d0               # [44]
	lea      -1(%esi),%esi
	movb     (%esi),%bl
#;;   roxl.b    #1,d0                  # [45 CC]
	btw      $0,xflag
	rclb     $1,%bl
MASCR3:
#;;   bcc.b     mascr10                # [46]
	jnc      MASCR10
#;;   clr.w     d1                     # [47]
	xorw     %dx,%dx
#;;   lsl.b     #1,d0                  # [48 CC NE X]
	shlb     $1,%bl
	setcb    xflag
#;;   bne.b     mascr4                 # [49]
	jne      MASCR4
#;;   move.b    -(a0),d0               # [50]
	lea      -1(%esi),%esi
	movb     (%esi),%bl
#;;   roxl.b    #1,d0                  # [51 CC]
	btw      $0,xflag
	rclb     $1,%bl
MASCR4:
#;;   bcc.b     mascr9                 # [52]
	jnc      MASCR9
#;;   moveq     #3,d3                  # [54]
	movl     $3,d3_l
MASCR5:
#;;   clr.w     d1                     # [55]
	xorw     %dx,%dx
#;;   move.b    (a3,d3.w),d2           # [56]
	movswl   d3_w,%ecx
	addl     a3_l,%ecx
	movb     0(%ecx),%al
	movb     %al,d2_b
#;;   ext.w     d2                     # [57]
	movsbl   d2_b,%eax
	movw     %ax,d2_w
#;;   moveq     #-1,d4                 # [58]
	movl     $-1,d4_l
#;;   lsl.w     d2,d4                  # [59]
	movb     d2_b,%al
	movw     %ax,%cx
	shlw     %cl,d4_w
#;;   not.w     d4                     # [60]
	notw     d4_w
#;;   subq.w    #1,d2                  # [61]
	subw     $1,d2_w
MASCR6:
#;;   lsl.b     #1,d0                  # [62 NE X]
	shlb     $1,%bl
	setcb    xflag
#;;   bne.b     mascr7                 # [63]
	jne      MASCR7
#;;   move.b    -(a0),d0               # [64]
	lea      -1(%esi),%esi
	movb     (%esi),%bl
#;;   roxl.b    #1,d0                  # [65 X]
	btw      $0,xflag
	rclb     $1,%bl
	setcb    xflag
MASCR7:
#;;   roxl.w    #1,d1                  # [66]
	btw      $0,xflag
	rclw     $1,%dx
#;;   dbra      d2,mascr6              # [67]
	decw     d2_w
	cmpw     $-1,d2_w
	jne      MASCR6
#;;   tst.w     d3                     # [68 EQ]
	testw    $0xffff,d3_w
#;;   beq.b     mascr8                 # [69]
	je       MASCR8
#;;   cmp.w     d1,d4                  # [70 NE]
	cmpw     %dx,d4_w
#;;   dbne      d3,mascr5              # [71]
	jne      _PA_58_
	decw     d3_w
	cmpw     $-1,d3_w
	jne      MASCR5
_PA_58_:         
MASCR8:
#;;   move.b    4(a3,d3.w),d2          # [73]
	movswl   d3_w,%ecx
	addl     a3_l,%ecx
	movb     4(%ecx),%al
	movb     %al,d2_b
#;;   ext.w     d2                     # [74]
	movsbl   d2_b,%eax
	movw     %ax,d2_w
#;;   add.w     d2,d1                  # [75]
	addw     d2_w,%dx
MASCR9:
#;;   move.b    -(a0),-(a1)            # [76]
	lea      -1(%esi),%esi
	lea      -1(%edi),%edi
	movb     (%esi),%al
	movb     %al,(%edi)
#;;   cmp.l     a5,a1                  # [77 EQ]
	cmpl     a5_l,%edi
#;;   dbeq      d1,mascr9              # [78]
	je       _PA_64_
	decw     %dx
	cmpw     $-1,%dx
	jne      MASCR9
_PA_64_:         
#;;   cmp.w     d7,d1                  # [79 NE]
	cmpw     d7_w,%dx
#;;   bne.w     mascr28                # [80]
	jne      MASCR28
MASCR10:
#;;   moveq     #3,d2                  # [82]
	movl     $3,d2_l
MASCR11:
#;;   lsl.b     #1,d0                  # [83 CC NE X]
	shlb     $1,%bl
	setcb    xflag
#;;   bne.b     mascr12                # [84]
	jne      MASCR12
#;;   move.b    -(a0),d0               # [85]
	lea      -1(%esi),%esi
	movb     (%esi),%bl
#;;   roxl.b    #1,d0                  # [86 CC]
	btw      $0,xflag
	rclb     $1,%bl
MASCR12:
#;;   bcc.b     mascr13                # [87]
	jnc      MASCR13
#;;   dbra      d2,mascr11             # [88]
	decw     d2_w
	cmpw     $-1,d2_w
	jne      MASCR11
MASCR13:
#;;   clr.w     d1                     # [90]
	xorw     %dx,%dx
#;;   addq.w    #1,d2                  # [91]
	addw     $1,d2_w
#;;   move.b    8(a3,d2.w),d3          # [92 EQ]
	movswl   d2_w,%ecx
	addl     a3_l,%ecx
	movb     8(%ecx),%al
	movb     %al,d3_b
	testb    %al,%al
#;;   beq.b     mascr16                # [93]
	je       MASCR16
#;;   ext.w     d3                     # [94]
	movsbl   d3_b,%eax
	movw     %ax,d3_w
#;;   subq.w    #1,d3                  # [95]
	subw     $1,d3_w
MASCR14:
#;;   lsl.b     #1,d0                  # [96 NE X]
	shlb     $1,%bl
	setcb    xflag
#;;   bne.b     mascr15                # [97]
	jne      MASCR15
#;;   move.b    -(a0),d0               # [98]
	lea      -1(%esi),%esi
	movb     (%esi),%bl
#;;   roxl.b    #1,d0                  # [99 X]
	btw      $0,xflag
	rclb     $1,%bl
	setcb    xflag
MASCR15:
#;;   roxl.w    #1,d1                  # [100]
	btw      $0,xflag
	rclw     $1,%dx
#;;   dbra      d3,mascr14             # [101]
	decw     d3_w
	cmpw     $-1,d3_w
	jne      MASCR14
MASCR16:
#;;   move.b    13(a3,d2.w),d3         # [103]
	movswl   d2_w,%ecx
	addl     a3_l,%ecx
	movb     13(%ecx),%al
	movb     %al,d3_b
#;;   ext.w     d3                     # [104]
	movsbl   d3_b,%eax
	movw     %ax,d3_w
#;;   add.w     d3,d1                  # [105]
	addw     d3_w,%dx
#;;   cmp.w     #2,d1                  # [106 EQ]
	cmpw     $2,%dx
#;;   beq.b     mascr22                # [107]
	je       MASCR22
#;;   moveq     #1,d3                  # [108]
	movl     $1,d3_l
MASCR17:
#;;   lsl.b     #1,d0                  # [109 CC NE X]
	shlb     $1,%bl
	setcb    xflag
#;;   bne.b     mascr18                # [110]
	jne      MASCR18
#;;   move.b    -(a0),d0               # [111]
	lea      -1(%esi),%esi
	movb     (%esi),%bl
#;;   roxl.b    #1,d0                  # [112 CC]
	btw      $0,xflag
	rclb     $1,%bl
MASCR18:
#;;   bcc.b     mascr19                # [113]
	jnc      MASCR19
#;;   dbra      d3,mascr17             # [114]
	decw     d3_w
	cmpw     $-1,d3_w
	jne      MASCR17
MASCR19:
#;;   addq.w    #1,d3                  # [116]
	addw     $1,d3_w
#;;   clr.w     d2                     # [117]
	movw     $0,d2_w
#;;   move.b    18(a3,d3.w),d4         # [118]
	movswl   d3_w,%ecx
	addl     a3_l,%ecx
	movb     18(%ecx),%al
	movb     %al,d4_b
#;;   ext.w     d4                     # [119]
	movsbl   d4_b,%eax
	movw     %ax,d4_w
MASCR20:
#;;   lsl.b     #1,d0                  # [120 NE X]
	shlb     $1,%bl
	setcb    xflag
#;;   bne.b     mascr21                # [121]
	jne      MASCR21
#;;   move.b    -(a0),d0               # [122]
	lea      -1(%esi),%esi
	movb     (%esi),%bl
#;;   roxl.b    #1,d0                  # [123 X]
	btw      $0,xflag
	rclb     $1,%bl
	setcb    xflag
MASCR21:
#;;   roxl.w    #1,d2                  # [124]
	btw      $0,xflag
	rclw     $1,d2_w
#;;   dbra      d4,mascr20             # [125]
	decw     d4_w
	cmpw     $-1,d4_w
	jne      MASCR20
#;;   lsl.w     #1,d3                  # [126]
	shlw     $1,d3_w
#;;   add.w     22(a3,d3.w),d2         # [127]
	movswl   d3_w,%ecx
	addl     a3_l,%ecx
	movw     22(%ecx),%ax
	addw     %ax,d2_w
#;;   bra.b     mascr26                # [128]
	jmp      MASCR26
MASCR22:
#;;   clr.w     d2                     # [130]
	movw     $0,d2_w
#;;   moveq     #5,d3                  # [131]
	movl     $5,d3_l
#;;   clr.w     d4                     # [132]
	movw     $0,d4_w
#;;   lsl.b     #1,d0                  # [133 CC NE X]
	shlb     $1,%bl
	setcb    xflag
#;;   bne.b     mascr23                # [134]
	jne      MASCR23
#;;   move.b    -(a0),d0               # [135]
	lea      -1(%esi),%esi
	movb     (%esi),%bl
#;;   roxl.b    #1,d0                  # [136 CC]
	btw      $0,xflag
	rclb     $1,%bl
MASCR23:
#;;   bcc.b     mascr24                # [137]
	jnc      MASCR24
#;;   moveq     #8,d3                  # [138]
	movl     $8,d3_l
#;;   moveq     #64,d4                 # [139]
	movl     $64,d4_l
MASCR24:
#;;   lsl.b     #1,d0                  # [140 NE X]
	shlb     $1,%bl
	setcb    xflag
#;;   bne.b     mascr25                # [141]
	jne      MASCR25
#;;   move.b    -(a0),d0               # [142]
	lea      -1(%esi),%esi
	movb     (%esi),%bl
#;;   roxl.b    #1,d0                  # [143 X]
	btw      $0,xflag
	rclb     $1,%bl
	setcb    xflag
MASCR25:
#;;   roxl.w    #1,d2                  # [144]
	btw      $0,xflag
	rclw     $1,d2_w
#;;   dbra      d3,mascr24             # [145]
	decw     d3_w
	cmpw     $-1,d3_w
	jne      MASCR24
#;;   add.w     d4,d2                  # [146]
	movw     d4_w,%ax
	addw     %ax,d2_w
MASCR26:
#;;   lea       (a1,d2.w),a2           # [147]
	movswl   d2_w,%ecx
	lea      0(%edi,%ecx),%eax
#;;   ext.l     d1                     # [148]
	movswl   %dx,%edx
#;;   add.l     d1,a2                  # [149]
	movl     %eax,a2_l
	addl     %edx,a2_l
#;;   subq.w    #1,d1                  # [150]
	subw     $1,%dx
MASCR27:
#;;   move.b    -(a2),-(a1)            # [151]
	subl     $1,a2_l
	lea      -1(%edi),%edi
	movl     a2_l,%ecx
	movb     (%ecx),%al
	movb     %al,(%edi)
#;;   cmp.l     a5,a1                  # [152 EQ]
	cmpl     a5_l,%edi
#;;   dbeq      d1,mascr27             # [153]
	je       _PA_134_
	decw     %dx
	cmpw     $-1,%dx
	jne      MASCR27
_PA_134_:         
#;;   cmp.w     d7,d1                  # [154 EQ]
	cmpw     d7_w,%dx
#;;   beq.w     mascr2                 # [155]
	je       MASCR2
MASCR28:
#;;   rts                              # [156]
	ret      
