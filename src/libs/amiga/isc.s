#;------------------------------------------------------------------------------
#; ISC Pass 1 Decruncher
#;
#; IN :	A1 = Pointer To Crunched Data
#;	A2 = Pointer To Destination
#;	D5 = Count Mode (0=On)
#;	D7 = If Count Mode: 0, Else Anything
#;
#; OUT:	D7 = Length Of Decrunched Data
#;
    .text
    .globl ISC1P
    .globl _ISC1P
ISC1P:
_ISC1P:
    movl 4(%esp),%edi

    movl 8(%esp), %eax
    movl %eax,a2_l
    
    movl 12(%esp),%eax
    movl %eax,d5_w
    
    movl 16(%esp),%eax
    movl %eax,d7_l

#;;   moveq     #0,d0                  # [16]
	movl     $0,%ebx
#;;   moveq     #0,d1                  # [17]
	movl     $0,%edx
#;;   move.b    (a1)+,d0               # [18 NE]
	movb     (%edi),%bl
	testb    %bl,%bl
	lea      1(%edi),%edi
#;;   bne.b     isc1p1                 # [19]
	jne      ISC1P1
#;;   rts                              # [20]
    movl d7_l,%eax
	ret      
ISC1P1:
#;;   move.b    d0,d1                  # [22]
	movb     %bl,%dl
#;;   and.b     #$1f,d0                # [23]
	andb     $0x1f,%bl
#;;   and.b     #$e0,d1                # [24]
	andb     $0xe0,%dl
#;;   lsr.b     #4,d1                  # [25]
	shrb     $4,%dl
#;;   lea       isc1p2(pc),a3          # [26]
	movl     $ISC1P2,a3_l
#;;   add.l     d1,a3                  # [27]
	addl     %edx,a3_l
#;;   jsr       (a3)                   # [28]
	call     *a3_l
#;;   bra.b     isc1p                  # [29]
	jmp      ISC1P
ISC1P2:
#;;   bra.b     isc1p6                 # [31]
	jmp      ISC1P6
#;;   bra.b     isc1p14                # [32]
	jmp      ISC1P14
#;;   bra.b     isc1p10                # [33]
	jmp      ISC1P10
#;;   bra.b     isc1p11                # [34]
	jmp      ISC1P11
#;;   bra.b     isc1p4                 # [35]
	jmp      ISC1P4
#;;   bra.b     isc1p3                 # [36]
	jmp      ISC1P3
#;;   bra.b     isc1p15                # [37]
	jmp      ISC1P15
#;;   clr.b     d1                     # [38]
	xorb     %dl,%dl
#;;   bra.b     isc1p5                 # [39]
	jmp      ISC1P5
ISC1P3:
#;;   lsl.w     #8,d0                  # [41]
	shlw     $8,%bx
#;;   move.b    (a1)+,d0               # [42]
	movb     (%edi),%bl
	lea      1(%edi),%edi
ISC1P4:
#;;   move.b    (a1)+,d1               # [43]
	movb     (%edi),%dl
	lea      1(%edi),%edi
ISC1P5:
#;;   addq.l    #1,d7                  # [44]
	addl     $1,d7_l
#;;   tst.w     d5                     # [45 EQ]
	testw    $0xffff,d5_w
#;;   beq.b     isc1p5a                # [46]
	je       ISC1P5a
#;;   move.b    d1,(a2)+               # [47]
	movl     a2_l,%ecx
	addl     $1,a2_l
	movb     %dl,(%ecx)
ISC1P5a:
#;;   dbra      d0,isc1p5              # [48]
	decw     %bx
	cmpw     $-1,%bx
	jne      ISC1P5
#;;   rts                              # [49]
    movl d7_l,%eax
	ret      
ISC1P6:
#;;   move.b    (a1)+,d1               # [51]
	movb     (%edi),%dl
	lea      1(%edi),%edi
ISC1P7:
#;;   moveq     #0,d6                  # [52]
	movl     $0,d6_l
ISC1P8:
#;;   addq.l    #1,d7                  # [53]
	addl     $1,d7_l
