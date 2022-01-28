    .text
    .globl  stonecracker3
    .globl  _stonecracker3
stonecracker3:
_stonecracker3:
    movl 4(%esp),%eax
    movl %eax,Source

    movl 8(%esp),%eax
    movl %eax,Dest

    movl 12(%esp),%eax
    movl %eax,Temp

#;;   lea       Dest,a4                # [1]
	movl     $Dest,a4_l
#;;   lea       Source,a3              # [2]
	movl     $Source,a3_l
#;;   bne       error                  # [4]
	jne      error
#;;   lea       Temp,a2                # [5]
	movl     $Temp,a2_l
#;;   lea       16(a3),a5              # [6]
	movl     a3_l,%ecx
	lea      16(%ecx),%eax
#;;   move.l    a4,a0                  # [7]
	movl     a4_l,%esi
#;;   add.l     8(a3),a0               # [8]
	addl     8(%ecx),%esi
#;;   add.l     12(a3),a5              # [9]
	movl     %eax,a5_l
	movl     12(%ecx),%eax
	addl     %eax,a5_l
#;;   moveq     #127,d3                # [10]
	movl     $127,d3_l
#;;   moveq     #0,d4                  # [11]
	movl     $0,d4_l
#;;   moveq     #3,d5                  # [12]
	movl     $3,d5_l
#;;   moveq     #7,d6                  # [13]
	movl     $7,d6_l
#;;   move.b    7(a3),d4               # [14]
	movb     7(%ecx),%al
	movb     %al,d4_b
#;;   move.l    -(a5),d7               # [16]
	subl     $4,a5_l
	movl     a5_l,%ecx
	movl     (%ecx),%eax
	movl     %eax,d7_l
stc1:
#;;   lsr.l     #1,d7                  # [17 CC NE X]
	shrl     $1,d7_l
	setcb    xflag
#;;   bne.s     stc2                   # [18]
	jne      stc2
#;;   move.l    -(a5),d7               # [19]
	subl     $4,a5_l
	movl     a5_l,%ecx
	movl     (%ecx),%eax
	movl     %eax,d7_l
#;;   roxr.l    #1,d7                  # [20 CC]
	btw      $0,xflag
	rcrl     $1,d7_l
stc2:
#;;   bcc.s     stc7                   # [21]
	jnc      stc7
#;;   moveq     #1,d2                  # [22]
	movl     $1,d2_l
stc3:
#;;   moveq     #2,d1                  # [23]
	movl     $2,%edx
#;;   add.l     d0,d2                  # [25]
	addl     %ebx,d2_l
#;;   cmp       d6,d0                  # [26 EQ]
	cmpw     d6_w,%bx
#;;   beq.s     stc3                   # [27]
	je       stc3
stc4:
#;;   moveq     #7,d1                  # [28]
	movl     $7,%edx
stc5:
#;;   lsr.l     #1,d7                  # [29 NE X]
	shrl     $1,d7_l
	setcb    xflag
#;;   bne.s     stc6                   # [30]
	jne      stc6
#;;   move.l    -(a5),d7               # [31]
	subl     $4,a5_l
	movl     a5_l,%ecx
	movl     (%ecx),%eax
	movl     %eax,d7_l
#;;   move      d7,(a2)                # [32]
	movw     d7_w,%ax
	movl     a2_l,%ecx
	movw     %ax,(%ecx)
#;;   roxr.l    #1,d7                  # [33 X]
	btw      $0,xflag
	rcrl     $1,d7_l
	setcb    xflag
stc6:
#;;   roxr.b    #1,d0                  # [34]
	btw      $0,xflag
	rcrb     $1,%bl
#;;   dbf       d1,stc5                # [35]
	decw     %dx
	cmpw     $-1,%dx
	jne      stc5
#;;   move.b    d0,-(a0)               # [36]
	lea      -1(%esi),%esi
	movb     %bl,(%esi)
#;;   subq.l    #1,d2                  # [37 NE]
	subl     $1,d2_l
