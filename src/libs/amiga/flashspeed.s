#;------------------------------------------------------------------------------
#; FlashSpeed 1.0 Unpacker Routine
#;
#; IN :	A1 = Pointer To Decrunched Data
#;	A2 = Pointer To Crunched Data
#;	D7 = Count Or Depack (0=Depack)
#;
#; OUT:	D0 = Length Of Decrunched Data
#;
    .text
    .globl  UFLSP
    .globl  _UFLSP
UFLSP:
_UFLSP:
    movl 4(%esp), %edi
    
    movl 8(%esp), %eax
    movl %eax, a2_l

    movl 12(%esp), %eax
    movl %eax, d7_w

UFLSP0:
#;;   lea       $70(a2),a0             # [15]
	movl     a2_l,%ecx
	lea      0x70(%ecx),%esi
#;;   moveq     #0,d0                  # [16]
	movl     $0,%ebx
#;;   moveq     #0,d1                  # [17]
	movl     $0,%edx
#;;   moveq     #0,d2                  # [18]
	movl     $0,d2_l
#;;   moveq     #1,d3                  # [19]
	movl     $1,d3_l
UFLSP1:
#;;   addq.l    #1,d2                  # [20]
	addl     $1,d2_l
#;;   tst.w     d7                     # [21 NE]
	testw    $0xffff,d7_w
#;;   bne.b     uflsp2                 # [22]
	jne      UFLSP2
#;;   move.b    (a0),(a1)+             # [23]
	movb     (%esi),%al
	stosb    
UFLSP2:
#;;   cmp.b     (a0)+,d3               # [24 NE]
	lodsb    
	cmpb     %al,d3_b
#;;   bne.b     uflsp1                 # [25]
	jne      UFLSP1
#;;   cmp.b     (a0),d3                # [26 NE]
	movb     (%esi),%al
	cmpb     %al,d3_b
#;;   bne.b     uflsp1                 # [27]
	jne      UFLSP1
#;;   cmp.b     1(a0),d3               # [28 EQ]
	movb     1(%esi),%al
	cmpb     %al,d3_b
#;;   beq.b     uflsp3                 # [29]
	je       UFLSP3
#;;   subq.l    #1,d2                  # [30]
	subl     $1,d2_l
#;;   subq.l    #1,a1                  # [31]
	lea      -1(%edi),%edi
#;;   addq.l    #1,a0                  # [32]
	lea      1(%esi),%esi
#;;   bra.b     uflsp4                 # [33]
	jmp      UFLSP4
UFLSP3:
#;;   addq.l    #2,a0                  # [35]
	lea      2(%esi),%esi
UFLSP4:
#;;   move.b    (a0)+,d1               # [36 EQ]
	movb     (%esi),%dl
	testb    %dl,%dl
	lea      1(%esi),%esi
#;;   beq.b     uflsp7                 # [37]
	je       UFLSP7
#;;   move.b    (a0)+,d0               # [38]
	movb     (%esi),%bl
	lea      1(%esi),%esi
#;;   subq.w    #1,d1                  # [39]
	subw     $1,%dx
UFLSP5:
#;;   addq.l    #1,d2                  # [40]
	addl     $1,d2_l
#;;   tst.w     d7                     # [41 NE]
	testw    $0xffff,d7_w
#;;   bne.b     uflsp6                 # [42]
	jne      UFLSP6
#;;   move.b    d0,(a1)+               # [43]
	movb     %bl,(%edi)
	lea      1(%edi),%edi
UFLSP6:
#;;   dbra      d1,uflsp5              # [44]
	decw     %dx
	cmpw     $-1,%dx
	jne      UFLSP5
#;;   moveq     #0,d1                  # [45]
	movl     $0,%edx
#;;   bra.b     uflsp1                 # [46]
	jmp      UFLSP1
UFLSP7:
#;;   move.l    d2,d0                  # [48]
	movl     d2_l,%ebx
#;;   rts                              # [49]
    movl %ebx,%eax
	ret      