#;;   tst.w     d5                     # [54 EQ]
	testw    $0xffff,d5_w
#;;   beq.b     isc1p8a                # [55]
	je       ISC1P8a
#;;   move.b    (a1,d6.w),(a2)+        # [56]
	movswl   d6_w,%ecx
	movb     0(%edi,%ecx),%al
	movl     a2_l,%ecx
	addl     $1,a2_l
	movb     %al,(%ecx)
ISC1P8a:
#;;   addq.b    #1,d6                  # [57]
	addb     $1,d6_b
#;;   cmp.b     d6,d0                  # [58 NE]
	cmpb     d6_b,%bl
#;;   bne.b     isc1p8                 # [59]
	jne      ISC1P8
#;;   dbra      d1,isc1p7              # [60]
	decw     %dx
	cmpw     $-1,%dx
	jne      ISC1P7
ISC1P9:
#;;   move.b    (a1)+,d0               # [61]
	movb     (%edi),%bl
	lea      1(%edi),%edi
#;;   subq.b    #1,d6                  # [62 NE]
	subb     $1,d6_b
#;;   bne.b     isc1p9                 # [63]
	jne      ISC1P9
#;;   rts                              # [64]
    movl d7_l,%eax
	ret      
ISC1P10:
#;;   move.b    #1,d2                  # [66]
	movb     $1,d2_b
#;;   bra.b     isc1p12                # [67]
	jmp      ISC1P12
ISC1P11:
#;;   move.b    (a1)+,d2               # [69]
	movb     (%edi),%al
	movb     %al,d2_b
	lea      1(%edi),%edi
ISC1P12:
#;;   move.b    (a1)+,d3               # [70]
	movb     (%edi),%al
	movb     %al,d3_b
	lea      1(%edi),%edi
ISC1P13:
#;;   addq.l    #1,d7                  # [71]
	addl     $1,d7_l
#;;   tst.w     d5                     # [72 EQ]
	testw    $0xffff,d5_w
#;;   beq.b     isc113a                # [73]
	je       ISC113a
#;;   move.b    d3,(a2)+               # [74]
	movb     d3_b,%al
	movl     a2_l,%ecx
	addl     $1,a2_l
	movb     %al,(%ecx)
ISC113a:
#;;   add.b     d2,d3                  # [75]
	movb     d2_b,%al
	addb     %al,d3_b
#;;   dbra      d0,isc1p13             # [76]
	decw     %bx
	cmpw     $-1,%bx
	jne      ISC1P13
#;;   rts                              # [77]
    movl d7_l,%eax
	ret      
ISC1P14:
#;;   lsl.w     #8,d0                  # [79]
	shlw     $8,%bx
#;;   move.b    (a1)+,d0               # [80]
	movb     (%edi),%bl
	lea      1(%edi),%edi
ISC1P15:
#;;   move.b    (a1)+,d1               # [81]
	movb     (%edi),%dl
	lea      1(%edi),%edi
#;;   addq.l    #1,d7                  # [82]
	addl     $1,d7_l
#;;   tst.w     d5                     # [83 EQ]
	testw    $0xffff,d5_w
#;;   beq.b     isc115a                # [84]
	je       ISC115a
#;;   move.b    d1,(a2)+               # [85]
	movl     a2_l,%ecx
	addl     $1,a2_l
	movb     %dl,(%ecx)
ISC115a:
#;;   dbra      d0,isc1p15             # [86]
	decw     %bx
	cmpw     $-1,%bx
	jne      ISC1P15
#;;   rts                              # [87]
    movl d7_l,%eax
	ret      
    
    
    
    
#;------------------------------------------------------------------------------
#; ISC Pass 2 Decruncher
#;
#; IN :	A1 = Pointer To Crunched Data
#;	A2 = Pointer To Destination
#;	D4 = Count Mode (0=On)
#;	D3 = If Count Mode: 0, Else Anything
#;
#; OUT:	D3 = Length Of Decrunched Data
#;
    .globl ISC2P
    .globl _ISC2P
