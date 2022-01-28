#pragma once
#define __declspec(X)

#define DO_LOE_PENALIZE_NONDET // yep this helps; it's freaky

namespace Ppmz2
{
	// Forward defs to reduce header file dependencies
	class Context;
	class Exclude;
	class See;

    class LocalOrderEstimation
    {
    public:

        typedef enum
        {
	        LOETYPE_NONE = 0,
	        LOETYPE_MPS,
	        LOETYPE_ND1,
	        LOETYPE_COUNT
        } LOEType;

        __declspec(dllexport) static int ChooseOrder(LOEType loeType, Context ** contexts, unsigned long cntx, int maxOrder, Exclude* exc, See* see, bool useFull);		
    };
}