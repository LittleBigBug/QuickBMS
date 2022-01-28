#; ------------------------------------------
#; PackFire 1.2i - (large depacker)
#; ------------------------------------------
#; ------------------------------------------
#; packed data in a0
#; dest in a1
#; probs buffer in a2 (must be 15980 bytes)

    .text
    .globl  packfire
    .globl  _packfire
packfire:
_packfire:
    movl 4(%esp),%esi
    movl 8(%esp),%edi
    movl 12(%esp),%eax
    movl %eax,a2_l


#;;   KTOPVALUE               =       16777216 # [7]
	.equ	KTOPVALUE,16777216
#;;   KBITMODELTOTAL          =       2048 # [8]
	.equ	KBITMODELTOTAL,2048
#;;   KNUMMOVEBITS            =       5 # [9]
	.equ	KNUMMOVEBITS,5
#;;   KNUMPOSBITSMAX          =       4 # [10]
	.equ	KNUMPOSBITSMAX,4
#;;   KLENNUMLOWBITS          =       3 # [11]
	.equ	KLENNUMLOWBITS,3
#;;   KLENNUMMIDBITS          =       3 # [12]
	.equ	KLENNUMMIDBITS,3
#;;   KLENNUMLOWSYMBOLS       =       8 # [13]
	.equ	KLENNUMLOWSYMBOLS,8
#;;   KLENNUMMIDSYMBOLS       =       8 # [14]
	.equ	KLENNUMMIDSYMBOLS,8
#;;   KLENNUMHIGHBITS         =       8 # [15]
	.equ	KLENNUMHIGHBITS,8
#;;   LENCHOICE2              =       1 # [16]
	.equ	LENCHOICE2,1
#;;   LENLOW                  =       2 # [17]
	.equ	LENLOW,2
#;;   LENMID                  =       130 # [18]
	.equ	LENMID,130
#;;   LENHIGH                 =       258 # [19]
	.equ	LENHIGH,258
#;;   KNUMLITSTATES           =       7 # [20]
	.equ	KNUMLITSTATES,7
#;;   KSTARTPOSMODELINDEX     =       4 # [21]
	.equ	KSTARTPOSMODELINDEX,4
#;;   KENDPOSMODELINDEX       =       14 # [22]
	.equ	KENDPOSMODELINDEX,14
#;;   KNUMPOSSLOTBITS         =       6 # [23]
	.equ	KNUMPOSSLOTBITS,6
#;;   KNUMLENTOPOSSTATES      =       4 # [24]
	.equ	KNUMLENTOPOSSTATES,4
#;;   KNUMALIGNBITS           =       4 # [25]
	.equ	KNUMALIGNBITS,4
#;;   KMATCHMINLEN            =       2 # [26]
	.equ	KMATCHMINLEN,2
#;;   ISREP                   =       192 # [27]
	.equ	ISREP,192
#;;   ISREPG0                 =       204 # [28]
	.equ	ISREPG0,204
#;;   ISREPG1                 =       216 # [29]
	.equ	ISREPG1,216
#;;   ISREPG2                 =       228 # [30]
	.equ	ISREPG2,228
#;;   ISREP0LONG              =       240 # [31]
	.equ	ISREP0LONG,240
#;;   POSSLOT                 =       432 # [32]
	.equ	POSSLOT,432
#;;   SPECPOS                 =       688 # [33]
	.equ	SPECPOS,688
#;;   ALIGN                   =       802 # [34]
	.equ	ALIGN,802
#;;   LENCODER                =       818 # [35]
	.equ	LENCODER,818
#;;   REPLENCODER             =       1332 # [36]
	.equ	REPLENCODER,1332
#;;   LITERAL                 =       1846 # [37]
	.equ	LITERAL,1846
start:
#;;   lea       var(pc),a5             # [43]
	movl     $var,a5_l
#;;   movem.l   (a0)+,d0/d5            # [44]
	movl     0(%esi),%ebx
	movl     4(%esi),%eax
	movl     %eax,d5_l
	addl     $8,%esi
#;;   move.l    d0,d2                  # [45]
	movl     %ebx,d2_l
#;;   lea       (a2),a6                # [46]
	movl     a2_l,%ebp
