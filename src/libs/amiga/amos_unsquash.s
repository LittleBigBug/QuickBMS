    .text
    .globl UnSquash
    .globl _UnSquash
UnSquash:
_UnSquash:

movl 4(%esp), %eax
movl %eax,d3_l
movl 8(%esp), %edx




#;;   move.l    D3,D0                  # [17]
	movl     d3_l,%ebx
#;;   add.l     D0,D1                  # [18]
	addl     %ebx,%edx
#;;   movea.l   D1,A0                  # [19]
	movl     %edx,%esi
#;;   movea.l   D0,A1                  # [20]
	movl     %ebx,%edi
#;;   movea.l   -(A0),A2               # [21]
	lea      -12(%esi),%esi
	movl     8(%esi),%eax
	movl     %eax,a2_l
#;;   move.l    a2,d7                  # [22]
	movl     a2_l,%eax
	movl     %eax,d7_l
#;;   adda.l    A1,A2                  # [23]
	addl     %edi,a2_l
#;;   move.l    -(A0),D5               # [24]
	movl     4(%esi),%eax
	movl     %eax,d5_l
#;;   move.l    -(A0),D0               # [25]
	movl     0(%esi),%ebx
#;;   eor.l     D0,D5                  # [26]
	xorl     %ebx,d5_l
L22446E:
#;;   lsr.l     #1,D0                  # [31 CS NE X]
	shrl     $1,%ebx
	setcb    xflag
#;;   bne.s     L224476                # [32]
	jne      L224476
#;;   bsr       L2244E8                # [33 (CS)]
	call     L2244E8
L224476:
#;;   bcs.s     L2244AE                # [34]
	jc       L2244AE
#;;   moveq     #8,D1                  # [35]
	movl     $8,%edx
#;;   moveq     #1,D3                  # [36]
	movl     $1,d3_l
#;;   lsr.l     #1,D0                  # [37 CS NE X]
	shrl     $1,%ebx
	setcb    xflag
#;;   bne.s     L224484                # [38]
	jne      L224484
#;;   bsr       L2244E8                # [39 (CS)]
	call     L2244E8
L224484:
#;;   bcs.s     L2244D4                # [40]
	jc       L2244D4
#;;   moveq     #3,D1                  # [41]
	movl     $3,%edx
#;;   clr.w     D4                     # [42]
	movw     $0,d4_w
L22448A:
#;;   bsr       L2244F4                # [43]
	call     L2244F4
#;;   move.w    D2,D3                  # [44]
	movw     d2_w,%ax
	movw     %ax,d3_w
#;;   add.w     D4,D3                  # [45]
	movw     d4_w,%ax
	addw     %ax,d3_w
L224492:
#;;   moveq     #7,D1                  # [46]
	movl     $7,%edx
L224494:
#;;   lsr.l     #1,D0                  # [47 NE X]
	shrl     $1,%ebx
	setcb    xflag
#;;   bne.s     L22449A                # [48]
	jne      L22449A
#;;   bsr.s     L2244E8                # [49 (X)]
	call     L2244E8
	setcb    xflag
L22449A:
#;;   roxl.l    #1,D2                  # [50]
	btw      $0,xflag
	rcll     $1,d2_l
#;;   dbf       D1,L224494             # [51]
	decw     %dx
	cmpw     $-1,%dx
	jne      L224494
#;;   cmp.l     a1,a2                  # [52 LE]
	cmpl     %edi,a2_l
#;;   ble.b     bad_squash_mem         # [53]
	jle      bad_squash_mem
#;;   move.b    D2,-(A2)               # [54]
	movb     d2_b,%al
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     %al,(%ecx)
#;;   dbf       D3,L224492             # [55]
	decw     d3_w
	cmpw     $-1,d3_w
	jne      L224492
#;;   bra.s     L2244E0                # [56]
	jmp      L2244E0
L2244A8:
#;;   moveq     #8,D1                  # [57]
	movl     $8,%edx
#;;   moveq     #8,D4                  # [58]
	movl     $8,d4_l
#;;   bra.s     L22448A                # [59]
	jmp      L22448A
L2244AE:
#;;   moveq     #2,D1                  # [60]
	movl     $2,%edx
#;;   bsr.s     L2244F4                # [61]
	call     L2244F4
