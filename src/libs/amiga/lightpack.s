    .text
    .globl LIGHT15
    .globl _LIGHT15
LIGHT15:
_LIGHT15:
    movl 4(%esp), %esi  # a0
    movl 8(%esp), %edi  # a1
    movl 12(%esp), %eax
    movl %eax,d2_l

#;;   move.b    #$97,d3                # [2]
	movb     $0x97,d3_b
LIGHT1:
#;;   cmp.b     (a0),d3                # [3 EQ]
	movb     (%esi),%al
	cmpb     %al,d3_b
#;;   beq.b     light2                 # [4]
	je       LIGHT2
#;;   move.b    (a0)+,(a1)+            # [5]
	movsb    
#;;   subq.l    #1,d2                  # [6 NE]
	subl     $1,d2_l
#;;   bne.b     light1                 # [7]
	jne      LIGHT1
#;;   bra.b     light4                 # [8]
	jmp      LIGHT4
LIGHT2:
#;;   move.b    1(a0),d0               # [10]
	movb     1(%esi),%bl
#;;   lsl.w     #8,d0                  # [11]
	shlw     $8,%bx
#;;   move.b    2(a0),d0               # [12]
	movb     2(%esi),%bl
#;;   move.b    3(a0),d1               # [13]
	movb     3(%esi),%dl
#;;   subq.w    #1,d0                  # [14]
	subw     $1,%bx
LIGHT3:
#;;   move.b    d1,(a1)+               # [15]
	movb     %dl,(%edi)
	lea      1(%edi),%edi
#;;   subq.l    #1,d2                  # [16 EQ]
	subl     $1,d2_l
#;;   beq.b     light4                 # [17]
	je       LIGHT4
#;;   dbra      d0,light3              # [18]
	decw     %bx
	cmpw     $-1,%bx
	jne      LIGHT3
#;;   addq.l    #4,a0                  # [19]
	lea      4(%esi),%esi
#;;   bra.b     light1                 # [20]
	jmp      LIGHT1
LIGHT4:
#;;   moveq     #-1,d0                 # [22]
	movl     $-1,%ebx
LIGHTO:
#;;   rts                              # [23]
	movl %ebx,%eax
	ret      
