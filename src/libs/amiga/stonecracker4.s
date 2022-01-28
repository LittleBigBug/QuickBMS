    .text
    .globl stonecracker403
    .globl _stonecracker403
stonecracker403:
_stonecracker403:
    movl 4(%esp),%esi
    movl 8(%esp),%edi


#;;   movem.l   d3-d7/a2-a3,-(sp)      # [35]
	pushl    a3_l
	pushl    a2_l
	pushl    d7_l
	pushl    d6_l
	pushl    d5_l
	pushl    d4_l
	pushl    d3_l
#;;   beq.s     l0                     # [37]
	je       l0
#;;   movem.l   (sp)+,d3-d7/a2-a3      # [38]
	popl     d3_l
	popl     d4_l
	popl     d5_l
	popl     d6_l
	popl     d7_l
	popl     a2_l
	popl     a3_l
#;;   moveq     #0,d0                  # [39]
	movl     $0,%ebx
#;;   rts                              # [40]
    movl %ebx,%eax
	ret      
l0:
#;;   addq      #4,a1                  # [42]
	lea      4(%edi),%edi
#;;   move.l    a0,d4                  # [43]
	movl     %esi,d4_l
#;;   add.l     (a1)+,a0               # [44]
	addl     (%edi),%esi
	lea      4(%edi),%edi
#;;   add.l     (a1)+,a1               # [45]
	addl     (%edi),%edi
	lea      4(%edi),%edi
#;;   move      (a1),d6                # [46]
	movw     (%edi),%ax
	movw     %ax,d6_w
#;;   move      -(a1),d7               # [47]
	lea      -2(%edi),%edi
	movw     (%edi),%ax
	movw     %ax,d7_w
#;;   moveq     #16,d5                 # [48]
	movl     $16,d5_l
#;;   lea       l11(pc),a3             # [49]
	movl     $l11,a3_l
#;;   l1:             lflag            # [51 (Macro)]
l1:
#;;   subq      #1,d6                  # [51 PL]
	subw     $1,d6_w
#;;   bpl.s     __000l7                # [51]
	jns      __000l7
#;;   move      -(a1),d7               # [51]
	lea      -2(%edi),%edi
	movw     (%edi),%ax
	movw     %ax,d7_w
#;;   moveq     #15,d6                 # [51]
	movl     $15,d6_l
__000l7:
#;;   lsr       #1,d7                  # [51 CS]
	shrw     $1,d7_w
#;;   bcs.s     l3                     # [52]
	jc       l3
#;;   moveq     #8,d1                  # [54]
	movl     $8,%edx
#;;                   lbits            # [55 (Macro)]
#;;   move      d6,d0                  # [55]
	movw     d6_w,%bx
#;;   sub       d1,d6                  # [55 PL]
	subw     %dx,d6_w
#;;   bpl.s     __001l9                # [55]
	jns      __001l9
#;;   ror.l     d0,d7                  # [55]
	movl     %ebx,%ecx
	rorl     %cl,d7_l
#;;   move      -(a1),d7               # [55]
	lea      -2(%edi),%edi
	movw     (%edi),%ax
	movw     %ax,d7_w
#;;   rol.l     d0,d7                  # [55]
	movl     %ebx,%ecx
	roll     %cl,d7_l
#;;   add       d5,d6                  # [55]
	movw     d5_w,%ax
	addw     %ax,d6_w
__001l9:
#;;   move      d7,d0                  # [55]
	movw     d7_w,%bx
#;;   lsr.l     d1,d7                  # [55]
	movl     %edx,%ecx
	shrl     %cl,d7_l
#;;   add       d1,d1                  # [55]
	addw     %dx,%dx
#;;   and       -4(a3,d1.w),d0         # [55]
	movswl   %dx,%ecx
	addl     a3_l,%ecx
	andw     -4(%ecx),%bx
#;;   move.b    d0,-(a0)               # [56]
	lea      -1(%esi),%esi
	movb     %bl,(%esi)
l2:
#;;   cmp.l     a0,d4                  # [57 CS]
	cmpl     %esi,d4_l
#;;   blo.s     l1                     # [58]
	jc       l1
lexit:
#;;   movem.l   (sp)+,d3-d7/a2-a3      # [59]
	popl     d3_l
	popl     d4_l
	popl     d5_l
	popl     d6_l
	popl     d7_l
	popl     a2_l
	popl     a3_l
#;;   moveq     #1,d0                  # [60]
	movl     $1,%ebx
#;;   rts                              # [61]
	movl %ebx,%eax
	ret      
	.data
	.p2align	2
