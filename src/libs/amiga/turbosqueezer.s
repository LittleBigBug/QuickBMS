#;------------------------------------------------------------------------------
#; TurboSqueeze 6.1 Unpacker
#;
#; IN :	A0 = End Of Crunched Data
#;	A1 = End Of Decruncher Buffer
#;	A2 = Start Of Decruncher Buffer
#;
    .text
    .globl UTSQ61
    .globl _UTSQ61
UTSQ61:
_UTSQ61:
    movl 4(%esp),%esi
    
    movl 8(%esp),%edi
    
    movl 12(%esp),%eax
    movl %eax,a2_l
    


#;;   moveq     #-1,d5                 # [9]
	movl     $-1,d5_l
#;;   moveq     #$3f,d6                # [10]
	movl     $0x3f,d6_l
#;;   moveq     #0,d7                  # [11]
	movl     $0,d7_l
UTS611:
#;;   moveq     #0,d3                  # [12]
	movl     $0,d3_l
#;;   lsr.l     #1,d7                  # [13 CC NE]
	shrl     $1,d7_l
#;;   bne.b     uts612                 # [14]
	jne      UTS612
#;;   bsr.b     uts6116                # [15 (CC)]
	call     UTS6116
UTS612:
#;;   bcc.b     uts6111                # [16]
	jnc      UTS6111
#;;   lsr.l     #1,d7                  # [17 CS NE]
	shrl     $1,d7_l
#;;   bne.b     uts613                 # [18]
	jne      UTS613
#;;   bsr.b     uts6116                # [19 (CS)]
	call     UTS6116
UTS613:
#;;   bcs.b     uts614                 # [20]
	jc       UTS614
#;;   moveq     #0,d2                  # [21]
	movl     $0,d2_l
#;;   bra.b     uts6110                # [22]
	jmp      UTS6110
UTS614:
#;;   lsr.l     #1,d7                  # [24 CS NE]
	shrl     $1,d7_l
#;;   bne.b     uts615                 # [25]
	jne      UTS615
#;;   bsr.b     uts6116                # [26 (CS)]
	call     UTS6116
UTS615:
#;;   bcs.b     uts616                 # [27]
	jc       UTS616
#;;   moveq     #1,d2                  # [28]
	movl     $1,d2_l
#;;   moveq     #0,d1                  # [29]
	movl     $0,%edx
#;;   bra.b     uts619                 # [30]
	jmp      UTS619
UTS616:
#;;   lsr.l     #1,d7                  # [32 CC NE]
	shrl     $1,d7_l
#;;   bne.b     uts617                 # [33]
	jne      UTS617
#;;   bsr.b     uts6116                # [34 (CC)]
	call     UTS6116
UTS617:
#;;   bcc.b     uts618                 # [35]
	jnc      UTS618
#;;   moveq     #7,d2                  # [36]
	movl     $7,d2_l
#;;   moveq     #5,d1                  # [37]
	movl     $5,%edx
#;;   bra.b     uts619                 # [38]
	jmp      UTS619
UTS618:
#;;   moveq     #1,d1                  # [40]
	movl     $1,%edx
#;;   moveq     #3,d2                  # [41]
	movl     $3,d2_l
UTS619:
#;;   bsr.b     uts6117                # [42]
	call     UTS6117
#;;   move.w    d0,d3                  # [43]
	movw     %bx,d3_w
#;;   add.w     d0,d2                  # [44]
	addw     %bx,d2_w
UTS6110:
#;;   moveq     #7,d1                  # [45]
	movl     $7,%edx
#;;   bsr.b     uts6117                # [46]
	call     UTS6117
#;;   move.b    d0,-(a1)               # [47]
	lea      -1(%edi),%edi
	movb     %bl,(%edi)
#;;   cmp.l     a2,a1                  # [48 EQ]
	cmpl     a2_l,%edi
#;;   dbeq      d2,uts6110             # [49]
	je       _PA_37_
	decw     d2_w
	cmpw     $-1,d2_w
	jne      UTS6110