#;;   lea       (a1),a3                # [47]
	movl     %edi,a3_l
clear_dest:
#;;   sf.b      (a3)+                  # [48]
	movl     a3_l,%ecx
	addl     $1,a3_l
	movb     $0,(%ecx)
#;;   subq.l    #1,d0                  # [49 GE]
	subl     $1,%ebx
#;;   bge.b     clear_dest             # [50]
	jge      clear_dest
#;;   move.w    #7990-1,d7             # [51]
	movw     $7990-1,d7_w
fill_probs:
#;;   move.w    #KBITMODELTOTAL>>1,(a2)+ # [52]
	movl     a2_l,%ecx
	addl     $2,a2_l
	movw     $KBITMODELTOTAL>>1,(%ecx)
#;;   dbf       d7,fill_probs          # [53]
	decw     d7_w
	cmpw     $-1,d7_w
	jne      fill_probs
#;;   lea       (a1),a4                # [54]
	movl     %edi,a4_l
#;;   moveq     #0,d3                  # [55]
	movl     $0,d3_l
#;;   moveq     #0,d4                  # [56]
	movl     $0,d4_l
#;;   moveq     #0,d6                  # [57]
	movl     $0,d6_l
#;;   moveq     #-1,d7                 # [58]
	movl     $-1,d7_l
#;;   move.l    d7,(a5)+               # [59]
	movl     d7_l,%eax
	movl     a5_l,%ecx
	movl     %eax,0(%ecx)
#;;   neg.l     d7                     # [60]
	negl     d7_l
#;;   move.l    d7,(a5)+               # [61]
	movl     d7_l,%eax
	movl     %eax,4(%ecx)
#;;   move.l    d7,(a5)+               # [62]
	addl     $12,a5_l
	movl     %eax,8(%ecx)
#;;   move.l    d7,(a5)                # [63]
	movl     a5_l,%ecx
	movl     %eax,(%ecx)
#;;   lea       -12(a5),a5             # [64]
	addl     $(-12),a5_l
#;;   move.l    d6,a2                  # [65]
	movl     d6_l,%eax
	movl     %eax,a2_l
depack_loop:
#;;   move.l    d2,-(a7)               # [66]
	pushl    d2_l
#;;   lea       (a6),a1                # [67]
	movl     %ebp,%edi
#;;   move.l    d4,d0                  # [68]
	movl     d4_l,%ebx
#;;   lsl.l     #KNUMPOSBITSMAX,d0     # [69]
	shll     $KNUMPOSBITSMAX,%ebx
#;;   bsr.w     Check_Fix_Range2       # [70 (NE)]
	call     Check_Fix_Range2
#;;   bne.b     fix_range1             # [71]
	jne      fix_range1
#;;   lea       (LITERAL*2)(a6),a3     # [72]
	lea      LITERAL*2(%ebp),%eax
#;;   moveq     #1,d3                  # [73]
	movl     $1,d3_l
#;;   cmp.w     #KNUMLITSTATES,d4      # [74 MI]
	cmpw     $KNUMLITSTATES,d4_w
	movl     %eax,a3_l
#;;   bmi.b     max_lit_state_2        # [75]
	js       max_lit_state_2
#;;   move.l    a2,d0                  # [76]
	movl     a2_l,%ebx
#;;   sub.l     d7,d0                  # [77]
	subl     d7_l,%ebx
#;;   moveq     #0,d1                  # [78]
	movl     $0,%edx
#;;   move.b    (a4,d0.l),d1           # [79]
	movl     a4_l,%ecx
	movb     0(%ecx,%ebx),%dl
max_lit_loop1:
#;;   add.l     d1,d1                  # [80]
	addl     %edx,%edx
#;;   move.l    d1,d2                  # [81]
	movl     %edx,d2_l
#;;   and.l     #$100,d2               # [82]
	andl     $0x100,d2_l
#;;   move.l    d2,d0                  # [83]
	movl     d2_l,%ebx
#;;   add.w     #$100,d0               # [84]
	addw     $0x100,%bx
#;;   add.w     d0,d0                  # [85]
	addw     %bx,%bx
#;;   lea       (a3,d0.l),a1           # [86]
	movl     a3_l,%ecx
	lea      0(%ecx,%ebx),%edi