ISC2P:
_ISC2P:
    movl 4(%esp),%edi

    movl 8(%esp), %eax
    movl %eax,a2_l
    
    movl 12(%esp),%eax
    movl %eax,d4_w
    
    movl 16(%esp),%eax
    movl %eax,d3_l

    
#;;   lea       isc2p7(pc),a3          # [99]
	movl     $ISC2P7,a3_l
#;;   moveq     #0,d6                  # [100]
	movl     $0,d6_l
#;;   moveq     #0,d7                  # [101]
	movl     $0,d7_l
ISC2P1:
#;;   moveq     #0,d0                  # [102]
	movl     $0,%ebx
#;;   moveq     #0,d1                  # [103]
	movl     $0,%edx
#;;   bsr.w     isc2p26                # [104]
	call     ISC2P26
#;;   bclr      #3,d0                  # [105 NE]
	btrl     $(3)&31,%ebx
#;;   bne.b     isc2p6                 # [106]
	jc       ISC2P6
#;;   bclr      #2,d0                  # [107 EQ]
	btrl     $(2)&31,%ebx
#;;   beq.b     isc2p17                # [108]
	jnc      ISC2P17
ISC2P2:
#;;   move.w    d0,d1                  # [109]
	movw     %bx,%dx
ISC2P3:
#;;   bsr.w     isc2p24                # [110]
	call     ISC2P24
#;;   addq.l    #1,d3                  # [111]
	addl     $1,d3_l
#;;   tst.w     d4                     # [112 EQ]
	testw    $0xffff,d4_w
#;;   beq.b     isc2p3a                # [113]
	je       ISC2P3a
#;;   move.b    d0,(a2)+               # [114]
	movl     a2_l,%ecx
	addl     $1,a2_l
	movb     %bl,(%ecx)
ISC2P3a:
#;;   dbra      d1,isc2p3              # [115]
	decw     %dx
	cmpw     $-1,%dx
	jne      ISC2P3
#;;   bra.b     isc2p1                 # [116]
	jmp      ISC2P1
ISC2P4:
#;;   lsl.w     #8,d0                  # [118]
	shlw     $8,%bx
#;;   bsr.b     isc2p24                # [119]
	call     ISC2P24
ISC2P5:
#;;   addq.w    #4,d0                  # [120]
	addw     $4,%bx
#;;   bra.b     isc2p2                 # [121]
	jmp      ISC2P2
ISC2P6:
#;;   move.b    d0,d1                  # [123]
	movb     %bl,%dl
#;;   bsr.w     isc2p26                # [124]
	call     ISC2P26
#;;   lsl.b     #1,d1                  # [125]
	shlb     $1,%dl
#;;   jmp       (a3,d1.w)              # [126]
	movswl   %dx,%ecx
	addl     a3_l,%ecx
	lea      0(%ecx),%eax
	jmp      *%eax
ISC2P7:
#;;   bra.b     isc2p5                 # [128]
	jmp      ISC2P5
#;;   bra.b     isc2p4                 # [129]
	jmp      ISC2P4
#;;   bra.b     isc2p23                # [130]
	jmp      ISC2P23
#;;   bra.b     isc2p10                # [131]
	jmp      ISC2P10
#;;   bra.b     isc2p20                # [132]
	jmp      ISC2P20
#;;   bra.b     isc2p21                # [133]
	jmp      ISC2P21
#;;   bra.b     isc2p22                # [134]
	jmp      ISC2P22
#;;   bclr      #3,d0                  # [136 EQ]
	btrl     $(3)&31,%ebx
#;;   beq.b     isc2p8                 # [137]
	jnc      ISC2P8
#;;   moveq     #1,d1                  # [138]
	movl     $1,%edx
#;;   bra.b     isc2p9                 # [139]
	jmp      ISC2P9
ISC2P8:
#;;   moveq     #2,d1                  # [141]
	movl     $2,%edx
ISC2P9:
#;;   bsr.b     isc2p16                # [142]
	call     ISC2P16
#;;   bra.b     isc2p19                # [143]
	jmp      ISC2P19
ISC2P10:
#;;   bclr      #3,d0                  # [145 EQ]
	btrl     $(3)&31,%ebx
