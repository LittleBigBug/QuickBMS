    .text
    .globl stonecracker2
    .globl _stonecracker2
stonecracker2:
_stonecracker2:
    movl 4(%esp),%eax
    movl %eax,Source

    movl 8(%esp),%eax
    movl %eax,Dest

#;;   lea       Source,a4              # [3]
	movl     $Source,a4_l
#;;   lea       12(a4),a5              # [4]
	movl     a4_l,%ecx
	lea      12(%ecx),%eax
#;;   lea       Dest,a0                # [5]
	movl     $Dest,%esi
#;;   add.l     8(a4),a5               # [6]
	movl     %eax,a5_l
	movl     8(%ecx),%eax
	addl     %eax,a5_l
#;;   move.l    a0,a3                  # [7]
	movl     %esi,a3_l
#;;   add.l     4(a4),a0               # [8]
	addl     4(%ecx),%esi
#;;   moveq     #127,d3                # [9]
	movl     $127,d3_l
#;;   moveq     #0,d4                  # [10]
	movl     $0,d4_l
#;;   moveq     #3,d5                  # [11]
	movl     $3,d5_l
#;;   moveq     #7,d6                  # [12]
	movl     $7,d6_l
#;;   move.b    3(a4),d4               # [13]
	movb     3(%ecx),%al
	movb     %al,d4_b
#;;   move.l    -(a5),d7               # [15]
	subl     $4,a5_l
	movl     a5_l,%ecx
	movl     (%ecx),%eax
	movl     %eax,d7_l
deloop:
#;;   lsr.l     #1,d7                  # [16 CC NE X]
	shrl     $1,d7_l
	setcb    xflag
#;;   bne.s     not_empty0             # [17]
	jne      not_empty0
#;;   move.l    -(a5),d7               # [18]
	subl     $4,a5_l
	movl     a5_l,%ecx
	movl     (%ecx),%eax
	movl     %eax,d7_l
#;;   roxr.l    #1,d7                  # [19 CC]
	btw      $0,xflag
	rcrl     $1,d7_l
not_empty0:
#;;   bcc.s     copydata               # [20]
	jnc      copydata
#;;   moveq     #0,d2                  # [21]
	movl     $0,d2_l
bytekpl:
#;;   move      d5,d1                  # [22]
	movw     d5_w,%dx
#;;   bsr.s     getbits                # [23]
	call     getbits
#;;   add       d0,d2                  # [24]
	addw     %bx,d2_w
#;;   cmp       d6,d0                  # [25 EQ]
	cmpw     d6_w,%bx
#;;   beq.s     bytekpl                # [26]
	je       bytekpl
#;;   subq      #1,d2                  # [27]
	subw     $1,d2_w
byteloop:
#;;   move      d6,d1                  # [28]
	movw     d6_w,%dx
bytebits:
#;;   lsr.l     #1,d7                  # [29 NE X]
	shrl     $1,d7_l
	setcb    xflag
#;;   bne.s     not_empty2             # [30]
	jne      not_empty2
#;;   move.l    -(a5),d7               # [31]
	subl     $4,a5_l
	movl     a5_l,%ecx
	movl     (%ecx),%eax
	movl     %eax,d7_l
#;;   roxr.l    #1,d7                  # [32 X]
	btw      $0,xflag
	rcrl     $1,d7_l
	setcb    xflag
not_empty2:
#;;   roxr.b    #1,d0                  # [33]
	btw      $0,xflag
	rcrb     $1,%bl
#;;   dbf       d1,bytebits            # [34]
	decw     %dx
	cmpw     $-1,%dx
	jne      bytebits
#;;   move.b    d0,-(a0)               # [35]
	lea      -1(%esi),%esi
	movb     %bl,(%esi)
#;;   dbf       d2,byteloop            # [36]
	decw     d2_w
	cmpw     $-1,d2_w
	jne      byteloop
#;;   bra.s     test                   # [37]
	jmp      test
copydata:
#;;   moveq     #2-1,d1                # [39]
	movl     $2-1,%edx
#;;   bsr.s     getfast                # [40]
	call     getfast
