# a3 in
# a0 in + insz
# a1 out

    .text
    .globl PACIT
    .globl _PACIT
PACIT:
_PACIT:
    movl 4(%esp), %eax
    movl %eax, a3_l
    
    movl 8(%esp), %esi
    
    movl 12(%esp), %edi



#;;   moveq     #-1,d7                 # [1]
	movl     $-1,d7_l
#;;   move.l    -(a0),a2               # [2]
	lea      -8(%esi),%esi
	movl     4(%esi),%eax
	movl     %eax,a2_l
#;;   add.l     a1,a2                  # [3]
	addl     %edi,a2_l
#;;   move.l    -(a0),d0               # [4]
	movl     0(%esi),%ebx
PACIT1:
#;;   moveq     #0,d3                  # [5]
	movl     $0,d3_l
#;;   lsr.l     #1,d0                  # [6 CS NE]
	shrl     $1,%ebx
#;;   bne.b     pacit2                 # [7]
	jne      PACIT2
#;;   bsr.b     pacit9                 # [8 (CS)]
	call     PACIT9
PACIT2:
#;;   bcs.b     pacit7                 # [9]
	jc       PACIT7
PACIT3:
#;;   moveq     #2,d1                  # [10]
	movl     $2,%edx
#;;   bsr.b     pacit10                # [11]
	call     PACIT10
#;;   add.w     d2,d3                  # [12]
	movw     d2_w,%ax
	addw     %ax,d3_w
#;;   cmp.w     #3,d2                  # [13 EQ]
	cmpw     $3,d2_w
#;;   beq.b     pacit3                 # [14]
	je       PACIT3
PACIT4:
#;;   moveq     #7,d1                  # [15]
	movl     $7,%edx
PACIT5:
#;;   lsr.l     #1,d0                  # [16 NE X]
	shrl     $1,%ebx
	setcb    xflag
#;;   bne.b     pacit6                 # [17]
	jne      PACIT6
#;;   bsr.b     pacit9                 # [18 (X)]
	call     PACIT9
	setcb    xflag
PACIT6:
#;;   roxl.w    #1,d2                  # [19]
	btw      $0,xflag
	rclw     $1,d2_w
#;;   dbra      d1,pacit5              # [20]
	decw     %dx
	cmpw     $-1,%dx
	jne      PACIT5
#;;   move.b    d2,-(a2)               # [21]
	movb     d2_b,%al
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     %al,(%ecx)
#;;   cmp.l     a1,a2                  # [22 EQ]
	cmpl     %edi,a2_l
#;;   dbeq      d3,pacit4              # [23]
	je       _PA_22_
	decw     d3_w
	cmpw     $-1,d3_w
	jne      PACIT4
_PA_22_:         
#;;   cmp.w     d7,d3                  # [24 EQ]
	movw     d7_w,%ax
	cmpw     %ax,d3_w
#;;   beq.b     pacit7                 # [25]
	je       PACIT7
#;;   rts                              # [26]
	ret      
PACIT7:
#;;   moveq     #3,d1                  # [28]
	movl     $3,%edx
#;;   bsr.b     pacit10                # [29]
	call     PACIT10
#;;   cmp.b     #3,d2                  # [30 EQ]
	cmpb     $3,d2_b
#;;   beq.b     pacit13                # [31]
	je       PACIT13
#;;   cmp.b     #7,d2                  # [32 EQ]
	cmpb     $7,d2_b
#;;   beq.b     pacit13                # [33]
	je       PACIT13
#;;   move.b    (a3,d2.w),d1           # [34]
	movswl   d2_w,%ecx
	addl     a3_l,%ecx
	movb     0(%ecx),%dl
#;;   ext.w     d1                     # [35]
	movsbw   %dl,%dx
#;;   and.b     #3,d2                  # [36]
	andb     $3,d2_b
#;;   addq.w    #1,d2                  # [37]
	addw     $1,d2_w
#;;   move.w    d2,d3                  # [38]
	movw     d2_w,%ax
	movw     %ax,d3_w