#;;   beq.b     isc2p13                # [146]
	jnc      ISC2P13
#;;   bclr      #2,d0                  # [147 EQ]
	btrl     $(2)&31,%ebx
#;;   beq.b     isc2p11                # [148]
	jnc      ISC2P11
#;;   moveq     #3,d1                  # [149]
	movl     $3,%edx
#;;   bra.b     isc2p12                # [150]
	jmp      ISC2P12
ISC2P11:
#;;   moveq     #2,d1                  # [152]
	movl     $2,%edx
ISC2P12:
#;;   bsr.b     isc2p14                # [153]
	call     ISC2P14
#;;   bra.b     isc2p19                # [154]
	jmp      ISC2P19
ISC2P13:
#;;   bclr      #2,d0                  # [156 NE]
	btrl     $(2)&31,%ebx
#;;   bne.b     isc2p18                # [157]
	jc       ISC2P18
#;;   bsr.b     isc2p16                # [158]
	call     ISC2P16
#;;   bra.b     isc2p29                # [159]
	jmp      ISC2P29
ISC2P14:
#;;   lsl.w     #8,d0                  # [161]
	shlw     $8,%bx
ISC2P15:
#;;   bsr.b     isc2p24                # [162]
	call     ISC2P24
ISC2P16:
#;;   lsl.l     #8,d0                  # [163]
	shll     $8,%ebx
#;;   bra.b     isc2p24                # [164]
	jmp      ISC2P24
ISC2P17:
#;;   bsr.b     isc2p16                # [166]
	call     ISC2P16
#;;   moveq     #0,d1                  # [167]
	movl     $0,%edx
#;;   tst.w     d0                     # [168 NE]
	testw    %bx,%bx
#;;   bne.b     isc2p33                # [169]
	jne      ISC2P33
#;;   rts                              # [170]
    movl d3_l,%eax
	ret      
ISC2P18:
#;;   bsr.b     isc2p16                # [172]
	call     ISC2P16
#;;   add.w     #$800,d0               # [173]
	addw     $0x800,%bx
#;;   moveq     #1,d1                  # [174]
	movl     $1,%edx
ISC2P19:
#;;   bra.b     isc2p33                # [175]
	jmp      ISC2P33
ISC2P20:
#;;   move.b    d0,d1                  # [177]
	movb     %bl,%dl
#;;   bsr.b     isc2p15                # [178]
	call     ISC2P15
#;;   bra.b     isc2p32                # [179]
	jmp      ISC2P32
ISC2P21:
#;;   bsr.b     isc2p16                # [181]
	call     ISC2P16
#;;   bra.b     isc2p30                # [182]
	jmp      ISC2P30
ISC2P22:
#;;   move.b    d0,d1                  # [184]
	movb     %bl,%dl
#;;   bsr.b     isc2p24                # [185]
	call     ISC2P24
#;;   bra.b     isc2p32                # [186]
	jmp      ISC2P32
ISC2P23:
#;;   bra.b     isc2p28                # [188]
	jmp      ISC2P28
ISC2P24:
#;;   tst.b     d7                     # [190 NE]
	testb    $0xff,d7_b
#;;   bne.b     isc2p25                # [191]
	jne      ISC2P25
#;;   move.b    (a1)+,d0               # [192]
	movb     (%edi),%bl
	lea      1(%edi),%edi
#;;   rts                              # [193]
    movl d3_l,%eax
	ret      
ISC2P25:
#;;   move.b    d6,d0                  # [195]
	movb     d6_b,%bl
#;;   lsl.b     #4,d0                  # [196]
	shlb     $4,%bl
#;;   move.b    (a1)+,d6               # [197]
	movb     (%edi),%al
	movb     %al,d6_b
	lea      1(%edi),%edi
#;;   move.b    d6,d5                  # [198]
	movb     d6_b,%al
	movb     %al,d5_b
#;;   lsr.b     #4,d5                  # [199]
	shrb     $4,d5_b
#;;   or.b      d5,d0                  # [200]
	orb      d5_b,%bl