#;;   l10:            dc.w    5,0,8,32,10,288,12,1312 # [63]
l10:
	.short	5,0,8,32,10,288,12,1312
	.text
	.p2align	2
l3:
#;;   moveq     #2,d1                  # [65]
	movl     $2,%edx
#;;                   lbits            # [66 (Macro)]
#;;   move      d6,d0                  # [66]
	movw     d6_w,%bx
#;;   sub       d1,d6                  # [66 PL]
	subw     %dx,d6_w
#;;   bpl.s     __002l9                # [66]
	jns      __002l9
#;;   ror.l     d0,d7                  # [66]
	movl     %ebx,%ecx
	rorl     %cl,d7_l
#;;   move      -(a1),d7               # [66]
	lea      -2(%edi),%edi
	movw     (%edi),%ax
	movw     %ax,d7_w
#;;   rol.l     d0,d7                  # [66]
	movl     %ebx,%ecx
	roll     %cl,d7_l
#;;   add       d5,d6                  # [66]
	movw     d5_w,%ax
	addw     %ax,d6_w
__002l9:
#;;   move      d7,d0                  # [66]
	movw     d7_w,%bx
#;;   lsr.l     d1,d7                  # [66]
	movl     %edx,%ecx
	shrl     %cl,d7_l
#;;   add       d1,d1                  # [66]
	addw     %dx,%dx
#;;   and       -4(a3,d1.w),d0         # [66]
	movswl   %dx,%ecx
	addl     a3_l,%ecx
	andw     -4(%ecx),%bx
#;;   add       d0,d0                  # [67]
	addw     %bx,%bx
#;;   add       d0,d0                  # [68]
	addw     %bx,%bx
#;;   movem     l10(pc,d0.w),d1/d3     # [69]
	movswl   %bx,%ecx
	movswl   l10+0(%ecx),%edx
	movswl   l10+2(%ecx),%eax
	movl     %eax,d3_l
#;;                   lbits            # [70 (Macro)]
#;;   move      d6,d0                  # [70]
	movw     d6_w,%bx
#;;   sub       d1,d6                  # [70 PL]
	subw     %dx,d6_w
#;;   bpl.s     __003l9                # [70]
	jns      __003l9
#;;   ror.l     d0,d7                  # [70]
	movl     %ebx,%ecx
	rorl     %cl,d7_l
#;;   move      -(a1),d7               # [70]
	lea      -2(%edi),%edi
	movw     (%edi),%ax
	movw     %ax,d7_w
#;;   rol.l     d0,d7                  # [70]
	movl     %ebx,%ecx
	roll     %cl,d7_l
#;;   add       d5,d6                  # [70]
	movw     d5_w,%ax
	addw     %ax,d6_w
__003l9:
#;;   move      d7,d0                  # [70]
	movw     d7_w,%bx
#;;   lsr.l     d1,d7                  # [70]
	movl     %edx,%ecx
	shrl     %cl,d7_l
#;;   add       d1,d1                  # [70]
	addw     %dx,%dx
#;;   and       -4(a3,d1.w),d0         # [70]
	movswl   %dx,%ecx
	addl     a3_l,%ecx
	andw     -4(%ecx),%bx
#;;   add       d3,d0                  # [71]
	addw     d3_w,%bx
#;;   lea       1(a0,d0.w),a2          # [72]
	movswl   %bx,%ecx
	lea      1(%esi,%ecx),%eax
#;;                   lflag            # [74 (Macro)]
#;;   subq      #1,d6                  # [74 PL]
	subw     $1,d6_w
	movl     %eax,a2_l
#;;   bpl.s     __004l7                # [74]
	jns      __004l7
#;;   move      -(a1),d7               # [74]
	lea      -2(%edi),%edi
	movw     (%edi),%ax
	movw     %ax,d7_w
#;;   moveq     #15,d6                 # [74]
	movl     $15,d6_l
__004l7:
#;;   lsr       #1,d7                  # [74 CS]
	shrw     $1,d7_w
#;;   bcs.s     l5_2                   # [75]
	jc       l5_2
#;;                   lflag            # [76 (Macro)]
#;;   subq      #1,d6                  # [76 PL]
	subw     $1,d6_w
#;;   bpl.s     __005l7                # [76]
	jns      __005l7
#;;   move      -(a1),d7               # [76]
	lea      -2(%edi),%edi
	movw     (%edi),%ax
	movw     %ax,d7_w
#;;   moveq     #15,d6                 # [76]
	movl     $15,d6_l
__005l7:
#;;   lsr       #1,d7                  # [76 CS]
	shrw     $1,d7_w