#;;   bsr.b     pacit10                # [39]
	call     PACIT10
#;;   addq.w    #1,d2                  # [40]
	addw     $1,d2_w
PACIT8:
#;;   move.b    -1(a2,d2.w),-(a2)      # [41]
	subl     $1,a2_l
	movswl   d2_w,%ecx
	addl     a2_l,%ecx
	movb     -1(%ecx),%al
	movl     a2_l,%ecx
	movb     %al,(%ecx)
#;;   cmp.l     a1,a2                  # [42 EQ]
	cmpl     %edi,a2_l
#;;   dbeq      d3,pacit8              # [43]
	je       _PA_41_
	decw     d3_w
	cmpw     $-1,d3_w
	jne      PACIT8
_PA_41_:         
#;;   cmp.w     d7,d3                  # [44 EQ]
	movw     d7_w,%ax
	cmpw     %ax,d3_w
#;;   beq.b     pacit1                 # [45]
	je       PACIT1
#;;   rts                              # [46]
	ret      
PACIT9:
#;;   move.l    -(a0),d0               # [48]
	lea      -4(%esi),%esi
	movl     (%esi),%ebx
#;;   move.w    #$0010,ccr             # [49 X]
	movb     $((0x10)>>4)&1,xflag
#;;   roxr.l    #1,d0                  # [50 CS X]
	btw      $0,xflag
	rcrl     $1,%ebx
	setcb    xflag
#;;   rts                              # [51]
	ret      
PACIT10:
#;;   subq.w    #1,d1                  # [53]
	subw     $1,%dx
#;;   clr.w     d2                     # [54]
	movw     $0,d2_w
PACIT11:
#;;   lsr.l     #1,d0                  # [55 NE X]
	shrl     $1,%ebx
	setcb    xflag
#;;   bne.b     pacit12                # [56]
	jne      PACIT12
#;;   bsr.b     pacit9                 # [57 (X)]
	call     PACIT9
	setcb    xflag
PACIT12:
#;;   roxl.w    #1,d2                  # [58]
	btw      $0,xflag
	rclw     $1,d2_w
#;;   dbra      d1,pacit11             # [59]
	decw     %dx
	cmpw     $-1,%dx
	jne      PACIT11
#;;   rts                              # [60]
	ret      
PACIT13:
#;;   move.b    (a3,d2.w),d1           # [62]
	movswl   d2_w,%ecx
	addl     a3_l,%ecx
	movb     0(%ecx),%dl
#;;   ext.w     d1                     # [63]
	movsbw   %dl,%dx
#;;   bsr.b     pacit10                # [64]
	call     PACIT10
#;;   addq.w    #1,d2                  # [65]
	addw     $1,d2_w
#;;   move.w    d2,d3                  # [66]
	movw     d2_w,%ax
	movw     %ax,d3_w
#;;   moveq     #-1,d4                 # [67]
	movl     $-1,d4_l
PACIT14:
#;;   addq.l    #1,d4                  # [68]
	addl     $1,d4_l
#;;   moveq     #1,d1                  # [69]
	movl     $1,%edx
#;;   bsr.b     pacit10                # [70]
	call     PACIT10
#;;   tst.w     d2                     # [71 NE]
	testw    $0xffff,d2_w
#;;   bne.b     pacit14                # [72]
	jne      PACIT14
#;;   add.w     d4,d4                  # [73]
	movw     d4_w,%ax
	addw     %ax,d4_w
#;;   addq.l    #3,d4                  # [74]
	addl     $3,d4_l
#;;   move.w    d4,d1                  # [75]
	movw     d4_w,%dx
#;;   bsr.b     pacit10                # [76]
	call     PACIT10
#;;   addq.w    #4,d2                  # [77]
	addw     $4,d2_w
#;;   exg       d2,d3                  # [78]
	movl     d2_l,%eax
	xchgl    %eax,d3_l
	movl     %eax,d2_l
#;;   bra.b     pacit8                 # [79]
	jmp      PACIT8
