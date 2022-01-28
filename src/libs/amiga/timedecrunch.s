#;*****************************************************
#;*    Timecruncher V2.1 Data Decruncher              *
#;*   Uses Registers D0,D1,D2,D3,A0,A1,A2             *
#;* a0=address for crunched data                      *
#;* a1=address where the decrunched data places       *
#;*                                                   *
#;* Improved and Fixed for data decrunching by        *
#;* Mr.Crook & Him of VOX DEI.                        *
#;* This source MUST be spread with the cruncher.     *
#;*****************************************************
    .text
    .globl time_decrunch
    .globl _time_decrunch
time_decrunch:
_time_decrunch:
    movl 4(%esp),%esi
    movl 8(%esp),%edi



#;;   move.l    a1,a2                  # [11]
	movl     %edi,a2_l
#;;   add.l     (a0)+,a2               # [12]
	lodsl    
	addl     %eax,a2_l
#;;   add.l     (a0),a0                # [13]
	addl     (%esi),%esi
#;;   subq.l    #8,a0                  # [14]
	lea      -8(%esi),%esi
#;;   move.l    (a0),d0                # [16]
	movl     (%esi),%ebx
again:
#;;   moveq     #3,d1                  # [17]
	movl     $3,%edx
#;;   bsr.w     d46                    # [18]
	call     d46
#;;   tst.b     d2                     # [19 EQ]
	testb    $0xff,d2_b
#;;   beq.b     caa                    # [20]
	je       caa
#;;   cmpi.w    #7,d2                  # [21 NE]
	cmpw     $7,d2_w
#;;   bne.b     c7a                    # [22]
	jne      c7a
#;;   lsr.l     #1,d0                  # [23 CC NE]
	shrl     $1,%ebx
#;;   bne.b     c5e                    # [24]
	jne      c5e
#;;   bsr.w     d3c                    # [25 (CC)]
	call     d3c
c5e:
#;;   bcc.b     c72                    # [26]
	jnc      c72
#;;   moveq     #10,d1                 # [27]
	movl     $10,%edx
#;;   bsr.w     d46                    # [28]
	call     d46
#;;   tst.w     d2                     # [29 NE]
	testw    $0xffff,d2_w
#;;   bne.b     c7a                    # [30]
	jne      c7a
#;;   moveq     #18,d1                 # [31]
	movl     $18,%edx
#;;   bsr.w     d46                    # [32]
	call     d46
#;;   bra.b     c7a                    # [33]
	jmp      c7a
c72:
#;;   moveq     #4,d1                  # [34]
	movl     $4,%edx
#;;   bsr.w     d46                    # [35]
	call     d46
#;;   addq.w    #7,d2                  # [36]
	addw     $7,d2_w
c7a:
#;;   subq.w    #1,d2                  # [37]
	subw     $1,d2_w
c7c:
#;;   moveq     #7,d1                  # [38]
	movl     $7,%edx
c7e:
#;;   lsr.l     #1,d0                  # [39 EQ X]
	shrl     $1,%ebx
	setcb    xflag
#;;   beq.b     c90                    # [40]
	je       c90
#;;   roxl.l    #1,d3                  # [41]
	btw      $0,xflag
	rcll     $1,d3_l
#;;   dbra      d1,c7e                 # [42]
	decw     %dx
	cmpw     $-1,%dx
	jne      c7e
#;;   move.b    d3,-(a2)               # [43]
	movb     d3_b,%al
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     %al,(%ecx)
#;;   dbra      d2,c7c                 # [44]
	decw     d2_w
	cmpw     $-1,d2_w
	jne      c7c
#;;   bra.b     caa                    # [45]
	jmp      caa
c90:
#;;   move.l    -(a0),d0               # [47]
	lea      -4(%esi),%esi
	movl     (%esi),%ebx
#;;   move.w    d0,$00dff180           # [48]
	movw     %bx,0xdff180
#;;   move      #$10,ccr               # [49 X]
	movb     $((0x10)>>4)&1,xflag
#;;   roxr.l    #1,d0                  # [50 X]
	btw      $0,xflag
	rcrl     $1,%ebx
#;;   roxl.l    #1,d3                  # [51]
	rcll     $1,d3_l
#;;   dbra      d1,c7e                 # [52]
	decw     %dx
	cmpw     $-1,%dx
	jne      c7e
#;;   move.b    d3,-(a2)               # [53]
	movb     d3_b,%al
	subl     $1,a2_l
	movl     a2_l,%ecx
	movb     %al,(%ecx)
#;;   dbra      d2,c7c                 # [54]
	decw     d2_w
	cmpw     $-1,d2_w
	jne      c7c
caa:
#;;   cmp.l     a2,a1                  # [56 GE]
	cmpl     a2_l,%edi
#;;   bge.w     exit                   # [57]
	jge      exit
#;;   moveq     #$02,d1                # [58]
	movl     $0x2,%edx
#;;   bsr.w     d46                    # [59]
	call     d46
#;;   moveq     #2,d3                  # [60]
	movl     $2,d3_l
#;;   moveq     #8,d1                  # [61]
	movl     $8,%edx
#;;   tst.w     d2                     # [62 EQ]
	testw    $0xffff,d2_w
#;;   beq.b     d26                    # [63]
	je       d26
#;;   moveq     #4,d3                  # [64]
	movl     $4,d3_l
#;;   cmpi.w    #2,d2                  # [65 EQ]
	cmpw     $2,d2_w
#;;   beq.w     d10                    # [66]
	je       d10
#;;   moveq     #3,d3                  # [67]
	movl     $3,d3_l