#;;   bcs.s     l5_3                   # [77]
	jc       l5_3
#;;                   lflag            # [78 (Macro)]
#;;   subq      #1,d6                  # [78 PL]
	subw     $1,d6_w
#;;   bpl.s     __006l7                # [78]
	jns      __006l7
#;;   move      -(a1),d7               # [78]
	lea      -2(%edi),%edi
	movw     (%edi),%ax
	movw     %ax,d7_w
#;;   moveq     #15,d6                 # [78]
	movl     $15,d6_l
__006l7:
#;;   lsr       #1,d7                  # [78 CS]
	shrw     $1,d7_w
#;;   bcs.s     l5_4                   # [79]
	jc       l5_4
#;;   moveq     #0,d3                  # [81]
	movl     $0,d3_l
l4:
#;;   moveq     #3,d1                  # [82]
	movl     $3,%edx
#;;                   lbits            # [83 (Macro)]
#;;   move      d6,d0                  # [83]
	movw     d6_w,%bx
#;;   sub       d1,d6                  # [83 PL]
	subw     %dx,d6_w
#;;   bpl.s     __007l9                # [83]
	jns      __007l9
#;;   ror.l     d0,d7                  # [83]
	movl     %ebx,%ecx
	rorl     %cl,d7_l
#;;   move      -(a1),d7               # [83]
	lea      -2(%edi),%edi
	movw     (%edi),%ax
	movw     %ax,d7_w
#;;   rol.l     d0,d7                  # [83]
	movl     %ebx,%ecx
	roll     %cl,d7_l
#;;   add       d5,d6                  # [83]
	movw     d5_w,%ax
	addw     %ax,d6_w
__007l9:
#;;   move      d7,d0                  # [83]
	movw     d7_w,%bx
#;;   lsr.l     d1,d7                  # [83]
	movl     %edx,%ecx
	shrl     %cl,d7_l
#;;   add       d1,d1                  # [83]
	addw     %dx,%dx
#;;   and       -4(a3,d1.w),d0         # [83]
	movswl   %dx,%ecx
	addl     a3_l,%ecx
	andw     -4(%ecx),%bx
#;;   add       d0,d3                  # [84]
	addw     %bx,d3_w
#;;   subq      #7,d0                  # [85 EQ]
	subw     $7,%bx
#;;   beq.s     l4                     # [86]
	je       l4
l5:
#;;   move.b    -(a2),-(a0)            # [87]
	subl     $1,a2_l
	lea      -1(%esi),%esi
	movl     a2_l,%ecx
	movb     (%ecx),%al
	movb     %al,(%esi)
#;;   dbf       d3,l5                  # [88]
	decw     d3_w
	cmpw     $-1,d3_w
	jne      l5
l5_4:
#;;   move.b    -(a2),-(a0)            # [89]
	subl     $1,a2_l
	lea      -1(%esi),%esi
	movl     a2_l,%ecx
	movb     (%ecx),%al
	movb     %al,(%esi)
l5_3:
#;;   move.b    -(a2),-(a0)            # [90]
	subl     $1,a2_l
	lea      -1(%esi),%esi
	movl     a2_l,%ecx
	movb     (%ecx),%al
	movb     %al,(%esi)
l5_2:
#;;   move.b    -(a2),-(a0)            # [91]
	subl     $2,a2_l
	lea      -2(%esi),%esi
	movl     a2_l,%ecx
	movb     1(%ecx),%al
	movb     %al,1(%esi)
#;;   move.b    -(a2),-(a0)            # [92]
	movb     0(%ecx),%al
	movb     %al,0(%esi)
#;;   cmp.l     a0,d4                  # [93 CS]
	cmpl     %esi,d4_l
#;;   blo       l1                     # [94]
	jc       l1
#;;   movem.l   (sp)+,d3-d7/a2-a3      # [95]
	popl     d3_l
	popl     d4_l
	popl     d5_l
	popl     d6_l
	popl     d7_l
	popl     a2_l
	popl     a3_l
#;;   moveq     #1,d0                  # [96]
	movl     $1,%ebx
#;;   rts                              # [97]
	movl %ebx,%eax
	ret      
	.data
	.p2align	2
#;;   l11:            dc.w    $0003,$0007,$000f,$001f # [99]
l11:
	.short	0x3,0x7,0xf,0x1f
#;;                   dc.w    $003f,$007f,$00ff,$01ff # [100]
	.short	0x3f,0x7f,0xff,0x1ff
#;;                   dc.w    $03ff,$07ff,$0fff # [101]
	.short	0x3ff,0x7ff,0xfff