#;;   bsr.w     Check_Code_Bound       # [87 (NE)]
	call     Check_Code_Bound
#;;   bne.b     Check_Code_Bound_1     # [88]
	jne      Check_Code_Bound_1
#;;   tst.l     d2                     # [89 NE]
	testl    $0xffffffff,d2_l
#;;   bne.b     max_lit_state_2        # [90]
	jne      max_lit_state_2
#;;   bra.b     No_Check_Code_Bound_1  # [91]
	jmp      No_Check_Code_Bound_1
Check_Code_Bound_1:
#;;   tst.l     d2                     # [92 EQ]
	testl    $0xffffffff,d2_l
#;;   beq.b     max_lit_state_2        # [93]
	je       max_lit_state_2
No_Check_Code_Bound_1:
#;;   cmp.w     #$100,d3               # [94 MI]
	cmpw     $0x100,d3_w
#;;   bmi.b     max_lit_loop1          # [95]
	js       max_lit_loop1
max_lit_state_2:
#;;   cmp.w     #$100,d3               # [96 CC]
	cmpw     $0x100,d3_w
#;;   bhs.b     max_lit_state_exit     # [97]
	jnc      max_lit_state_exit
#;;   bsr.w     Check_Code_Bound2      # [98]
	call     Check_Code_Bound2
#;;   bra.b     max_lit_state_2        # [99]
	jmp      max_lit_state_2
	.data
	.p2align	2
#;;   table_state:            dc.b    0,0,0,0 # [100]
table_state:
	.byte	0,0,0,0
#;;                           dc.b    4-3,5-3,6-3,7-3,8-3,9-3 # [101]
	.byte	4-3,5-3,6-3,7-3,8-3,9-3
#;;                           dc.b    10-6,11-6 # [102]
	.byte	10-6,11-6
	.text
	.p2align	2
max_lit_state_exit:
#;;   move.b    d3,d0                  # [103]
	movb     d3_b,%bl
#;;   bsr.w     store_prev_byte2       # [104]
	call     store_prev_byte2
#;;   move.b    table_state(pc,d4.w),d4 # [105]
	movswl   d4_w,%ecx
	movb     table_state(%ecx),%al
	movb     %al,d4_b
#;;   bra.w     cont                   # [106]
	jmp      cont
fix_range1:
#;;   lea       (ISREP*2)(a6),a1       # [107]
	lea      ISREP*2(%ebp),%edi
#;;   bsr.w     Check_Fix_Range3       # [108 (NE)]
	call     Check_Fix_Range3
#;;   bne.b     Check_Fix_Range_2      # [109]
	jne      Check_Fix_Range_2
#;;   move.l    rep2-var(a5),rep3-var(a5) # [110]
	movl     a5_l,%ecx
	movl     rep2-var(%ecx),%eax
	movl     %eax,rep3-var(%ecx)
#;;   move.l    rep1-var(a5),rep2-var(a5) # [111]
	movl     rep1-var(%ecx),%eax
	movl     %eax,rep2-var(%ecx)
#;;   move.l    d7,rep1-var(a5)        # [112]
	movl     d7_l,%eax
	movl     %eax,rep1-var(%ecx)
#;;   move.l    d4,d0                  # [113]
	movl     d4_l,%ebx
#;;   moveq     #0,d4                  # [114]
	movl     $0,d4_l
#;;   cmp.w     #KNUMLITSTATES,d0      # [115 MI]
	cmpw     $KNUMLITSTATES,%bx
#;;   bmi.b     change_state_3         # [116]
	js       change_state_3
#;;   moveq     #3,d4                  # [117]
	movl     $3,d4_l
change_state_3:
#;;   lea       (LENCODER*2)(a6),a1    # [118]
	lea      LENCODER*2(%ebp),%edi
#;;   bra.b     Check_Fix_Range_3      # [119]
	jmp      Check_Fix_Range_3
Check_Fix_Range_2:
#;;   lea       (ISREPG0*2)(a6),a1     # [120]
	lea      ISREPG0*2(%ebp),%edi
#;;   bsr.w     Check_Fix_Range3       # [121 (NE)]
	call     Check_Fix_Range3