_PA_37_:         
#;;   cmp.w     d5,d2                  # [50 NE]
	movw     d5_w,%ax
	cmpw     %ax,d2_w
#;;   bne.b     uts6115                # [51]
	jne      UTS6115
UTS6111:
#;;   cmp.w     d6,d3                  # [53 EQ]
	movw     d6_w,%ax
	cmpw     %ax,d3_w
#;;   beq.b     uts611                 # [54]
	je       UTS611
#;;   lsr.l     #1,d7                  # [55 CS NE]
	shrl     $1,d7_l
#;;   bne.b     uts6112                # [56]
	jne      UTS6112
#;;   bsr.b     uts6116                # [57 (CS)]
	call     UTS6116
UTS6112:
#;;   bcs.b     uts6120                # [58]
	jc       UTS6120
#;;   moveq     #11,d1                 # [59]
	movl     $11,%edx
#;;   bsr.b     uts6117                # [60]
	call     UTS6117
#;;   move.w    d0,d3                  # [61]
	movw     %bx,d3_w
#;;   moveq     #2,d1                  # [62]
	movl     $2,%edx
UTS6113:
#;;   bsr.b     uts6117                # [63]
	call     UTS6117
#;;   addq.w    #2,d0                  # [64]
	addw     $2,%bx
UTS6114:
#;;   move.b    (a1,d3.w),-(a1)        # [65]
	lea      -1(%edi),%edi
	movswl   d3_w,%ecx
	movb     0(%edi,%ecx),%al
	movb     %al,(%edi)
#;;   cmp.l     a2,a1                  # [66 EQ]
	cmpl     a2_l,%edi
#;;   dbeq      d0,uts6114             # [67]
	je       _PA_54_
	decw     %bx
	cmpw     $-1,%bx
	jne      UTS6114
_PA_54_:         
#;;   cmp.w     d5,d0                  # [68 EQ]
	cmpw     d5_w,%bx
#;;   beq.b     uts611                 # [69]
	je       UTS611
UTS6115:
#;;   rts                              # [70]
	ret      
UTS6116:
#;;   move.l    -(a0),d7               # [72]
	lea      -4(%esi),%esi
	movl     (%esi),%eax
	movl     %eax,d7_l
#;;   move.w    #$0010,ccr             # [73 X]
	movb     $((0x10)>>4)&1,xflag
#;;   roxr.l    #1,d7                  # [74 CC CS]
	btw      $0,xflag
	rcrl     $1,d7_l
#;;   rts                              # [75]
	ret      
UTS6117:
#;;   moveq     #0,d0                  # [77]
	movl     $0,%ebx
UTS6118:
#;;   lsr.l     #1,d7                  # [78 NE X]
	shrl     $1,d7_l
	setcb    xflag
#;;   bne.b     uts6119                # [79]
	jne      UTS6119
#;;   move.l    -(a0),d7               # [80]
	lea      -4(%esi),%esi
	movl     (%esi),%eax
	movl     %eax,d7_l
#;;   move.w    #$0010,ccr             # [81 X]
	movb     $((0x10)>>4)&1,xflag
#;;   roxr.l    #1,d7                  # [82 X]
	btw      $0,xflag
	rcrl     $1,d7_l
	setcb    xflag
UTS6119:
#;;   roxl.w    #1,d0                  # [83]
	btw      $0,xflag
	rclw     $1,%bx
#;;   dbra      d1,uts6118             # [84]
	decw     %dx
	cmpw     $-1,%dx
	jne      UTS6118
#;;   rts                              # [85]
	ret      
UTS6120:
#;;   lsr.l     #1,d7                  # [87 CS NE]
	shrl     $1,d7_l
#;;   bne.b     uts6121                # [88]
	jne      UTS6121
#;;   bsr.b     uts6116                # [89 (CS)]
	call     UTS6116
