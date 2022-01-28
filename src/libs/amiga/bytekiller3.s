    .text
    .globl DeCr00
    .globl _DeCr00
DeCr00:
_DeCr00:
    movl 4(%esp),%eax
    movl %eax,Source

    movl 8(%esp),%eax
    movl %eax,Dest

    movl 12(%esp),%eax
    movl %eax,Temp

#;;           section ByteKillerV3.0_DeCrunch,CODE # [8]
#DeCr00:
#;;   movem.l   d0-a6,-(a7)            # [10]
	pushl    %ebp
	pushl    a5_l
	pushl    a4_l
	pushl    a3_l
	pushl    a2_l
	pushl    %edi
	pushl    %esi
	pushl    d7_l
	pushl    d6_l
	pushl    d5_l
	pushl    d4_l
	pushl    d3_l
	pushl    d2_l
	pushl    %edx
	pushl    %ebx
#;;   lea       Source,a0              # [11]
	movl     $Source,%esi
#;;   lea       Dest,a1                # [13]
	movl     $Dest,%edi
#;;   lea       $dff180,a6             # [14]
	#screen flashing movl     $0xdff180,%ebp
#;;   move.l    (a0)+,d0               # [15]
	movl     0(%esi),%ebx
#;;   move.l    (a0)+,d1               # [16]
	movl     4(%esi),%edx
	lea      8(%esi),%esi
#;;   add.l     d0,a0                  # [17]
	lea      (%esi,%ebx),%esi
#;;   move.l    (a0),d0                # [18]
	movl     (%esi),%ebx
#;;   move.l    a1,a2                  # [19]
	movl     %edi,a2_l
#;;   add.l     d1,a2                  # [20]
	addl     %edx,a2_l
#;;   moveq     #3,d5                  # [21]
	movl     $3,d5_l
#;;   moveq     #2,d6                  # [22]
	movl     $2,d6_l
#;;   moveq     #$10,d7                # [23]
	movl     $0x10,d7_l
DeCr01:
#;;   lsr.l     #1,d0                  # [24 CS NE]
	shrl     $1,%ebx
#;;   bne.b     DeCr02                 # [25]
	jne      DeCr02
#;;   bsr.b     DeCr14                 # [26 (CS)]
	call     DeCr14
DeCr02:
#;;   bcs.b     DeCr09                 # [27]
	jc       DeCr09
#;;   moveq     #8,d1                  # [28]
	movl     $8,%edx
#;;   moveq     #1,d3                  # [29]
	movl     $1,d3_l
#;;   lsr.l     #1,d0                  # [30 CS NE]
	shrl     $1,%ebx
#;;   bne.b     DeCr03                 # [31]
	jne      DeCr03
#;;   bsr.b     DeCr14                 # [32 (CS)]
	call     DeCr14
DeCr03:
#;;   bcs.b     DeCr11                 # [33]
	jc       DeCr11
#;;   moveq     #3,d1                  # [34]
	movl     $3,%edx
#;;   moveq     #0,d4                  # [35]
	movl     $0,d4_l
DeCr04:
#;;   bsr.b     DeCr15                 # [36]
	call     DeCr15
#;;   move.w    d2,d3                  # [37]
	movw     d2_w,%ax
	movw     %ax,d3_w
#;;   add.w     d4,d3                  # [38]
	movw     d4_w,%ax
	addw     %ax,d3_w
DeCr05:
#;;   moveq     #7,d1                  # [39]
	movl     $7,%edx
DeCr06:
#;;   lsr.l     #1,d0                  # [40 NE X]
	shrl     $1,%ebx
	setcb    xflag
#;;   bne.b     DeCr07                 # [41]
	jne      DeCr07
#;;   bsr.b     DeCr14                 # [42 (X)]
	call     DeCr14
	setcb    xflag
DeCr07:
#;;   roxl.l    #1,d2                  # [43]
	btw      $0,xflag
	rcll     $1,d2_l
#;;   dbf       d1,DeCr06              # [44]
	decw     %dx
	cmpw     $-1,%dx
	jne      DeCr06
#;;   move.b    d2,-(a2)               # [45]
	movb     d2_b,%al
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     %al,(%ecx)
#;;   dbf       d3,DeCr05              # [46]
	decw     d3_w
	cmpw     $-1,d3_w
	jne      DeCr05
#;;   bra.b     DeCr13                 # [47]
	jmp      DeCr13
DeCr08:
#;;   moveq     #8,d1                  # [48]
	movl     $8,%edx
#;;   moveq     #8,d4                  # [49]
	movl     $8,d4_l