#;;   bne.b     Check_Fix_Range_4      # [122]
	jne      Check_Fix_Range_4
#;;   lea       (ISREP0LONG*2)(a6),a1  # [123]
	lea      ISREP0LONG*2(%ebp),%edi
#;;   move.l    d4,d0                  # [124]
	movl     d4_l,%ebx
#;;   lsl.l     #KNUMPOSBITSMAX,d0     # [125]
	shll     $KNUMPOSBITSMAX,%ebx
#;;   bsr.w     Check_Fix_Range2       # [126 (NE)]
	call     Check_Fix_Range2
#;;   bne.b     Check_Fix_Range_5      # [127]
	jne      Check_Fix_Range_5
#;;   move.l    d4,d0                  # [128]
	movl     d4_l,%ebx
#;;   moveq     #9,d4                  # [129]
	movl     $9,d4_l
#;;   cmp.w     #KNUMLITSTATES,d0      # [130 MI]
	cmpw     $KNUMLITSTATES,%bx
#;;   bmi.b     change_state_4         # [131]
	js       change_state_4
#;;   moveq     #11,d4                 # [132]
	movl     $11,d4_l
change_state_4:
#;;   bsr.w     store_prev_byte        # [133]
	call     store_prev_byte
#;;   bra.w     cont                   # [134]
	jmp      cont
Check_Fix_Range_4:
#;;   lea       (ISREPG1*2)(a6),a1     # [135]
	lea      ISREPG1*2(%ebp),%edi
#;;   bsr.w     Check_Fix_Range3       # [136 (NE)]
	call     Check_Fix_Range3
#;;   bne.b     Check_Fix_Range_6b     # [137]
	jne      Check_Fix_Range_6b
#;;   move.l    rep1-var(a5),d1        # [138]
	movl     a5_l,%ecx
	movl     rep1-var(%ecx),%edx
#;;   bra.b     Check_Fix_Range_7      # [139]
	jmp      Check_Fix_Range_7
Check_Fix_Range_6b:
#;;   lea       (ISREPG2*2)(a6),a1     # [140]
	lea      ISREPG2*2(%ebp),%edi
#;;   bsr.w     Check_Fix_Range3       # [141 (NE)]
	call     Check_Fix_Range3
#;;   bne.b     Check_Fix_Range_8      # [142]
	jne      Check_Fix_Range_8
#;;   move.l    rep2-var(a5),d1        # [143]
	movl     a5_l,%ecx
	movl     rep2-var(%ecx),%edx
#;;   bra.b     Check_Fix_Range_9      # [144]
	jmp      Check_Fix_Range_9
Check_Fix_Range_8:
#;;   move.l    rep3-var(a5),d1        # [145]
	movl     a5_l,%ecx
	movl     rep3-var(%ecx),%edx
#;;   move.l    rep2-var(a5),rep3-var(a5) # [146]
	movl     rep2-var(%ecx),%eax
	movl     %eax,rep3-var(%ecx)
Check_Fix_Range_9:
#;;   move.l    rep1-var(a5),rep2-var(a5) # [147]
	movl     a5_l,%ecx
	movl     rep1-var(%ecx),%eax
	movl     %eax,rep2-var(%ecx)
Check_Fix_Range_7:
#;;   move.l    d7,rep1-var(a5)        # [148]
	movl     d7_l,%eax
	movl     a5_l,%ecx
	movl     %eax,rep1-var(%ecx)
#;;   move.l    d1,d7                  # [149]
	movl     %edx,d7_l
Check_Fix_Range_5:
#;;   move.l    d4,d0                  # [150]
	movl     d4_l,%ebx
#;;   moveq     #8,d4                  # [151]
	movl     $8,d4_l
#;;   cmp.w     #KNUMLITSTATES,d0      # [152 MI]
	cmpw     $KNUMLITSTATES,%bx
#;;   bmi.b     change_state_5         # [153]
	js       change_state_5
#;;   moveq     #11,d4                 # [154]
	movl     $11,d4_l
change_state_5:
#;;   lea       (REPLENCODER*2)(a6),a1 # [155]
	lea      REPLENCODER*2(%ebp),%edi
Check_Fix_Range_3:
#;;   lea       (a1),a3                # [156]
	movl     %edi,a3_l