UTS6121:
#;;   bcs.b     uts6122                # [90]
	jc       UTS6122
#;;   moveq     #5,d1                  # [91]
	movl     $5,%edx
#;;   bsr.b     uts6117                # [92]
	call     UTS6117
#;;   move.w    d0,d3                  # [93]
	movw     %bx,d3_w
#;;   moveq     #1,d1                  # [94]
	movl     $1,%edx
#;;   bra.b     uts6113                # [95]
	jmp      UTS6113
UTS6122:
#;;   lsr.l     #1,d7                  # [97 CS NE]
	shrl     $1,d7_l
#;;   bne.b     uts6123                # [98]
	jne      UTS6123
#;;   bsr.b     uts6116                # [99 (CS)]
	call     UTS6116
UTS6123:
#;;   bcs.b     uts6125                # [100]
	jc       UTS6125
#;;   moveq     #9,d1                  # [101]
	movl     $9,%edx
#;;   moveq     #64,d2                 # [102]
	movl     $64,d2_l
UTS6124:
#;;   bsr.b     uts6117                # [103]
	call     UTS6117
#;;   add.w     d2,d0                  # [104]
	addw     d2_w,%bx
#;;   move.b    (a1,d0.w),-(a1)        # [105]
	lea      -1(%edi),%edi
	movswl   %bx,%ecx
	movb     0(%edi,%ecx),%al
	movb     %al,(%edi)
#;;   cmp.l     a2,a1                  # [106 EQ]
	cmpl     a2_l,%edi
#;;   beq.b     uts6115                # [107]
	je       UTS6115
#;;   move.b    (a1,d0.w),-(a1)        # [108]
	lea      -1(%edi),%edi
	movb     0(%edi,%ecx),%al
	movb     %al,(%edi)
#;;   cmp.l     a2,a1                  # [109 NE]
	cmpl     a2_l,%edi
#;;   bne.w     uts611                 # [110]
	jne      UTS611
#;;   rts                              # [111]
	ret      
UTS6125:
#;;   lsr.l     #1,d7                  # [113 CS NE]
	shrl     $1,d7_l
#;;   bne.b     uts6126                # [114]
	jne      UTS6126
#;;   bsr.b     uts6116                # [115 (CS)]
	call     UTS6116
UTS6126:
#;;   bcs.b     uts6127                # [116]
	jc       UTS6127
#;;   moveq     #5,d1                  # [117]
	movl     $5,%edx
#;;   moveq     #0,d2                  # [118]
	movl     $0,d2_l
#;;   bra.b     uts6124                # [119]
	jmp      UTS6124
UTS6127:
#;;   lsr.l     #1,d7                  # [121 CS NE]
	shrl     $1,d7_l
#;;   bne.b     uts6128                # [122]
	jne      UTS6128
#;;   bsr.b     uts6116                # [123 (CS)]
	call     UTS6116
UTS6128:
#;;   bcs.b     uts6130                # [124]
	jc       UTS6130
#;;   moveq     #7,d1                  # [125]
	movl     $7,%edx
#;;   bsr.b     uts6117                # [126]
	call     UTS6117
#;;   moveq     #4,d1                  # [127]
	movl     $4,%edx
UTS6129:
#;;   move.w    d0,d3                  # [128]
	movw     %bx,d3_w
#;;   bsr.b     uts6117                # [129]
	call     UTS6117
#;;   add.w     #10,d0                 # [130]
	addw     $10,%bx
#;;   bra.w     uts6114                # [131]
	jmp      UTS6114
UTS6130:
#;;   moveq     #11,d1                 # [133]
	movl     $11,%edx
#;;   bsr.b     uts6117                # [134]
	call     UTS6117
#;;   moveq     #6,d1                  # [135]
	movl     $6,%edx
#;;   bra.b     uts6129                # [136]
	jmp      UTS6129
    
    
    
    
    
