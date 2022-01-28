#;------------------------------------------------------------------------------
#; Crunch Master Decruncher
#;
#; IN :	D6 = Count Flag (0=Count Only)
#;
#; OUT:	D7 = Number Of Decrunched Bytes
#;
    .text
    .globl UCRMAS
    .globl _UCRMAS
UCRMAS:
_UCRMAS:
    movl 4(%esp),%eax
    movl %eax,UI_CrunchAdrTemp
    
    movl 8(%esp),%eax
    movl %eax,UI_DecrunchAdr
    
    movl 12(%esp),%eax
    movl %eax,UI_DecrunchLen
    
    movl $1,d6_w
    


#;;   move.l    UI_CrunchAdrTemp(a4),a2 # [9]
	movl     a4_l,%ecx
	movl     UI_CrunchAdrTemp,%eax
	movl     %eax,a2_l
#;;   lea       $a2(a2),a2             # [10]
	addl     $0xa2,a2_l
#;;   lea       8(a2),a0               # [11]
	movl     a2_l,%ecx
	lea      8(%ecx),%esi
#;;   move.l    a0,a5                  # [12]
	movl     %esi,a5_l
#;;   add.l     (a2),a0                # [13]
	addl     (%ecx),%esi
#;;   subq.l    #6,a0                  # [14]
	lea      -6(%esi),%esi
#;;   move.l    UI_DecrunchAdr(a4),a1  # [15]
	movl     a4_l,%ecx
	movl     UI_DecrunchAdr,%edi
#;;   add.l     UI_DecrunchLen(a4),a1  # [16]
	addl     UI_DecrunchLen,%edi
#;;   moveq     #0,d4                  # [17]
	movl     $0,d4_l
#;;   moveq     #0,d7                  # [18]
	movl     $0,d7_l
CRMAS1:
#;;   subq.l    #1,d4                  # [20]
	subl     $1,d4_l
#;;   cmp.w     #$5456,(a0)            # [21 NE]
	cmpw     $0x5456,(%esi)
#;;   bne.b     crmas3                 # [22]
	jne      CRMAS3
#;;   cmp.w     #$5456,6(a0)           # [23 NE]
	cmpw     $0x5456,6(%esi)
#;;   bne.b     crmas3                 # [24]
	jne      CRMAS3
#;;   tst.l     d4                     # [25 EQ]
	testl    $0xffffffff,d4_l
#;;   beq.b     crmas3                 # [26]
	je       CRMAS3
#;;   moveq     #3,d4                  # [27]
	movl     $3,d4_l
#;;   move.w    2(a0),d0               # [28]
	movw     2(%esi),%bx
#;;   move.w    4(a0),d1               # [29]
	movw     4(%esi),%dx
#;;   addq.l    #6,a1                  # [30]
	lea      6(%edi),%edi
#;;   subq.l    #6,d7                  # [31]
	subl     $6,d7_l
CRMAS2:
#;;   tst.w     d6                     # [32 EQ]
	testw    $0xffff,d6_w
#;;   beq.b     crmas2a                # [33]
	je       CRMAS2a
#;;   move.w    d1,-(a1)               # [34]
	lea      -2(%edi),%edi
	movw     %dx,(%edi)
CRMAS2a:
#;;   addq.l    #2,d7                  # [35]
	addl     $2,d7_l
#;;   dbra      d0,crmas2              # [36]
	decw     %bx
	cmpw     $-1,%bx
	jne      CRMAS2
#;;   bra.b     crmas4                 # [37]
	jmp      CRMAS4
CRMAS3:
#;;   tst.w     d6                     # [39 EQ]
	testw    $0xffff,d6_w
#;;   beq.b     crmas3a                # [40]
	je       CRMAS3a
#;;   move.w    (a0),-(a1)             # [41]
	lea      -2(%edi),%edi
	movw     (%esi),%ax
	movw     %ax,(%edi)
CRMAS3a:
#;;   addq.l    #2,d7                  # [42]
	addl     $2,d7_l
CRMAS4:
#;;   subq.l    #2,a0                  # [43]
	lea      -2(%esi),%esi
#;;   cmp.l     a5,a0                  # [44 CC]
	cmpl     a5_l,%esi
#;;   bcc.b     crmas1                 # [45]
	jnc      CRMAS1
#;;   rts                              # [46]
    movl d7_l,%eax
	ret      