#;;   bsr.w     Check_Fix_Range        # [157 (NE)]
	call     Check_Fix_Range
#;;   bne.b     Check_Fix_Range_10     # [158]
	jne      Check_Fix_Range_10
#;;   lea       (LENLOW*2)+(KLENNUMLOWBITS*2)(a3),a3 # [159]
	addl     $(LENLOW*2+KLENNUMLOWBITS*2),a3_l
#;;   moveq     #0,d3                  # [160]
	movl     $0,d3_l
#;;   moveq     #KLENNUMLOWBITS,d1     # [161]
	movl     $KLENNUMLOWBITS,%edx
#;;   bra.b     Check_Fix_Range_11     # [162]
	jmp      Check_Fix_Range_11
Check_Fix_Range_10:
#;;   lea       (LENCHOICE2*2)(a3),a1  # [163]
	movl     a3_l,%ecx
	lea      LENCHOICE2*2(%ecx),%edi
#;;   bsr.w     Check_Fix_Range        # [164 (NE)]
	call     Check_Fix_Range
#;;   bne.b     Check_Fix_Range_12     # [165]
	jne      Check_Fix_Range_12
#;;   lea       (LENMID*2)+(KLENNUMMIDBITS*2)(a3),a3 # [166]
	addl     $(LENMID*2+KLENNUMMIDBITS*2),a3_l
#;;   moveq     #KLENNUMLOWSYMBOLS,d3  # [167]
	movl     $KLENNUMLOWSYMBOLS,d3_l
#;;   moveq     #KLENNUMMIDBITS,d1     # [168]
	movl     $KLENNUMMIDBITS,%edx
#;;   bra.b     Check_Fix_Range_11     # [169]
	jmp      Check_Fix_Range_11
Check_Fix_Range_12:
#;;   lea       (LENHIGH*2)(a3),a3     # [170]
	addl     $(LENHIGH*2),a3_l
#;;   moveq     #KLENNUMLOWSYMBOLS+KLENNUMMIDSYMBOLS,d3 # [171]
	movl     $KLENNUMLOWSYMBOLS+KLENNUMMIDSYMBOLS,d3_l
#;;   moveq     #KLENNUMHIGHBITS,d1    # [172]
	movl     $KLENNUMHIGHBITS,%edx
Check_Fix_Range_11:
#;;   move.l    d1,d2                  # [173]
	movl     %edx,d2_l
#;;   moveq     #1,d6                  # [174]
	movl     $1,d6_l
Check_Code_Bound_Loop:
#;;   exg.l     d6,d3                  # [175]
	movl     d6_l,%eax
	xchgl    %eax,d3_l
	movl     %eax,d6_l
#;;   bsr.w     Check_Code_Bound2      # [176]
	call     Check_Code_Bound2
#;;   exg.l     d6,d3                  # [177]
	movl     d6_l,%eax
	xchgl    %eax,d3_l
#;;   subq.l    #1,d2                  # [178 NE]
	subl     $1,d2_l
	movl     %eax,d6_l
#;;   bne.b     Check_Code_Bound_Loop  # [179]
	jne      Check_Code_Bound_Loop
#;;   moveq     #1,d0                  # [180]
	movl     $1,%ebx
#;;   lsl.l     d1,d0                  # [181]
	movl     %edx,%ecx
	shll     %cl,%ebx
#;;   sub.l     d0,d6                  # [182]
	subl     %ebx,d6_l
#;;   add.l     d3,d6                  # [183]
	movl     d3_l,%eax
	addl     %eax,d6_l
#;;   cmp.w     #4,d4                  # [184 CC]
	cmpw     $4,d4_w
#;;   bhs.w     change_state_6         # [185]
	jnc      change_state_6
#;;   addq.w    #KNUMLITSTATES,d4      # [186]
	addw     $KNUMLITSTATES,d4_w
#;;   move.l    d6,d0                  # [187]
	movl     d6_l,%ebx
#;;   cmp.w     #KNUMLENTOPOSSTATES,d0 # [188 MI]
	cmpw     $KNUMLENTOPOSSTATES,%bx
#;;   bmi.b     check_len              # [189]
	js       check_len