#;;   and.b     #15,d6                 # [201]
	andb     $15,d6_b
#;;   rts                              # [202]
    movl d3_l,%eax
	ret      
ISC2P26:
#;;   tst.b     d7                     # [204 NE]
	testb    $0xff,d7_b
#;;   bne.b     isc2p27                # [205]
	jne      ISC2P27
#;;   moveq     #1,d7                  # [206]
	movl     $1,d7_l
#;;   move.b    (a1)+,d6               # [207]
	movb     (%edi),%al
	movb     %al,d6_b
	lea      1(%edi),%edi
#;;   move.b    d6,d0                  # [208]
	movb     d6_b,%bl
#;;   and.b     #15,d6                 # [209]
	andb     $15,d6_b
#;;   lsr.b     #4,d0                  # [210]
	shrb     $4,%bl
#;;   rts                              # [211]
    movl d3_l,%eax
	ret      
ISC2P27:
#;;   moveq     #0,d7                  # [213]
	movl     $0,d7_l
#;;   move.b    d6,d0                  # [214]
	movb     d6_b,%bl
#;;   rts                              # [215]
    movl d3_l,%eax
	ret      
ISC2P28:
#;;   bsr.b     isc2p14                # [217]
	call     ISC2P14
#;;   bclr      #$13,d0                # [218 EQ]
	btrl     $(0x13)&31,%ebx
#;;   beq.b     isc2p30                # [219]
	jnc      ISC2P30
ISC2P29:
#;;   move.l    d0,d2                  # [220]
	movl     %ebx,d2_l
#;;   bsr.b     isc2p15                # [221]
	call     ISC2P15
#;;   bra.b     isc2p31                # [222]
	jmp      ISC2P31
ISC2P30:
#;;   move.l    d0,d2                  # [224]
	movl     %ebx,d2_l
#;;   moveq     #0,d0                  # [225]
	movl     $0,%ebx
#;;   bsr.b     isc2p24                # [226]
	call     ISC2P24
ISC2P31:
#;;   move.w    d0,d1                  # [227]
	movw     %bx,%dx
#;;   move.l    d2,d0                  # [228]
	movl     d2_l,%ebx
ISC2P32:
#;;   addq.l    #1,d1                  # [229]
	addl     $1,%edx
ISC2P33:
#;;   move.l    a2,a0                  # [230]
	movl     a2_l,%esi
#;;   sub.l     d0,a0                  # [231]
	subl     %ebx,%esi
#;;   addq.w    #1,d1                  # [232]
	addw     $1,%dx
ISC2P34:
#;;   move.b    (a0)+,d0               # [233]
	movb     (%esi),%bl
	lea      1(%esi),%esi
#;;   addq.l    #1,d3                  # [234]
	addl     $1,d3_l
#;;   tst.w     d4                     # [235 EQ]
	testw    $0xffff,d4_w
#;;   beq.b     isc234a                # [236]
	je       ISC234a
#;;   move.b    d0,(a2)+               # [237]
	movl     a2_l,%ecx
	addl     $1,a2_l
	movb     %bl,(%ecx)
ISC234a:
#;;   dbra      d1,isc2p34             # [238]
	decw     %dx
	cmpw     $-1,%dx
	jne      ISC2P34
#;;   bra.w     isc2p1                 # [239]
	jmp      ISC2P1
    
    
    
    
    
    
    
#;------------------------------------------------------------------------------
#; ISC Pass 3 Decruncher
#;
#; IN :	A1 = Pointer To Crunched Data
#;	A2 = Pointer To Destination
#;	D5 = Count Mode (0=On)
#;	D7 = If Count Mode: 0, Else Anything
#;
#; OUT:	D7 = Length Of Decrunched Data
#;
    .globl ISC3P
    .globl _ISC3P
ISC3P:
_ISC3P:
    movl 4(%esp),%edi

    movl 8(%esp), %eax
    movl %eax,a2_l
    
    movl 12(%esp),%eax
    movl %eax,d5_w
    
    movl 16(%esp),%eax
    movl %eax,d7_l
    
    movl a2_l,%eax
    movl %eax,UI_CrunchAdrTemp
    movl 20(%esp),%eax
    movl %eax,UI_CrunchLenTemp
    
    

