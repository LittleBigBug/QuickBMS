/* Copyright (c) Microsoft Corporation. All rights reserved. */

// GrowBuf.h
//
// Declares TGrowBuf.
//
// Also declares GenericGrowBuf, a helper class used by TGrowBuf.
//

#pragma once

// GenericGrowBuf
//
// A helper class used by TGrowBuf, used to reduce code size (specifically
// when multiple template instantiations of TGrowBuf are used).
//
// GenericGrowBuf contains the same member variables as TGrowBuf, but pointers
// are expressed as byte pointers.
//
struct GenericGrowBuf
{
// class state
protected:
	// WARNING: members must match order of those in TGrowBuf!
	char *			m_pbBuf;		// start of buffer
	char *			m_pbEnd;		// item past last used item
	char *			m_pbMax;		// item past last item in buffer
	unsigned		m_cbGrowBy;		// no. bytes to grow buffer by (as needed)
	// WARNING: members must match order of those in TGrowBuf!

// operations
public:
	bool SetSize(unsigned cbNew);
	bool Grow(unsigned cbItem, unsigned cNewItems);
	void *Alloc(unsigned cbItem, unsigned cItems);
	void DeleteItems(unsigned cbItem, unsigned iItem, unsigned cItems);
};


// TGrowBuf<T>
//
// Maintains a buffer of items of class T.  Before writing into the buffer,
// call Need(N) to set the initial buffer size to N.  (If the buffer
// subsequently runs out of space, it is grown by N items at a time as needed.)
// Alternatively, use the TGrowBuf(cGrowBy) constructor to specify how big
// the buffer should be the first time it's written to.
//
// The buffer consists of two parts:
//
//     +---------------------------------------+
//     | used items            | unused items  |
//     +---------------------------------------+
//     ^Buf()                  ^End()          ^Max()
//
// Buf() returns a pointer to the the start of the buffer.  End() returns a
// pointer to the item past the end of used (valid) items in the buffer.
// Max() returns a pointer to the item past the end of the allocated space.
// Typical usage: write items into End(), incrementing as you go, and let
// TGrowBuf grow the buffer as needed.
//
// Whenever you write item(s) to the address returned by End(), you should
// call Need(N), where N is the number of items you plan to write.  If you
// only write one item at a time, you can call Next(), which returns End()
// and then increments it (growing the buffer as needed).  Call SetEnd()
// to move the End() pointer to a new location within the buffer -- note that
// no validation is done to ensure that the parameter to SetEnd() is valid.
//
// SetSize(N) sets the buffer size to N items (trucating it if needed).
//
template <class T> class TGrowBuf
{
// class state
protected:
	// WARNING: members must match order of those in GenericGrowBuf!
	T *				m_pBuf;			// start of buffer
	T *				m_pEnd;			// item past last used item
	T *				m_pMax;			// item past last item in buffer
	unsigned		m_cbGrowBy;		// no. bytes to grow buffer by (as needed)
	// WARNING: members must match order of those in GenericGrowBuf!

// construction/destruction
public:
	inline TGrowBuf();
	inline TGrowBuf(unsigned cGrowBy);
	inline ~TGrowBuf();

// operations
public:
	inline bool Need(unsigned cItems);
	inline T * Next();
	inline T * Alloc(unsigned cItems);
	inline bool WriteOne(T item);
	inline T * Write(const T *pItems, unsigned cItems);
	inline T * Buf() const { return m_pBuf; }
	inline T * End() const { return m_pEnd; }
	inline T * Max() const { return m_pMax; }
	inline void SetEnd(T *pEnd) { m_pEnd = pEnd; }
	inline void Skip(unsigned cItems) { m_pEnd += cItems; }
	inline void Reset() { m_pEnd = m_pBuf; }
	inline bool IsEmpty() { return (m_pEnd == m_pBuf); }
	inline unsigned Count() const;
    inline unsigned Size() const;
	inline bool SetSize(unsigned cItems);
	inline void DeleteItems(unsigned iItem, unsigned cItems);
	inline unsigned ItemSize() { return sizeof(T); }
	inline T * Item(unsigned iItem) { return m_pBuf + iItem; }
};