#;;   moveq     #KNUMLENTOPOSSTATES-1,d0 # [190]
	movl     $KNUMLENTOPOSSTATES-1,%ebx
check_len:
#;;   lea       (POSSLOT*2)(a6),a3     # [191]
	lea      POSSLOT*2(%ebp),%eax
#;;   lsl.l     #KNUMPOSSLOTBITS+1,d0  # [192]
	shll     $(KNUMPOSSLOTBITS+1),%ebx
#;;   add.l     d0,a3                  # [193]
	movl     %eax,a3_l
	addl     %ebx,a3_l
#;;   moveq     #KNUMPOSSLOTBITS,d2    # [194]
	movl     $KNUMPOSSLOTBITS,d2_l
#;;   moveq     #1,d3                  # [195]
	movl     $1,d3_l
Check_Code_Bound_Loop2:
#;;   bsr.w     Check_Code_Bound2      # [196]
	call     Check_Code_Bound2
#;;   subq.l    #1,d2                  # [197 NE]
	subl     $1,d2_l
#;;   bne.b     Check_Code_Bound_Loop2 # [198]
	jne      Check_Code_Bound_Loop2
#;;   sub.w     #(1<<KNUMPOSSLOTBITS),d3 # [199]
	subw     $1<<KNUMPOSSLOTBITS,d3_w
#;;   cmp.w     #KSTARTPOSMODELINDEX,d3 # [200 MI]
	cmpw     $KSTARTPOSMODELINDEX,d3_w
#;;   bmi.b     Check_PosSlot_1        # [201]
	js       Check_PosSlot_1
#;;   move.l    d3,d1                  # [202]
	movl     d3_l,%edx
#;;   lsr.l     #1,d1                  # [203]
	shrl     $1,%edx
#;;   subq.l    #1,d1                  # [204]
	subl     $1,%edx
#;;   move.l    d3,d0                  # [205]
	movl     d3_l,%ebx
#;;   moveq     #1,d7                  # [206]
	movl     $1,d7_l
#;;   and.l     d7,d0                  # [207]
	andl     d7_l,%ebx
#;;   addq.l    #2,d0                  # [208]
	addl     $2,%ebx
#;;   move.l    d0,d7                  # [209]
	movl     %ebx,d7_l
#;;   cmp.w     #KENDPOSMODELINDEX,d3  # [210 CC]
	cmpw     $KENDPOSMODELINDEX,d3_w
#;;   bhs.b     Check_PosSlot_3        # [211]
	jnc      Check_PosSlot_3
#;;   lsl.l     d1,d0                  # [212]
	movl     %edx,%ecx
	shll     %cl,%ebx
#;;   move.l    d0,d7                  # [213]
	movl     %ebx,d7_l
#;;   lea       (SPECPOS*2)(a6),a3     # [214]
	lea      SPECPOS*2(%ebp),%eax
#;;   sub.l     d3,d0                  # [215]
	subl     d3_l,%ebx
#;;   subq.l    #1,d0                  # [216]
	subl     $1,%ebx
#;;   add.l     d0,d0                  # [217]
	addl     %ebx,%ebx
#;;   add.l     d0,a3                  # [218]
	movl     %eax,a3_l
	addl     %ebx,a3_l
#;;   bra.b     Check_PosSlot_4        # [219]
	jmp      Check_PosSlot_4
Check_PosSlot_3:
#;;   subq.l    #KNUMALIGNBITS,d1      # [220]
	subl     $KNUMALIGNBITS,%edx
Shift_Range_Loop:
#;;   move.l    (a5),d0                # [221]
	movl     a5_l,%ecx
	movl     (%ecx),%ebx
#;;   lsr.l     #1,d0                  # [222]
	shrl     $1,%ebx
#;;   add.l     d7,d7                  # [223]
	movl     d7_l,%eax
	addl     %eax,d7_l
#;;   move.l    d0,(a5)                # [224]
	movl     %ebx,(%ecx)
#;;   cmp.l     d5,d0                  # [225 HI]
	cmpl     d5_l,%ebx
#;;   bhi.b     Check_Code             # [226]
	ja       Check_Code
#;;   sub.l     d0,d5                  # [227]
	subl     %ebx,d5_l
#;;   addq.l    #1,d7                  # [228]
	addl     $1,d7_l