#;;   bra.b     DeCr04                 # [50]
	jmp      DeCr04
DeCr09:
#;;   moveq     #2,d1                  # [51]
	movl     $2,%edx
#;;   bsr.b     DeCr15                 # [52]
	call     DeCr15
#;;   cmp.b     d6,d2                  # [53 LT]
	movb     d6_b,%al
	cmpb     %al,d2_b
#;;   blt.b     DeCr10                 # [54]
	jl       DeCr10
#;;   cmp.b     d5,d2                  # [55 EQ]
	movb     d5_b,%al
	cmpb     %al,d2_b
#;;   beq.b     DeCr08                 # [56]
	je       DeCr08
#;;   moveq     #8,d1                  # [57]
	movl     $8,%edx
#;;   bsr.b     DeCr15                 # [58]
	call     DeCr15
#;;   move.w    d2,d3                  # [59]
	movw     d2_w,%ax
	movw     %ax,d3_w
#;;   moveq     #$c,d1                 # [60]
	movl     $0xc,%edx
#;;   bra.b     DeCr11                 # [61]
	jmp      DeCr11
DeCr10:
#;;   moveq     #9,d1                  # [62]
	movl     $9,%edx
#;;   add.w     d2,d1                  # [63]
	addw     d2_w,%dx
#;;   addq.w    #2,d2                  # [64]
	addw     $2,d2_w
#;;   move.w    d2,d3                  # [65]
	movw     d2_w,%ax
	movw     %ax,d3_w
#;;   move.b    d3,(a6)                # [66]
	movb     d3_b,%al
	#movb     %al,(%ebp)
DeCr11:
#;;   bsr.b     DeCr15                 # [67]
	call     DeCr15
DeCr12:
#;;   subq.w    #1,a2                  # [68]
	subl     $1,a2_l
#;;   move.b    0(a2,d2.w),(a2)        # [69]
	movswl   d2_w,%ecx
	addl     a2_l,%ecx
	movb     0(%ecx),%al
	movl     a2_l,%ecx
	movb     %al,(%ecx)
#;;   dbf       d3,DeCr12              # [70]
	decw     d3_w
	cmpw     $-1,d3_w
	jne      DeCr12
DeCr13:
#;;   cmpa.l    a2,a1                  # [71 LT]
	cmpl     a2_l,%edi
#;;   blt.b     DeCr01                 # [72]
	jl       DeCr01
#;;   movem.l   (a7)+,d0-a6            # [73]
	popl     %ebx
	popl     %edx
	popl     d2_l
	popl     d3_l
	popl     d4_l
	popl     d5_l
	popl     d6_l
	popl     d7_l
	popl     %esi
	popl     %edi
	popl     a2_l
	popl     a3_l
	popl     a4_l
	popl     a5_l
	popl     %ebp
#;;   jmp       JumpIn                 # [74]
	#jmp      JumpIn
#;;   moveq     #-1,d0                 # [75]
	movl     $-1,%ebx
#;;   rts                              # [76]
    movl %ebx,%eax
	ret      
DeCr14:
#;;   move.l    -(a0),d0               # [77]
	lea      -4(%esi),%esi
	movl     (%esi),%ebx
#;;   move.w    d7,ccr                 # [78 X]
	btw      $4,d7_w
	setcb    xflag
#;;   roxr.l    #1,d0                  # [79 CS X]
	btw      $0,xflag
	rcrl     $1,%ebx
	setcb    xflag
#;;   rts                              # [80]
	ret      
DeCr15:
#;;   subq.w    #1,d1                  # [81]
	subw     $1,%dx
#;;   moveq     #0,d2                  # [82]
	movl     $0,d2_l
DeCr16:
#;;   lsr.l     #1,d0                  # [83 NE X]
	shrl     $1,%ebx
	setcb    xflag
#;;   bne.b     DeCr17                 # [84]
	jne      DeCr17
#;;   move.l    -(a0),d0               # [85]
	lea      -4(%esi),%esi
	movl     (%esi),%ebx
#;;   move.w    d7,ccr                 # [86 X]
	btw      $4,d7_w
	setcb    xflag
#;;   roxr.l    #1,d0                  # [87 X]
	btw      $0,xflag
	rcrl     $1,%ebx
	setcb    xflag
DeCr17:
#;;   roxl.l    #1,d2                  # [88]
	btw      $0,xflag
	rcll     $1,d2_l
#;;   dbf       d1,DeCr16              # [89]
	decw     %dx
	cmpw     $-1,%dx
	jne      DeCr16
#;;   rts                              # [90]
	ret      
DeCrDt:
