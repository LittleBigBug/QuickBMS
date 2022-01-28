#ifndef AZO_MATCHCODETABLE_H
#define AZO_MATCHCODETABLE_H

#include "AZOPrivate.h"

namespace AZO {

template <u_int N>
struct MatchLenExtraBit {
    static const u_int value = N < MATCH_LENGTH_SGAP ? 0 : (N-MATCH_LENGTH_SGAP) / MATCH_LENGTH_GAP;
};

template <u_int N>
struct MatchLenCode {
    static const u_int value = MatchLenCode<N-1>::value + (1 << MatchLenExtraBit<N-1>::value);
};

template <>
struct MatchLenCode<0> {
    static const u_int value = MATCH_MIN_LENGTH;
};

template <u_int N>
struct MatchDistExtraBit {
    static const u_int value = N < MATCH_DIST_SGAP ? 0 : (N-MATCH_DIST_SGAP) / MATCH_DIST_GAP;
};

template <u_int N>
struct MatchDistCode {
    static const u_int value = MatchDistCode<N-1>::value + (1u << MatchDistExtraBit<N-1>::value);
};

template <>
struct MatchDistCode<0> {
    static const u_int value = MATCH_MIN_DIST;
};



#define VALUE_TYPING \
    V(0),  V(1),  V(2),  V(3),  V(4),  V(5),  V(6),  V(7),  V(8),  V(9), \
    V(10), V(11), V(12), V(13), V(14), V(15), V(16), V(17), V(18), V(19), \
    V(20), V(21), V(22), V(23), V(24), V(25), V(26), V(27), V(28), V(29), \
    V(30), V(31), V(32), V(33), V(34), V(35), V(36), V(37), V(38), V(39), \
    V(40), V(41), V(42), V(43), V(44), V(45), V(46), V(47), V(48), V(49), \
    V(50), V(51), V(52), V(53), V(54), V(55), V(56), V(57), V(58), V(59), \
    V(60), V(61), V(62), V(63), V(64), V(65), V(66), V(67), V(68), V(69), \
    V(70), V(71), V(72), V(73), V(74), V(75), V(76), V(77), V(78), V(79), \
    V(80), V(81), V(82), V(83), V(84), V(85), V(86), V(87), V(88), V(89), \
    V(90), V(91), V(92), V(93), V(94), V(95), V(96), V(97), V(98), V(99), \
    V(100), V(101), V(102), V(103), V(104), V(105), V(106), V(107), V(108), V(109), \
    V(110), V(111), V(112), V(113), V(114), V(115), V(116), V(117), V(118), V(119), \
    V(120), V(121), V(122), V(123), V(124), V(125), V(126), V(127), /*V(128), V(129), \
    V(130), V(131), V(132), V(133), V(134), V(135), V(136), V(137), V(138), V(139), \
    V(140), V(141), V(142), V(143), V(144), V(145), V(146), V(147), V(148), V(149), \
    V(150), V(151), V(152), V(153), V(154), V(155), V(156), V(157), V(158), V(159), \
    V(160), V(161), V(162), V(163), V(164), V(165), V(166), V(167), V(168), V(169), \
    V(170), V(171), V(172), V(173), V(174), V(175), V(176), V(177), V(178), V(179), \
    V(180), V(181), V(182), V(183), V(184), V(185), V(186), V(187), V(188), V(189), \
    V(190), V(191), V(192), V(193), V(194), V(195), V(196), V(197), V(198), V(199), \
    V(200), V(201), V(202), V(203), V(204), V(205), V(206), V(207), V(208), V(209), \
    V(210), V(211), V(212), V(213), V(214), V(215), V(216), V(217), V(218), V(219), \
    V(220), V(221), V(222), V(223), V(224), V(225), V(226), V(227), V(228), V(229), \
    V(230), V(231), V(232), V(233), V(234), V(235), V(236), V(237), V(238), V(239), \
    V(240), V(241), V(242), V(243), V(244), V(245), V(246), V(247), V(248), V(249), \
    V(250), V(251), V(252), V(253), V(254), V(255)*/
    

#define V(N) MatchLenCode<N>::value
const u_int MATCH_LENGTH_CODE_TABLE[] = { VALUE_TYPING };
const u_int MATCH_MAX_LENGTH = V(MATCH_LENGTH_CODE_SIZE)-1;
#undef V

#define V(N) MatchLenExtraBit<N>::value
const u_int MATCH_LENGTH_EXTRABIT_TABLE[] = { VALUE_TYPING };
#undef V


#define V(N) MatchDistCode<N>::value
const u_int MATCH_DIST_CODE_TABLE[] = { VALUE_TYPING };
const u_int MATCH_MAX_DIST = V(MATCH_DIST_CODE_SIZE)-1;
#undef V

#define V(N) MatchDistExtraBit<N>::value
const u_int MATCH_DIST_EXTRABIT_TABLE[] = { VALUE_TYPING };
#undef V



template <u_int N, uint8_t CODE = 0>
struct MatchLenReverseCode {
	static const uint8_t value = N < MatchLenCode<CODE+1>::value ? CODE : MatchLenReverseCode<N, CODE+1>::value;
};

template <u_int N>
struct MatchLenReverseCode<N, MATCH_LENGTH_CODE_SIZE-1> {	
	static const uint8_t value = MATCH_LENGTH_CODE_SIZE-1;
};

template <u_int N, uint8_t CODE = 0>
struct MatchDistReverseCode {
	static const uint8_t value = N < MatchDistCode<CODE+1>::value ? CODE : MatchDistReverseCode<N, CODE+1>::value;
};

template <u_int N>
struct MatchDistReverseCode<N, MATCH_DIST_CODE_SIZE> {
	static const uint8_t value = MATCH_DIST_CODE_SIZE-1;
};


inline uint8_t GetMatchLengthCode_Slow(u_int value)
{
    ASSERT(MATCH_MIN_LENGTH <= value && value <= MATCH_MAX_LENGTH);

    for(uint8_t i=0; i<MATCH_LENGTH_CODE_SIZE-1; ++i) {
        if(value < MATCH_LENGTH_CODE_TABLE[i+1]) {
            return i;
        }
    }

    return MATCH_LENGTH_CODE_SIZE-1;
}

inline uint8_t GetMatchDistCode_Slow(u_int value)
{
    ASSERT(MATCH_MIN_DIST <= value && value <= MATCH_MAX_DIST);

    for(uint8_t i=0; i<MATCH_DIST_CODE_SIZE-1; ++i) {
        if(value < MATCH_DIST_CODE_TABLE[i+1]) {
            return i;
        }
    }

    return MATCH_DIST_CODE_SIZE-1;   
}

inline u_int Log2(u_int num)
{
	u_int log = 0;

	for (; num > 1; ++log)
		num >>= 1;

	return log;
}

inline uint8_t GetMatchLengthCode(u_int value)
{
    ASSERT(MATCH_MIN_LENGTH <= value && value <= MATCH_MAX_LENGTH);

	value -= MATCH_MIN_LENGTH;

	if(value < MATCH_LENGTH_SGAP)
		return static_cast<uint8_t>(value);

	value -= MATCH_LENGTH_SGAP;

    const u_int one(1);
	const u_int extraBit = Log2(value / MATCH_LENGTH_GAP + 1);
	const u_int ret = MATCH_LENGTH_SGAP + extraBit*MATCH_LENGTH_GAP
						+ (value - ((one<<extraBit)-1)*MATCH_LENGTH_GAP) / (one<<extraBit);

	return static_cast<uint8_t>(ret);
}

inline uint8_t GetMatchDistCode(u_int value)
{
    ASSERT(MATCH_MIN_DIST <= value && value <= MATCH_MAX_DIST);

	value -= MATCH_MIN_DIST;

	if(value < MATCH_DIST_SGAP)
		return static_cast<uint8_t>(value);

	value -= MATCH_DIST_SGAP;

    const u_int one(1);
	const u_int extraBit = Log2(value / MATCH_DIST_GAP + 1);
	const u_int ret = MATCH_DIST_SGAP + extraBit*MATCH_DIST_GAP
						+ (value - ((one<<extraBit)-1)*MATCH_DIST_GAP) / (one<<extraBit);

	return static_cast<uint8_t>(ret);
}

} //namespaces AZO

#endif /*AZO_MATCHCODETABLE_H*/