#;;   move.l    a4,-(sp)               # [251]
	pushl    a4_l
#;;   move.l    UI_CrunchAdrTemp,a6    # [252]
	movl     UI_CrunchAdrTemp,%ebp
#;;   move.l    UI_CrunchLenTemp,d0    # [253]
	movl     UI_CrunchLenTemp,%ebx
#;;   subq.l    #4+1,d0                # [254]
	subl     $4+1,%ebx
#;;   add.l     d0,a6                  # [255]
	lea      (%ebp,%ebx),%ebp
#;;   moveq     #0,d0                  # [257]
	movl     $0,%ebx
#;;   move.b    $6b(a0),d0             # [258]
	movb     0x6b(%esi),%bl
#;;   move.b    #$80,d4                # [259]
	movb     $0x80,d4_b
#;;   lea       $e4(a0),a3             # [261]
	lea      0xe4(%esi),%eax
	movl     %eax,a3_l
#;;   lea       $100(a3),a4            # [262]
	movl     a3_l,%ecx
	lea      0x100(%ecx),%eax
#;;   move.l    a4,a5                  # [263]
	movl     %eax,a5_l
#;;   add.l     d0,a5                  # [264]
	addl     %ebx,a5_l
#;;   add.l     d0,a1                  # [265]
	lea      (%edi,%ebx),%edi
#;;   add.l     d0,a1                  # [266]
	lea      (%edi,%ebx),%edi
	movl     %eax,a4_l
ISC3P1:
#;;   moveq     #0,d0                  # [268]
	movl     $0,%ebx
#;;   move.w    #$00ff,d1              # [269]
	movw     $0xff,%dx
ISC3P2:
#;;   addq.b    #1,d1                  # [270]
	addb     $1,%dl
#;;   lsl.b     #1,d4                  # [271 NE X]
	shlb     $1,d4_b
	setcb    xflag
#;;   bne.b     isc3p4                 # [272]
	jne      ISC3P4
#;;   move.b    (a1)+,d4               # [273 NE]
	movb     (%edi),%al
	movb     %al,d4_b
	testb    %al,%al
	lea      1(%edi),%edi
#;;   bne.b     isc3p3                 # [274]
	jne      ISC3P3
#;;   cmp.l     a6,a1                  # [275 CC]
	cmpl     %ebp,%edi
#;;   bcc.b     isc3p5                 # [276]
	jnc      ISC3P5
ISC3P3:
#;;   roxl.b    #1,d4                  # [277 X]
	btw      $0,xflag
	rclb     $1,d4_b
	setcb    xflag
ISC3P4:
#;;   roxl.b    #1,d0                  # [278]
	btw      $0,xflag
	rclb     $1,%bl
#;;   cmp.b     (a4,d1.w),d0           # [279 CS]
	movswl   %dx,%ecx
	addl     a4_l,%ecx
	cmpb     0(%ecx),%bl
#;;   bcs.b     isc3p2                 # [280]
	jc       ISC3P2
#;;   add.b     (a5,d1.w),d0           # [281]
	movswl   %dx,%ecx
	addl     a5_l,%ecx
	addb     0(%ecx),%bl
#;;   addq.l    #1,d7                  # [282]
	addl     $1,d7_l
#;;   tst.w     d6                     # [283 EQ]
	testw    $0xffff,d6_w
#;;   beq.b     isc3p1                 # [284]
	je       ISC3P1
#;;   move.b    (a3,d0.w),(a2)+        # [285]
	movswl   %bx,%ecx
	addl     a3_l,%ecx
	movb     0(%ecx),%al
	movl     a2_l,%ecx
	addl     $1,a2_l
	movb     %al,(%ecx)
#;;   bra.b     isc3p1                 # [286]
	jmp      ISC3P1
ISC3P5:
#;;   move.l    (sp)+,a4               # [288]
	popl     a4_l
#;;   rts                              # [289]
    movl d7_l,%eax
	ret      