#;------------------------------------------------------------------------------
#; TurboSqueeze 8.0 Unpacker
#;
#; IN :	A0 = End Of Crunched Data
#;	A1 = End Of Decruncher Buffer
#;	A2 = Start Of Decruncher Buffer
#;
    .globl UTSQ80
    .globl _UTSQ80
UTSQ80:
_UTSQ80:
    movl 4(%esp),%esi
    
    movl 8(%esp),%edi
    
    movl 12(%esp),%eax
    movl %eax,a2_l
    
    

#;;   moveq     #-1,d5                 # [145]
	movl     $-1,d5_l
#;;   moveq     #$3f,d6                # [146]
	movl     $0x3f,d6_l
#;;   moveq     #0,d7                  # [147]
	movl     $0,d7_l
UTS801:
#;;   moveq     #0,d3                  # [148]
	movl     $0,d3_l
#;;   add.l     d7,d7                  # [149 CC NE]
	movl     d7_l,%eax
	addl     %eax,d7_l
#;;   bne.b     uts802                 # [150]
	jne      UTS802
#;;   bsr.b     uts8016                # [151 (CC)]
	call     UTS8016
UTS802:
#;;   bcc.b     uts8011                # [152]
	jnc      UTS8011
#;;   add.l     d7,d7                  # [153 CS NE]
	movl     d7_l,%eax
	addl     %eax,d7_l
#;;   bne.b     uts803                 # [154]
	jne      UTS803
#;;   bsr.b     uts8016                # [155 (CS)]
	call     UTS8016
UTS803:
#;;   bcs.b     uts804                 # [156]
	jc       UTS804
#;;   moveq     #0,d2                  # [157]
	movl     $0,d2_l
#;;   bra.b     uts8010                # [158]
	jmp      UTS8010
UTS804:
#;;   add.l     d7,d7                  # [160 CS NE]
	movl     d7_l,%eax
	addl     %eax,d7_l
#;;   bne.b     uts805                 # [161]
	jne      UTS805
#;;   bsr.b     uts8016                # [162 (CS)]
	call     UTS8016
UTS805:
#;;   bcs.b     uts806                 # [163]
	jc       UTS806
#;;   moveq     #1,d2                  # [164]
	movl     $1,d2_l
#;;   moveq     #0,d1                  # [165]
	movl     $0,%edx
#;;   bra.b     uts809                 # [166]
	jmp      UTS809
UTS806:
#;;   add.l     d7,d7                  # [168 CC NE]
	movl     d7_l,%eax
	addl     %eax,d7_l
#;;   bne.b     uts807                 # [169]
	jne      UTS807
#;;   bsr.b     uts8016                # [170 (CC)]
	call     UTS8016
UTS807:
#;;   bcc.b     uts808                 # [171]
	jnc      UTS808
#;;   moveq     #7,d2                  # [172]
	movl     $7,d2_l
#;;   moveq     #5,d1                  # [173]
	movl     $5,%edx
#;;   bra.b     uts809                 # [174]
	jmp      UTS809
UTS808:
#;;   moveq     #1,d1                  # [176]
	movl     $1,%edx
#;;   moveq     #3,d2                  # [177]
	movl     $3,d2_l
UTS809:
#;;   bsr.b     uts8017                # [178]
	call     UTS8017
#;;   move.w    d0,d3                  # [179]
	movw     %bx,d3_w
#;;   add.w     d0,d2                  # [180]
	addw     %bx,d2_w
UTS8010:
#;;   moveq     #7,d1                  # [181]
	movl     $7,%edx
#;;   bsr.b     uts8017                # [182]
	call     UTS8017
#;;   move.b    d0,-(a1)               # [183]
	lea      -1(%edi),%edi
	movb     %bl,(%edi)
#;;   cmp.l     a2,a1                  # [184 EQ]
	cmpl     a2_l,%edi
#;;   dbeq      d2,uts8010             # [185]
	je       _PA_154_
	decw     d2_w
	cmpw     $-1,d2_w
	jne      UTS8010