#;;   moveq     #0,d1                  # [41]
	movl     $0,%edx
#;;   move.l    d0,d2                  # [42]
	movl     %ebx,d2_l
#;;   move.b    0(a4,d0.w),d1          # [43]
	movswl   %bx,%ecx
	addl     a4_l,%ecx
	movb     0(%ecx),%dl
#;;   cmp       d5,d0                  # [44 NE]
	cmpw     d5_w,%bx
#;;   bne.s     copyfast               # [45]
	jne      copyfast
#;;   lsr.l     #1,d7                  # [46 CS NE X]
	shrl     $1,d7_l
	setcb    xflag
#;;   bne.s     not_empty3             # [47]
	jne      not_empty3
#;;   move.l    -(a5),d7               # [48]
	subl     $4,a5_l
	movl     a5_l,%ecx
	movl     (%ecx),%eax
	movl     %eax,d7_l
#;;   roxr.l    #1,d7                  # [49 CS]
	btw      $0,xflag
	rcrl     $1,d7_l
not_empty3:
#;;   bcs.s     copykpl                # [50]
	jc       copykpl
copykpl127:
#;;   move      d6,d1                  # [52]
	movw     d6_w,%dx
#;;   bsr.s     getbits                # [53]
	call     getbits
#;;   add       d0,d2                  # [54]
	addw     %bx,d2_w
#;;   cmp       d3,d0                  # [55 EQ]
	cmpw     d3_w,%bx
#;;   beq.s     copykpl127             # [56]
	je       copykpl127
#;;   add       d6,d2                  # [57]
	movw     d6_w,%ax
	addw     %ax,d2_w
#;;   add       d6,d2                  # [58]
	addw     %ax,d2_w
#;;   bra.s     copyskip               # [59]
	jmp      copyskip
copykpl:
#;;   move      d5,d1                  # [61]
	movw     d5_w,%dx
#;;   bsr.s     getbits                # [62]
	call     getbits
#;;   add       d0,d2                  # [63]
	addw     %bx,d2_w
#;;   cmp       d6,d0                  # [64 EQ]
	cmpw     d6_w,%bx
#;;   beq.s     copykpl                # [65]
	je       copykpl
copyskip:
#;;   move      d4,d1                  # [66]
	movw     d4_w,%dx
copyfast:
#;;   addq      #1,d2                  # [67]
	addw     $1,d2_w
#;;   bsr.s     getfast                # [68]
	call     getfast
copyloop:
#;;   move.b    0(a0,d0.w),-(a0)       # [69]
	lea      -1(%esi),%esi
	movswl   %bx,%ecx
	movb     0(%esi,%ecx),%al
	movb     %al,(%esi)
#;;   dbf       d2,copyloop            # [70]
	decw     d2_w
	cmpw     $-1,d2_w
	jne      copyloop
test:
#;;   cmp.l     a0,a3                  # [71 CS]
	cmpl     %esi,a3_l
#;;   blo.s     deloop                 # [72]
	jc       deloop
#;;   rts                              # [73]
	ret      
getbits:
#;;   subq      #1,d1                  # [75]
	subw     $1,%dx
getfast:
#;;   moveq     #0,d0                  # [76]
	movl     $0,%ebx
bitloop:
#;;   lsr.l     #1,d7                  # [77 NE X]
	shrl     $1,d7_l
	setcb    xflag
#;;   bne.s     not_empty1             # [78]
	jne      not_empty1
#;;   move.l    -(a5),d7               # [79]
	subl     $4,a5_l
	movl     a5_l,%ecx
	movl     (%ecx),%eax
	movl     %eax,d7_l
#;;   roxr.l    #1,d7                  # [81 X]
	btw      $0,xflag
	rcrl     $1,d7_l
	setcb    xflag
not_empty1:
#;;   addx.l    d0,d0                  # [82]
	btw      $0,xflag
	adcl     %ebx,%ebx
#;;   dbf       d1,bitloop             # [83]
	decw     %dx
	cmpw     $-1,%dx
	jne      bitloop
#;;   rts                              # [84]
	ret      