Check_Code:
#;;   bsr.b     Get_Code               # [229]
	call     Get_Code
#;;   subq.l    #1,d1                  # [230 NE]
	subl     $1,%edx
#;;   bne.b     Shift_Range_Loop       # [231]
	jne      Shift_Range_Loop
#;;   lea       (ALIGN*2)(a6),a3       # [232]
	lea      ALIGN*2(%ebp),%eax
#;;   lsl.l     #KNUMALIGNBITS,d7      # [233]
	shll     $KNUMALIGNBITS,d7_l
#;;   moveq     #KNUMALIGNBITS,d1      # [234]
	movl     $KNUMALIGNBITS,%edx
	movl     %eax,a3_l
Check_PosSlot_4:
#;;   moveq     #1,d2                  # [235]
	movl     $1,d2_l
#;;   moveq     #1,d3                  # [236]
	movl     $1,d3_l
Check_Code_Bound_Loop3:
#;;   bsr.b     Check_Code_Bound2      # [237 (EQ)]
	call     Check_Code_Bound2
#;;   beq.b     Check_Code_Bound_2     # [238]
	je       Check_Code_Bound_2
#;;   or.l      d2,d7                  # [239]
	movl     d2_l,%eax
	orl      %eax,d7_l
Check_Code_Bound_2:
#;;   add.l     d2,d2                  # [240]
	movl     d2_l,%eax
	addl     %eax,d2_l
#;;   subq.l    #1,d1                  # [241 NE]
	subl     $1,%edx
#;;   bne.b     Check_Code_Bound_Loop3 # [242]
	jne      Check_Code_Bound_Loop3
#;;   bra.b     Check_PosSlot_2        # [243]
	jmp      Check_PosSlot_2
Check_PosSlot_1:
#;;   move.l    d3,d7                  # [244]
	movl     d3_l,%eax
	movl     %eax,d7_l
Check_PosSlot_2:
#;;   addq.l    #1,d7                  # [245]
	addl     $1,d7_l
change_state_6:
#;;   addq.l    #KMATCHMINLEN,d6       # [246]
	addl     $KMATCHMINLEN,d6_l
Copy_Rem_Bytes:
#;;   bsr.b     store_prev_byte        # [247]
	call     store_prev_byte
#;;   subq.l    #1,d6                  # [248 NE]
	subl     $1,d6_l
#;;   bne.b     Copy_Rem_Bytes         # [249]
	jne      Copy_Rem_Bytes
cont:
#;;   move.l    (a7)+,d2               # [250]
	popl     d2_l
#;;   cmp.l     d2,a2                  # [251 MI]
	movl     d2_l,%eax
	cmpl     %eax,a2_l
#;;   bmi.w     depack_loop            # [252]
	js       depack_loop
#;;   rts                              # [253]
	ret      
store_prev_byte:
#;;   move.l    a2,d0                  # [254]
	movl     a2_l,%ebx
#;;   sub.l     d7,d0                  # [255]
	subl     d7_l,%ebx
#;;   move.b    (a4,d0.l),d0           # [256]
	movl     a4_l,%ecx
	movb     0(%ecx,%ebx),%bl
store_prev_byte2:
#;;   move.b    d0,(a4,a2.l)           # [257]
	movl     a2_l,%ecx
	addl     a4_l,%ecx
	movb     %bl,0(%ecx)
#;;   addq.l    #1,a2                  # [258]
	addl     $1,a2_l
#;;   rts                              # [259]
	ret      
Check_Fix_Range3:
#;;   move.l    d4,d0                  # [260]
	movl     d4_l,%ebx
Check_Fix_Range2:
#;;   add.l     d0,d0                  # [261]
	addl     %ebx,%ebx
#;;   add.l     d0,a1                  # [262]
	lea      (%edi,%ebx),%edi
Check_Fix_Range:
#;;   move.l    (a5),d0                # [263]
	movl     a5_l,%ecx
	movl     (%ecx),%ebx
#;;   lsr.l     #8,d0                  # [264]
	shrl     $8,%ebx
#;;   lsr.l     #3,d0                  # [265]
	shrl     $3,%ebx
#;;   move.l    d1,-(a7)               # [266]
	pushl    %edx