_PA_154_:         
#;;   cmp.w     d5,d2                  # [186 NE]
	movw     d5_w,%ax
	cmpw     %ax,d2_w
#;;   bne.b     uts8015                # [187]
	jne      UTS8015
UTS8011:
#;;   cmp.w     d6,d3                  # [189 EQ]
	movw     d6_w,%ax
	cmpw     %ax,d3_w
#;;   beq.b     uts801                 # [190]
	je       UTS801
#;;   add.l     d7,d7                  # [191 CS NE]
	movl     d7_l,%eax
	addl     %eax,d7_l
#;;   bne.b     uts8012                # [192]
	jne      UTS8012
#;;   bsr.b     uts8016                # [193 (CS)]
	call     UTS8016
UTS8012:
#;;   bcs.b     uts8020                # [194]
	jc       UTS8020
#;;   moveq     #11,d1                 # [195]
	movl     $11,%edx
#;;   bsr.b     uts8017                # [196]
	call     UTS8017
#;;   move.w    d0,d3                  # [197]
	movw     %bx,d3_w
#;;   moveq     #2,d1                  # [198]
	movl     $2,%edx
UTS8013:
#;;   bsr.b     uts8017                # [199]
	call     UTS8017
#;;   addq.w    #2,d0                  # [200]
	addw     $2,%bx
UTS8014:
#;;   move.b    (a1,d3.w),-(a1)        # [201]
	lea      -1(%edi),%edi
	movswl   d3_w,%ecx
	movb     0(%edi,%ecx),%al
	movb     %al,(%edi)
#;;   cmp.l     a2,a1                  # [202 EQ]
	cmpl     a2_l,%edi
#;;   dbeq      d0,uts8014             # [203]
	je       _PA_171_
	decw     %bx
	cmpw     $-1,%bx
	jne      UTS8014
_PA_171_:         
#;;   cmp.w     d5,d0                  # [204 EQ]
	cmpw     d5_w,%bx
#;;   beq.b     uts801                 # [205]
	je       UTS801
UTS8015:
#;;   rts                              # [206]
	ret      
UTS8016:
#;;   move.l    -(a0),d7               # [208]
	lea      -4(%esi),%esi
	movl     (%esi),%eax
	movl     %eax,d7_l
#;;   move.w    #$0010,ccr             # [209 X]
	movb     $((0x10)>>4)&1,xflag
#;;   addx.l    d7,d7                  # [210 CC CS]
	movl     d7_l,%eax
	btw      $0,xflag
	adcl     %eax,d7_l
#;;   rts                              # [211]
	ret      
UTS8017:
#;;   moveq     #0,d0                  # [213]
	movl     $0,%ebx
UTS8018:
#;;   add.l     d7,d7                  # [214 NE X]
	movl     d7_l,%eax
	addl     %eax,d7_l
	setcb    xflag
#;;   bne.b     uts8019                # [215]
	jne      UTS8019
#;;   move.l    -(a0),d7               # [216]
	lea      -4(%esi),%esi
	movl     (%esi),%eax
	movl     %eax,d7_l
#;;   move.w    #$0010,ccr             # [217 X]
	movb     $((0x10)>>4)&1,xflag
#;;   addx.l    d7,d7                  # [218 X]
	movl     d7_l,%eax
	btw      $0,xflag
	adcl     %eax,d7_l
	setcb    xflag
UTS8019:
#;;   roxl.w    #1,d0                  # [219]
	btw      $0,xflag
	rclw     $1,%bx
#;;   dbra      d1,uts8018             # [220]
	decw     %dx
	cmpw     $-1,%dx
	jne      UTS8018
#;;   rts                              # [221]
	ret      
UTS8020:
#;;   add.l     d7,d7                  # [223 CS NE]
	movl     d7_l,%eax
	addl     %eax,d7_l
#;;   bne.b     uts8021                # [224]
	jne      UTS8021
#;;   bsr.b     uts8016                # [225 (CS)]
	call     UTS8016