#;;   bne.s     stc4                   # [38]
	jne      stc4
#;;   bra.s     stc13                  # [39]
	jmp      stc13
stc7:
#;;   moveq     #1,d1                  # [40]
	movl     $1,%edx
#;;   moveq     #0,d1                  # [42]
	movl     $0,%edx
#;;   move.l    d0,d2                  # [43]
	movl     %ebx,d2_l
#;;   move.b    4(a3,d0.w),d1          # [44]
	movswl   %bx,%ecx
	addl     a3_l,%ecx
	movb     4(%ecx),%dl
#;;   cmp       d5,d0                  # [45 NE]
	cmpw     d5_w,%bx
#;;   bne.s     stc11                  # [46]
	jne      stc11
#;;   lsr.l     #1,d7                  # [47 CS NE X]
	shrl     $1,d7_l
	setcb    xflag
#;;   bne.s     stc8                   # [48]
	jne      stc8
#;;   move.l    -(a5),d7               # [49]
	subl     $4,a5_l
	movl     a5_l,%ecx
	movl     (%ecx),%eax
	movl     %eax,d7_l
#;;   roxr.l    #1,d7                  # [50 CS]
	btw      $0,xflag
	rcrl     $1,d7_l
stc8:
#;;   bcs.s     stc10                  # [51]
	jc       stc10
stc9:
#;;   moveq     #6,d1                  # [52]
	movl     $6,%edx
#;;   add.l     d0,d2                  # [54]
	addl     %ebx,d2_l
#;;   cmp       d3,d0                  # [55 EQ]
	cmpw     d3_w,%bx
#;;   beq.s     stc9                   # [56]
	je       stc9
#;;   move      d4,d1                  # [57]
	movw     d4_w,%dx
#;;   bra.s     stc11                  # [58]
	jmp      stc11
stc10:
#;;   moveq     #2,d1                  # [59]
	movl     $2,%edx
#;;   add.l     d0,d2                  # [61]
	addl     %ebx,d2_l
#;;   cmp       d6,d0                  # [62 EQ]
	cmpw     d6_w,%bx
#;;   beq.s     stc10                  # [63]
	je       stc10
#;;   moveq     #7,d1                  # [64]
	movl     $7,%edx
stc11:
#;;   addq.l    #1,d2                  # [65]
	addl     $1,d2_l
stc12:
#;;   move.b    (a0,d0.w),-(a0)        # [67]
	lea      -1(%esi),%esi
	movswl   %bx,%ecx
	movb     0(%esi,%ecx),%al
	movb     %al,(%esi)
#;;   subq.l    #1,d2                  # [68 PL]
	subl     $1,d2_l
#;;   bpl.s     stc12                  # [69]
	jns      stc12
stc13:
#;;   cmp.l     a0,a4                  # [70 CS]
	cmpl     %esi,a4_l
#;;   blo.s     stc1                   # [71]
	jc       stc1
error:
#;;   rts                              # [72]
	ret      
stc14:
#;;   moveq     #0,d0                  # [73]
	movl     $0,%ebx
stc15:
#;;   lsr.l     #1,d7                  # [74 NE X]
	shrl     $1,d7_l
	setcb    xflag
#;;   bne.s     stc16                  # [75]
	jne      stc16
#;;   move.l    -(a5),d7               # [76]
	subl     $4,a5_l
	movl     a5_l,%ecx
	movl     (%ecx),%eax
	movl     %eax,d7_l
#;;   move      d7,(a2)                # [77]
	movw     d7_w,%ax
	movl     a2_l,%ecx
	movw     %ax,(%ecx)
#;;   roxr.l    #1,d7                  # [78 X]
	btw      $0,xflag
	rcrl     $1,d7_l
	setcb    xflag
stc16:
#;;   addx.l    d0,d0                  # [79]
	btw      $0,xflag
	adcl     %ebx,%ebx
#;;   dbf       d1,stc15               # [80]
	decw     %dx
	cmpw     $-1,%dx
	jne      stc15
#;;   rts                              # [81]
	ret      
