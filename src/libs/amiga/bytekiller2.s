    .text
    .globl ByteKiller2
    .globl _ByteKiller2
ByteKiller2:
_ByteKiller2:
    movl 4(%esp),%eax
    movl %eax,Source

    movl 8(%esp),%eax
    movl %eax,Dest

    movl 12(%esp),%eax
    movl %eax,Temp

#;;   movem.l   d0-d7/a0-a6,-(a7)      # [1]
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
#;;   lea.l     Temp,a6                # [2]
	movl     $Temp,%ebp
#;;   lea.l     Source,a0              # [3]
	movl     $Source,%esi
#;;   lea.l     Dest,a1                # [4]
	movl     $Dest,%edi
#;;   move.l    (a0)+,d0               # [5]
	movl     0(%esi),%ebx
#;;   move.l    (a0)+,d1               # [6]
	movl     4(%esi),%edx
#;;   move.l    (a0)+,d5               # [7]
	movl     8(%esi),%eax
	movl     %eax,d5_l
	lea      12(%esi),%esi
#;;   move.l    a1,a2                  # [8]
	movl     %edi,a2_l
#;;   add.l     d0,a0                  # [9]
	lea      (%esi,%ebx),%esi
#;;   add.l     d1,a2                  # [10]
	addl     %edx,a2_l
#;;   move.l    -(a0),d0               # [11]
	lea      -4(%esi),%esi
	movl     (%esi),%ebx
#;;   eor.l     d0,d5                  # [12]
	xorl     %ebx,d5_l
lbC0007F8:
#;;   lsr.l     #1,d0                  # [13 CS NE]
	shrl     $1,%ebx
#;;   bne.s     lbC000800              # [14]
	jne      lbC000800
#;;   bsr.s     lbC0008A0              # [15 (CS)]
	call     lbC0008A0
lbC000800:
#;;   bcs.s     lbC00083E              # [16]
	jc       lbC00083E
#;;   moveq     #8,d1                  # [17]
	movl     $8,%edx
#;;   moveq     #1,d3                  # [18]
	movl     $1,d3_l
#;;   lsr.l     #1,d0                  # [19 CS NE]
	shrl     $1,%ebx
#;;   bne.s     lbC00080E              # [20]
	jne      lbC00080E
#;;   bsr.s     lbC0008A0              # [21 (CS)]
	call     lbC0008A0
lbC00080E:
#;;   bcs.s     lbC00086A              # [22]
	jc       lbC00086A
#;;   moveq     #3,d1                  # [23]
	movl     $3,%edx
#;;   clr.w     d4                     # [24]
	movw     $0,d4_w
lbC000814:
#;;   bsr.s     lbC0008AC              # [25]
	call     lbC0008AC
#;;   move.w    d2,d3                  # [26]
	movw     d2_w,%ax
	movw     %ax,d3_w
#;;   add.w     d4,d3                  # [27]
	movw     d4_w,%ax
	addw     %ax,d3_w
lbC00081C:
#;;   moveq     #7,d1                  # [28]
	movl     $7,%edx
lbC00081E:
#;;   lsr.l     #1,d0                  # [29 NE X]
	shrl     $1,%ebx
	setcb    xflag
#;;   bne.s     lbC000826              # [30]
	jne      lbC000826
#;;   bsr.s     lbC0008A0              # [31 (X)]
	call     lbC0008A0
	setcb    xflag
lbC000826:
#;;   roxl.l    #1,d2                  # [32]
	btw      $0,xflag
	rcll     $1,d2_l
#;;   dbra      d1,lbC00081E           # [33]
	decw     %dx
	cmpw     $-1,%dx
	jne      lbC00081E
#;;   move.b    d2,-(a2)               # [34]
	movb     d2_b,%al
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     %al,(%ecx)
#;;   dbra      d3,lbC00081C           # [35]
	decw     d3_w
	cmpw     $-1,d3_w
	jne      lbC00081C
#;;   bra.s     lbC000878              # [36]
	jmp      lbC000878
lbC000836:
#;;   moveq     #8,d1                  # [38]
	movl     $8,%edx
#;;   moveq     #8,d4                  # [39]
	movl     $8,d4_l