UTS8021:
#;;   bcs.b     uts8022                # [226]
	jc       UTS8022
#;;   moveq     #5,d1                  # [227]
	movl     $5,%edx
#;;   bsr.b     uts8017                # [228]
	call     UTS8017
#;;   move.w    d0,d3                  # [229]
	movw     %bx,d3_w
#;;   moveq     #1,d1                  # [230]
	movl     $1,%edx
#;;   bra.b     uts8013                # [231]
	jmp      UTS8013
UTS8022:
#;;   add.l     d7,d7                  # [233 CS NE]
	movl     d7_l,%eax
	addl     %eax,d7_l
#;;   bne.b     uts8023                # [234]
	jne      UTS8023
#;;   bsr.b     uts8016                # [235 (CS)]
	call     UTS8016
UTS8023:
#;;   bcs.b     uts8025                # [236]
	jc       UTS8025
#;;   moveq     #9,d1                  # [237]
	movl     $9,%edx
#;;   moveq     #64,d2                 # [238]
	movl     $64,d2_l
UTS8024:
#;;   bsr.b     uts8017                # [239]
	call     UTS8017
#;;   add.w     d2,d0                  # [240]
	addw     d2_w,%bx
#;;   move.b    (a1,d0.w),-(a1)        # [241]
	lea      -1(%edi),%edi
	movswl   %bx,%ecx
	movb     0(%edi,%ecx),%al
	movb     %al,(%edi)
#;;   cmp.l     a2,a1                  # [242 EQ]
	cmpl     a2_l,%edi
#;;   beq.b     uts8015                # [243]
	je       UTS8015
#;;   move.b    (a1,d0.w),-(a1)        # [244]
	lea      -1(%edi),%edi
	movb     0(%edi,%ecx),%al
	movb     %al,(%edi)
#;;   cmp.l     a2,a1                  # [245 NE]
	cmpl     a2_l,%edi
#;;   bne.w     uts801                 # [246]
	jne      UTS801
#;;   rts                              # [247]
	ret      
UTS8025:
#;;   add.l     d7,d7                  # [249 CS NE]
	movl     d7_l,%eax
	addl     %eax,d7_l
#;;   bne.b     uts8026                # [250]
	jne      UTS8026
#;;   bsr.b     uts8016                # [251 (CS)]
	call     UTS8016
UTS8026:
#;;   bcs.b     uts8027                # [252]
	jc       UTS8027
#;;   moveq     #5,d1                  # [253]
	movl     $5,%edx
#;;   moveq     #0,d2                  # [254]
	movl     $0,d2_l
#;;   bra.b     uts8024                # [255]
	jmp      UTS8024
UTS8027:
#;;   add.l     d7,d7                  # [257 CS NE]
	movl     d7_l,%eax
	addl     %eax,d7_l
#;;   bne.b     uts8028                # [258]
	jne      UTS8028
#;;   bsr.b     uts8016                # [259 (CS)]
	call     UTS8016
UTS8028:
#;;   bcs.b     uts8030                # [260]
	jc       UTS8030
#;;   moveq     #7,d1                  # [261]
	movl     $7,%edx
#;;   bsr.b     uts8017                # [262]
	call     UTS8017
#;;   moveq     #4,d1                  # [263]
	movl     $4,%edx
UTS8029:
#;;   move.w    d0,d3                  # [264]
	movw     %bx,d3_w
#;;   bsr.b     uts8017                # [265]
	call     UTS8017
#;;   add.w     #10,d0                 # [266]
	addw     $10,%bx
#;;   bra.w     uts8014                # [267]
	jmp      UTS8014
UTS8030:
#;;   moveq     #11,d1                 # [269]
	movl     $11,%edx
#;;   bsr.b     uts8017                # [270]
	call     UTS8017
#;;   moveq     #6,d1                  # [271]
	movl     $6,%edx
#;;   bra.b     uts8029                # [272]
	jmp      UTS8029