// TGrowBuf()
//
// Constructor.
//
template <class T> TGrowBuf<T>::TGrowBuf()
{
	memset(this, 0, sizeof(*this));
}


// TGrowBuf(cGrowBy)
//
// Constructor.  <cGrowBy> specifies how many elements to grow the buffer
// by whenever it runs out of space (including the first time the buffer
// is written to).
//
template <class T> TGrowBuf<T>::TGrowBuf(unsigned cGrowBy)
{
	memset(this, 0, sizeof(*this));
	m_cbGrowBy = cGrowBy * sizeof(T);
}


// ~TGrowBuf()
//
// Destructor.
//
template <class T> TGrowBuf<T>::~TGrowBuf()
{
	((GenericGrowBuf *) this)->SetSize(0);
}


// fOK = Need(cItems)
//
// Grow the buffer as needed so that there is space for at least <cItems>
// beyond the end of used portion of the buffer.
//
// Return true on success, false on failure (out of memory).
//
template <class T> bool TGrowBuf<T>::Need(unsigned cItems)
{
	if (m_pEnd + cItems > m_pMax)
		return ((GenericGrowBuf *) this)->Grow(sizeof(T), cItems);
	return true;
}


// pNext = Next()
//
// Returns a pointer to the first unused item (at End()) and increments
// the number of items.  Return NULL on out-of-memory.
//
template <class T> T * TGrowBuf<T>::Next()
{
	if (m_pEnd >= m_pMax)
	{
		if (!((GenericGrowBuf *) this)->Grow(sizeof(T), 1))
			return 0;
	}
	return m_pEnd++;
}


// pAlloc = Alloc(cItems)
//
// Allocate space after the used portion of te buffer for <cItems> items,
// and return a pointer to that space.  SetEnd() is called automatically.
//
template <class T> T * TGrowBuf<T>::Alloc(unsigned cItems)
{
	return (T *) ((GenericGrowBuf *) this)->Alloc(sizeof(T), cItems);
}


// fOK = WriteOne(item)
//
// Write <item> to the end of the buffer.
//
// Return true on success, false on failure (out of memory).
//
template <class T> bool TGrowBuf<T>::WriteOne(T item)
{
	if (m_pEnd >= m_pMax)
	{
		if (!((GenericGrowBuf *) this)->Grow(sizeof(T), 1))
			return false;
	}
	*m_pEnd++ = item;
	return true;
}


// pWritten = Write(pItems, cItems)
//
// Write <cItems> items of type T, from <pItems>, to the end of the buffer,
// and return a pointer to the written-to space.  SetEnd() is called
// automatically.  Return NULL on error.
//
template <class T> T * TGrowBuf<T>::Write(const T *pItems, unsigned cItems)
{
	T *pT = (T *) ((GenericGrowBuf *) this)->Alloc(sizeof(T), cItems);
	if (pT != NULL)
		memcpy(pT, pItems, sizeof(T) * cItems);
	return pT;
}


// cItems = Count()
//
// Counts the number of items in the buffer (between Buf() and End())
//
template <class T> unsigned TGrowBuf<T>::Count() const
{
	return m_pEnd - m_pBuf;
}


// cItems = Size()
//
// Returns the size of the buffer
//
template <class T> unsigned TGrowBuf<T>::Size() const
{
    return m_pMax - m_pBuf;
}

// fOK = SetSize(cItems)
//
// Set the size of the buffer to be <cItems> items (which will result in
// truncating items from the end of the buffer if <cItems> is less than the
// current buffer size).  This operation will cause the buffer address to
// change (unless the old and new buffer sizes are equal).
// If <cItems> is zero, the buffer is freed.
//
// Return true on success, false on failure (out of memory).
//
template <class T> bool TGrowBuf<T>::SetSize(unsigned cItems)
{
	return ((GenericGrowBuf *) this)->SetSize(cItems * sizeof(T));
}


// DeleteItems(iItem, cItems)
//
// Delete <cItems> items from the buffer, starting at item number <iItem>.
// No error-checking is done on either argument.
// 
template <class T> void TGrowBuf<T>::DeleteItems(unsigned iItem,
	unsigned cItems)
{
	((GenericGrowBuf *) this)->DeleteItems(sizeof(T), iItem, cItems);
}