#;;   bra.s     lbC000814              # [40]
	jmp      lbC000814
lbC00083E:
#;;   moveq     #2,d1                  # [42]
	movl     $2,%edx
#;;   bsr.s     lbC0008AC              # [43]
	call     lbC0008AC
#;;   cmp.b     #2,d2                  # [44 LT]
	cmpb     $2,d2_b
#;;   blt.s     lbC000860              # [45]
	jl       lbC000860
#;;   cmp.b     #3,d2                  # [46 EQ]
	cmpb     $3,d2_b
#;;   beq.s     lbC000836              # [47]
	je       lbC000836
#;;   moveq     #8,d1                  # [48]
	movl     $8,%edx
#;;   bsr.s     lbC0008AC              # [49]
	call     lbC0008AC
#;;   move.w    d2,d3                  # [50]
	movw     d2_w,%ax
	movw     %ax,d3_w
#;;   move.w    #$C,d1                 # [51]
	movw     $0xc,%dx
#;;   bra.s     lbC00086A              # [52]
	jmp      lbC00086A
lbC000860:
#;;   move.w    #9,d1                  # [54]
	movw     $9,%dx
#;;   add.w     d2,d1                  # [55]
	addw     d2_w,%dx
#;;   addq.w    #2,d2                  # [56]
	addw     $2,d2_w
#;;   move.w    d2,d3                  # [57]
	movw     d2_w,%ax
	movw     %ax,d3_w
lbC00086A:
#;;   bsr.s     lbC0008AC              # [58]
	call     lbC0008AC
lbC00086E:
#;;   subq.w    #1,a2                  # [59]
	subl     $1,a2_l
#;;   move.b    0(a2,d2.w),(a2)        # [60]
	movswl   d2_w,%ecx
	addl     a2_l,%ecx
	movb     0(%ecx),%al
	movl     a2_l,%ecx
	movb     %al,(%ecx)
#;;   dbra      d3,lbC00086E           # [61]
	decw     d3_w
	cmpw     $-1,d3_w
	jne      lbC00086E
lbC000878:
#;;   move.w    a0,(a6)                # [62]
	movw     %si,(%ebp)
#;;   cmp.l     a2,a1                  # [63 LT]
	cmpl     a2_l,%edi
#;;   blt.s     lbC0007F8              # [64]
	jl       lbC0007F8
#;;   movem.l   (a7)+,d0-d7/a0-a6      # [65]
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
#;;   rts                              # [66]
	ret      
lbC0008A0:
#;;   move.l    -(a0),d0               # [68]
	lea      -4(%esi),%esi
	movl     (%esi),%ebx
#;;   eor.l     d0,d5                  # [69]
	xorl     %ebx,d5_l
#;;   move.w    #$10,CCR               # [70 X]
	movb     $((0x10)>>4)&1,xflag
#;;   roxr.l    #1,d0                  # [71 CS X]
	btw      $0,xflag
	rcrl     $1,%ebx
	setcb    xflag
#;;   rts                              # [72]
	ret      
lbC0008AC:
#;;   subq.w    #1,d1                  # [74]
	subw     $1,%dx
#;;   clr.w     d2                     # [75]
	movw     $0,d2_w
lbC0008B0:
#;;   lsr.l     #1,d0                  # [76 NE X]
	shrl     $1,%ebx
	setcb    xflag
#;;   bne.s     lbC0008BE              # [77]
	jne      lbC0008BE
#;;   move.l    -(a0),d0               # [78]
	lea      -4(%esi),%esi
	movl     (%esi),%ebx
#;;   eor.l     d0,d5                  # [79]
	xorl     %ebx,d5_l
#;;   move.w    #$10,CCR               # [80 X]
	movb     $((0x10)>>4)&1,xflag
#;;   roxr.l    #1,d0                  # [81 X]
	btw      $0,xflag
	rcrl     $1,%ebx
	setcb    xflag
lbC0008BE:
#;;   roxl.l    #1,d2                  # [82]
	btw      $0,xflag
	rcll     $1,d2_l
#;;   dbra      d1,lbC0008B0           # [83]
	decw     %dx
	cmpw     $-1,%dx
	jne      lbC0008B0
#;;   rts                              # [84]
	ret      