#;;   move.l    d0,d1                  # [267]
	movl     %ebx,%edx
#;;   swap      d1                     # [268]
	roll     $16,%edx
#;;   mulu.w    (a1),d0                # [269]
	movzwl   %bx,%ebx
	movzwl   (%edi),%eax
	imull    %eax,%ebx
#;;   mulu.w    (a1),d1                # [270]
	movzwl   %dx,%edx
	movzwl   (%edi),%eax
	imull    %eax,%edx
#;;   swap      d1                     # [271]
	roll     $16,%edx
#;;   add.l     d1,d0                  # [272]
	addl     %edx,%ebx
#;;   move.l    (a7)+,d1               # [273]
	popl     %edx
#;;   cmp.l     d5,d0                  # [274 LS]
	cmpl     d5_l,%ebx
#;;   bls.b     Range_Lower            # [275]
	jbe      Range_Lower
#;;   move.l    d0,(a5)                # [276]
	movl     %ebx,(%ecx)
#;;   move.w    #KBITMODELTOTAL,d0     # [277]
	movw     $KBITMODELTOTAL,%bx
#;;   sub.w     (a1),d0                # [278]
	subw     (%edi),%bx
#;;   lsr.w     #KNUMMOVEBITS,d0       # [279]
	shrw     $KNUMMOVEBITS,%bx
#;;   add.w     d0,(a1)                # [280]
	addw     %bx,(%edi)
Get_Code:
#;;   move.l    (a5),d0                # [281]
	movl     a5_l,%ecx
	movl     (%ecx),%ebx
#;;   cmp.l     #KTOPVALUE,d0          # [282 CC]
	cmpl     $KTOPVALUE,%ebx
#;;   bhs.b     top_range              # [283]
	jnc      top_range
#;;   lsl.l     #8,d0                  # [284]
	shll     $8,%ebx
#;;   move.l    d0,(a5)                # [285]
	movl     %ebx,(%ecx)
#;;   lsl.l     #8,d5                  # [286]
	shll     $8,d5_l
#;;   move.b    (a0)+,d5               # [287]
	lodsb    
	movb     %al,d5_b
top_range:
#;;   moveq     #0,d0                  # [288 EQ NE]
	movl     $0,%ebx
	testl    %ebx,%ebx
#;;   rts                              # [289]
	ret      
Check_Code_Bound2:
#;;   lea       (a3),a1                # [290]
	movl     a3_l,%edi
Check_Code_Bound:
#;;   add.l     d3,d3                  # [291]
	movl     d3_l,%eax
	addl     %eax,d3_l
#;;   lea       (a1,d3.l),a1           # [292]
	movl     d3_l,%ecx
	lea      0(%edi,%ecx),%edi
#;;   bsr.b     Check_Fix_Range        # [293 (EQ NE)]
	call     Check_Fix_Range
#;;   beq.b     Lower_Bound            # [294]
	je       Lower_Bound
#;;   addq.l    #1,d3                  # [295 EQ NE]
	addl     $1,d3_l
Lower_Bound:
#;;   rts                              # [296]
	ret      
Range_Lower:
#;;   sub.l     d0,(a5)                # [297]
	subl     %ebx,(%ecx)
#;;   sub.l     d0,d5                  # [298]
	subl     %ebx,d5_l
#;;   move.w    (a1),d0                # [299]
	movw     (%edi),%bx
#;;   lsr.w     #KNUMMOVEBITS,d0       # [300]
	shrw     $KNUMMOVEBITS,%bx
#;;   sub.w     d0,(a1)                # [301]
	subw     %bx,(%edi)
#;;   bsr.b     Get_Code               # [302]
	call     Get_Code
#;;   moveq     #1,d0                  # [303 EQ NE]
	movl     $1,%ebx
	testl    %ebx,%ebx
#;;   rts                              # [304]
	ret      
	.data
	.p2align	2
var:
#;;   range:                  dc.l    -1 # [306]
range:
	.long	-1
#;;   rep3:                   dc.l    1 # [307]
rep3:
	.long	1
#;;   rep2:                   dc.l    1 # [308]
rep2:
	.long	1
#;;   rep1:                   dc.l    1 # [309]
rep1:
	.long	1