#;;   cmpi.w    #1,d2                  # [68 EQ]
	cmpw     $1,d2_w
#;;   beq.w     d02                    # [69]
	je       d02
#;;   moveq     #2,d1                  # [70]
	movl     $2,%edx
#;;   bsr.w     d46                    # [71]
	call     d46
#;;   cmpi.w    #3,d2                  # [72 EQ]
	cmpw     $3,d2_w
#;;   beq.b     cf8                    # [73]
	je       cf8
#;;   cmpi.w    #2,d2                  # [74 EQ]
	cmpw     $2,d2_w
#;;   beq.b     cec                    # [75]
	je       cec
#;;   addq.w    #5,d2                  # [76]
	addw     $5,d2_w
#;;   move.w    d2,d3                  # [77]
	movw     d2_w,%ax
	movw     %ax,d3_w
#;;   bra.w     d10                    # [78]
	jmp      d10
cec:
#;;   moveq     #2,d1                  # [79]
	movl     $2,%edx
#;;   bsr.b     d46                    # [80]
	call     d46
#;;   addq.w    #7,d2                  # [81]
	addw     $7,d2_w
#;;   move.w    d2,d3                  # [82]
	movw     d2_w,%ax
	movw     %ax,d3_w
#;;   bra.w     d10                    # [83]
	jmp      d10
cf8:
#;;   moveq     #8,d1                  # [84]
	movl     $8,%edx
#;;   bsr.b     d46                    # [85]
	call     d46
#;;   move.w    d2,d3                  # [86]
	movw     d2_w,%ax
	movw     %ax,d3_w
#;;   bra.w     d10                    # [87]
	jmp      d10
d02:
#;;   moveq     #8,d1                  # [88]
	movl     $8,%edx
#;;   lsr.l     #1,d0                  # [89 CS NE]
	shrl     $1,%ebx
#;;   bne.b     d0a                    # [90]
	jne      d0a
#;;   bsr.b     d3c                    # [91 (CS)]
	call     d3c
d0a:
#;;   bcs.b     d26                    # [92]
	jc       d26
#;;   moveq     #14,d1                 # [93]
	movl     $14,%edx
#;;   bra.b     d26                    # [94]
	jmp      d26
d10:
#;;   moveq     #16,d1                 # [95]
	movl     $16,%edx
#;;   lsr.l     #1,d0                  # [96 CC NE]
	shrl     $1,%ebx
#;;   bne.b     d18                    # [97]
	jne      d18
#;;   bsr.b     d3c                    # [98 (CC)]
	call     d3c
d18:
#;;   bcc.b     d26                    # [99]
	jnc      d26
#;;   moveq     #8,d1                  # [100]
	movl     $8,%edx
#;;   lsr.l     #1,d0                  # [101 CS NE]
	shrl     $1,%ebx
#;;   bne.b     d22                    # [102]
	jne      d22
#;;   bsr.b     d3c                    # [103 (CS)]
	call     d3c
d22:
#;;   bcs.b     d26                    # [104]
	jc       d26
#;;   moveq     #12,d1                 # [105]
	movl     $12,%edx
d26:
#;;   bsr.b     d46                    # [106]
	call     d46
#;;   subq.w    #1,d3                  # [107]
	subw     $1,d3_w
d2a:
#;;   move.b    -1(a2,d2.l),-(a2)      # [108]
	subl     $1,a2_l
	movl     d2_l,%ecx
	addl     a2_l,%ecx
	movb     -1(%ecx),%al
	movl     a2_l,%ecx
	movb     %al,(%ecx)
#;;   dbra      d3,d2a                 # [109]
	decw     d3_w
	cmpw     $-1,d3_w
	jne      d2a
#;;   bra.w     again                  # [110]
	jmp      again
exit:
#;;   rts                              # [112]
	ret      
d3c:
#;;   move.l    -(a0),d0               # [114]
	lea      -4(%esi),%esi
	movl     (%esi),%ebx
#;;   move      #$10,ccr               # [115 X]
	movb     $((0x10)>>4)&1,xflag
#;;   roxr.l    #1,d0                  # [116 CC CS]
	btw      $0,xflag
	rcrl     $1,%ebx
#;;   rts                              # [117]
	ret      
d46:
#;;   clr.l     d2                     # [119]
	movl     $0,d2_l
#;;   subq.w    #1,d1                  # [120]
	subw     $1,%dx
roxlloop:
#;;   lsr.l     #1,d0                  # [121 EQ X]
	shrl     $1,%ebx
	setcb    xflag
#;;   beq.b     zerobit                # [122]
	je       zerobit
#;;   roxl.l    #1,d2                  # [123]
	btw      $0,xflag
	rcll     $1,d2_l
#;;   dbra      d1,roxlloop            # [124]
	decw     %dx
	cmpw     $-1,%dx
	jne      roxlloop
#;;   rts                              # [125]
	ret      
zerobit:
#;;   move.l    -(a0),d0               # [127]
	lea      -4(%esi),%esi
	movl     (%esi),%ebx
#;;   move      #$10,ccr               # [128 X]
	movb     $((0x10)>>4)&1,xflag
#;;   roxr.l    #1,d0                  # [129 X]
	btw      $0,xflag
	rcrl     $1,%ebx
#;;   roxl.l    #1,d2                  # [130]
	rcll     $1,d2_l
#;;   dbra      d1,roxlloop            # [131]
	decw     %dx
	cmpw     $-1,%dx
	jne      roxlloop
#;;   rts                              # [132]
	ret      
