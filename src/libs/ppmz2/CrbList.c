#include "stdafx.h"
#include "CrbList.h"

__declspec(dllexport) LinkNode *	LN_CutTail(LinkNode *pList)
{
LinkNode * LN;
	assert(pList);
	LN = pList->Prev;
	if ( LN == pList ) return NULL;
	LN_Cut(LN);
return LN;
}