/* Copyright (c) Microsoft Corporation. All rights reserved. */

// GrowBuf.cpp
//
// Implements TGrowBuf -- see "GrowBuf.h" for more information.
//
// Also implements GenericGrowBuf, a helper class used by TGrowBuf.
//

#include <string.h>
//#include <windows.h>
#include <stdlib.h>
//#include "stdafx.h"
#include "GrowBuf.h"

typedef unsigned char   BYTE;
typedef int             BOOL;
typedef int             HRESULT;
#define TRUE            1
#define FALSE           0
#define S_OK ((HRESULT)0x00000000L)
#define E_OUTOFMEMORY ((HRESULT)0x80000002L)
#define E_FAIL ((HRESULT)0x80000008L)
#define ZeroMemory(MEM,SIZE)      memset(MEM,0,SIZE)



// fOK = SetSize(cbNew)
//
// Set the size of the buffer to be <cbNew> (which may result in truncating
// data from the end of the buffer).  This operation will cause the buffer
// address to change (unless the old and new buffer sizes are equal).
// If <cbNew> is zero, the buffer is freed.
//
// Return true on success, false on failure (out of memory).
//
bool GenericGrowBuf::SetSize(unsigned cbNew)
{
	// optimization: if the buffer is already the desired size, we're done
	if (cbNew == (unsigned) (m_pbMax - m_pbBuf))
		return true;

	// allocate a new buffer <pbNew> (unless the new buffer size is zero)
	char *pbNew = NULL;
	if (cbNew > 0)
	{
		if ((pbNew = new char[cbNew]) == NULL)
			return false; // out of memory
	}

	// set <cbCopy> to be the number of bytes to copy from the old buffer
	// to the new buffer
	unsigned cbCopy = m_pbEnd - m_pbBuf; // no. items in old buffer
	if (cbCopy > cbNew) // limit to no. items in new buffer
		cbCopy = cbNew;

	// copy <cbCopy> bytes from the old buffer to the new buffer
	if (cbCopy > 0)
		memcpy(pbNew, m_pbBuf, cbCopy);

	// delete the old buffer (if any)
	if (m_pbBuf != NULL)
		delete [] m_pbBuf;

	// initialize member state of the new buffer
	m_pbBuf = pbNew;
	m_pbEnd = pbNew + cbCopy;
	m_pbMax = pbNew + cbNew;

	return true;
}


// fOK = Grow(cbItem, cNewItems)
//
// Grow the buffer so that there's enough space for <cNewItems> new items
// beyond <m_pbEnd>.  (Note that this doesn't grow the buffer by <cNewItems>
// items, since there may be some space still available at the end of the
// buffer.)  <cbItem> is the size of an item (in bytes).
//
// If the buffer needs to be grown, it is grown by <m_cbGrowBy> bytes at a
// time.  If <m_cbGrowBy> is zero (i.e. it hasn't been initialized yet),
// then <m_cbGrowBy> is set to the size of <cNewItems> items.
//
// Return true on success, false on failure (out of memory).
//
bool GenericGrowBuf::Grow(unsigned cbItem, unsigned cNewItems)
{
	// set <cbNewItems> to the size of the desired new items
	unsigned cbNewItems = cNewItems * cbItem;

	// set <m_cbGrow> if it hasn't been set yet
	if (m_cbGrowBy == 0)
		m_cbGrowBy = cbNewItems;

	// set <cbBuf> to the current buffer size (in bytes)
	unsigned cbBuf = m_pbMax - m_pbBuf;

	// set <cbWanted> to the desired buffer size (in bytes)
	unsigned cbWanted = (m_pbEnd - m_pbBuf) + cbNewItems;

	while (cbBuf < cbWanted)
	{
		// set iMult to the current buffer size, as a multiple of m_cbGrowBy,
		// rounded up
		int iMult = (cbBuf + m_cbGrowBy) / m_cbGrowBy;

		// increase iMult by approximately (and at least) 50%
		if (iMult == 1)
			iMult++; // force iMult to increase
		else
			iMult = (iMult * 3) / 2;

		// calculate new buffer size
		cbBuf = iMult * m_cbGrowBy;
	}

	// allocate the new buffer -- does nothing if the buffer size hasn't
	// changed
	return SetSize(cbBuf);
}


// pv = Alloc(cbItem, cItems)
//
// Allocate space after the used portion of the buffer for <cItems> items,
// and return a pointer to that space.  Advance the end-of-buffer pointer
// past the allocated space.
//
// <cbItem> is the size of an item (in bytes).
//
void *GenericGrowBuf::Alloc(unsigned cbItem, unsigned cItems)
{
	char *pbEndNew = m_pbEnd + cItems * cbItem;
	if (pbEndNew > m_pbMax)
	{
		if (!Grow(cbItem, cItems))
			return NULL;
		return Alloc(cbItem, cItems); // note that Grow() may have moved buffer
	}
	void *pvAlloc = m_pbEnd;
	m_pbEnd = pbEndNew;
	return pvAlloc;
}


// DeleteItems(cbItem, iItem, cItems)
//
// Delete <cItems> items, starting with item number <iItem>, from the buffer.
// <cbItem> is the size of an item (in bytes).
//
// No error checking is done on any argument.
//
void GenericGrowBuf::DeleteItems(unsigned cbItem, unsigned iItem,
	unsigned cItems)
{
	char *pbDelStart = m_pbBuf + iItem * cbItem;
	char *pbDelEnd = pbDelStart + cItems * cbItem;
	unsigned cbMove = m_pbEnd - pbDelEnd;
	if ((pbDelStart != pbDelEnd) && (cbMove > 0))
		memmove(pbDelStart, pbDelEnd, cbMove);
	m_pbEnd -= cItems * cbItem;
}

