# Notes by Luigi Auriemma
# PortAsm did a very good job, the only problem is that there is no switch
# to convert the endianess of the data (68k is big, x86 is little) so you
# need to do this operation by hand on the input buffer... I did it only
# for the unsquash algorithm!
# All the algorithms except unsquash have been NOT tested and not 100%
# verified (some arguments may have been not set... it happens).
# Note that PortAsm doesn't set the return value (%eax), I setted it
# manually only in some algorithms.
#
# Sources:
# http://www.amiga-stuff.com/crunchers-download.html
# http://aminet.net/package/util/libs/ulib4271

# for Mingw64
.code32

#########################################################################
#                                                                       #
#       PortAsm Code Translator Copyright (c) MicroAPL 1990-2000        #
#                   All Rights Reserved Worldwide                       #
#                                                                       #
#########################################################################
# Translated on Thu Mar  6 10:28:42 2014 by version 1.0.0 of PortAsm translator
# Demonstration version of PortAsm - for evaluation only
# Target assembler:    Gnu 'gas'
# Target runtime:      Win32


	.data
	#.global	memory_mapped_registers
memory_mapped_registers:
	#.global	d0_l
	#.global	d0_w
	#.global	d0_b
d0_l:
d0_w:
d0_b:
	.long	0
	#.global	d1_l
	#.global	d1_w
	#.global	d1_b
d1_l:
d1_w:
d1_b:
	.long	0
	#.global	d2_l
	#.global	d2_w
	#.global	d2_b
d2_l:
d2_w:
d2_b:
	.long	0
	#.global	d3_l
	#.global	d3_w
	#.global	d3_b
d3_l:
d3_w:
d3_b:
	.long	0
	#.global	d4_l
	#.global	d4_w
	#.global	d4_b
d4_l:
d4_w:
d4_b:
	.long	0
	#.global	d5_l
	#.global	d5_w
	#.global	d5_b
d5_l:
d5_w:
d5_b:
	.long	0
	#.global	d6_l
	#.global	d6_w
	#.global	d6_b
d6_l:
d6_w:
d6_b:
	.long	0
	#.global	d7_l
	#.global	d7_w
	#.global	d7_b
d7_l:
d7_w:
d7_b:
	.long	0
	#.global	a0_l
	#.global	a0_w
	#.global	a0_b
a0_l:
a0_w:
a0_b:
	.long	0
	#.global	a1_l
	#.global	a1_w
	#.global	a1_b
a1_l:
a1_w:
a1_b:
	.long	0
	#.global	a2_l
	#.global	a2_w
	#.global	a2_b
a2_l:
a2_w:
a2_b:
	.long	0
	#.global	a3_l
	#.global	a3_w
	#.global	a3_b
a3_l:
a3_w:
a3_b:
	.long	0
	#.global	a4_l
	#.global	a4_w
	#.global	a4_b
a4_l:
a4_w:
a4_b:
	.long	0
	#.global	a5_l
	#.global	a5_w
	#.global	a5_b
a5_l:
a5_w:
a5_b:
	.long	0
	#.global	xflag
xflag:
	.long	0
	#.global	_PA_Scratch0
_PA_Scratch0:
	.long	0


    #.global	UI_CruncherName
	#.global	UI_DecrunchAdr
	#.global	UI_DecrunchLen
	#.global	UI_Address
	#.global	UI_JumpAdr
	#.global	UI_ErrorNum
	#.global	UI_CrunchNum
	#.global	UI_CrunchType
    #.global UI_CrunchAdrTemp
    #.global UI_CrunchLenTemp
    #.global UI_Temp
    UI_CruncherName:
	UI_DecrunchAdr:
	UI_DecrunchLen:
	UI_Address:
	UI_JumpAdr:
	UI_ErrorNum:
	UI_CrunchNum:
	UI_CrunchType:
    UI_CrunchAdrTemp:
    UI_CrunchLenTemp:
    UI_Temp:
        .long 0

    #.global	Source
	#.global	Dest
	#.global	Temp
    Source:
	Dest:
	Temp:
        .long 0

    #.global UI_PasswordStr
    UI_PasswordStr:
        .ascii ""


# -Ilibs/amiga may not work and .include avoids issues with --export-all-symbols
.include "libs/amiga/amos_unsquash.s"
.include "libs/amiga/bytekiller.s"
.include "libs/amiga/bytekiller2.s"
.include "libs/amiga/bytekiller3.s"
.include "libs/amiga/crunchmania17b.s"
.include "libs/amiga/crunchmaniax.s"
.include "libs/amiga/crunchmaster.s"
.include "libs/amiga/crunchomatic.s"
.include "libs/amiga/discovery.s"
.include "libs/amiga/dms.s"
.include "libs/amiga/flashspeed.s"
.include "libs/amiga/iampacker.s"
.include "libs/amiga/isc.s"
.include "libs/amiga/lightpack.s"
.include "libs/amiga/mastercruncher.s"
.include "libs/amiga/maxpacker.s"
.include "libs/amiga/megacruncher.s"
.include "libs/amiga/p-compress.s"
.include "libs/amiga/packfire.s"
.include "libs/amiga/packit.s"
.include "libs/amiga/phd.s"
.include "libs/amiga/powerpacker.s"
.include "libs/amiga/spikecruncher.s"
.include "libs/amiga/stonecracker2.s"
.include "libs/amiga/stonecracker3.s"
.include "libs/amiga/stonecracker4.s"
.include "libs/amiga/tetrapack.s"
.include "libs/amiga/timedecrunch.s"
.include "libs/amiga/tryit.s"
.include "libs/amiga/tuc.s"
.include "libs/amiga/turbosqueezer.s"
.include "libs/amiga/turtlesmasher.s"