#;;   cmp.b     #2,D2                  # [62 LT]
	cmpb     $2,d2_b
#;;   blt.s     L2244CA                # [63]
	jl       L2244CA
#;;   cmp.b     #3,D2                  # [64 EQ]
	cmpb     $3,d2_b
#;;   beq.s     L2244A8                # [65]
	je       L2244A8
#;;   moveq     #8,D1                  # [66]
	movl     $8,%edx
#;;   bsr.s     L2244F4                # [67]
	call     L2244F4
#;;   move.w    D2,D3                  # [68]
	movw     d2_w,%ax
	movw     %ax,d3_w
#;;   move.w    #$C,D1                 # [69]
	movw     $0xc,%dx
#;;   bra.s     L2244D4                # [70]
	jmp      L2244D4
L2244CA:
#;;   move.w    #9,D1                  # [71]
	movw     $9,%dx
#;;   add.w     D2,D1                  # [72]
	addw     d2_w,%dx
#;;   addq.w    #2,D2                  # [73]
	addw     $2,d2_w
#;;   move.w    D2,D3                  # [74]
	movw     d2_w,%ax
	movw     %ax,d3_w
L2244D4:
#;;   bsr.s     L2244F4                # [75]
	call     L2244F4
L2244D6:
#;;   subq.w    #1,A2                  # [76]
	subl     $1,a2_l
#;;   cmp.l     a1,a2                  # [77 LT]
	cmpl     %edi,a2_l
#;;   blt.b     bad_squash_mem         # [78]
	jl       bad_squash_mem
#;;   move.b    0(A2,D2.W),(A2)        # [79]
	movswl   d2_w,%ecx
	addl     a2_l,%ecx
	movb     0(%ecx),%al
	movl     a2_l,%ecx
	movb     %al,(%ecx)
#;;   dbf       D3,L2244D6             # [80]
	decw     d3_w
	cmpw     $-1,d3_w
	jne      L2244D6
L2244E0:
#;;   cmpa.l    A2,A1                  # [81 LT]
	cmpl     a2_l,%edi
#;;   blt       L22446E                # [82]
	jl       L22446E
#;;   tst.l     d5                     # [83 EQ]
	testl    $0xffffffff,d5_l
#;;   beq.b     check_ok               # [84]
	je       check_ok
#;;   moveq.l   #-1,d3                 # [85]
	movl     $-1,d3_l
#;;   rts                              # [86]
    movl d3_l,%eax
	ret      
check_ok:
#;;   move.l    d7,d3                  # [88]
	movl     d7_l,%eax
	movl     %eax,d3_l
#;;   rts                              # [89]
    movl d3_l,%eax
	ret      
bad_squash_mem:
#;;   moveq.l   #-2,d3                 # [91]
	movl     $-2,d3_l
#;;   rts                              # [92]
    movl d3_l,%eax
	ret      
L2244E8:
#;;   move.l    -(A0),D0               # [94]
	lea      -4(%esi),%esi
	movl     (%esi),%ebx
#;;   eor.l     D0,D5                  # [95]
	xorl     %ebx,d5_l
#;;   roxr.l    #1,D0                  # [97 CS X]
	btw      $0,xflag
	rcrl     $1,%ebx
	setcb    xflag
#;;   rts                              # [98]
	ret      
L2244F4:
#;;   subq.w    #1,D1                  # [100]
	subw     $1,%dx
#;;   clr.w     D2                     # [101]
	movw     $0,d2_w
L2244F8:
#;;   lsr.l     #1,D0                  # [102 NE X]
	shrl     $1,%ebx
	setcb    xflag
#;;   bne.s     L224506                # [103]
	jne      L224506
#;;   move.l    -(A0),D0               # [104]
	lea      -4(%esi),%esi
	movl     (%esi),%ebx
#;;   eor.l     D0,D5                  # [105]
	xorl     %ebx,d5_l
#;;   roxr.l    #1,D0                  # [107 X]
	btw      $0,xflag
	rcrl     $1,%ebx
	setcb    xflag
L224506:
#;;   roxl.l    #1,D2                  # [108]
	btw      $0,xflag
	rcll     $1,d2_l
#;;   dbf       D1,L2244F8             # [109]
	decw     %dx
	cmpw     $-1,%dx
	jne      L2244F8
#;;   rts                              # [110]
	ret      
