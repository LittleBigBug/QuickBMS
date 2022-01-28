#pragma once

namespace Ppmz2
{
    class Exclude
    {
    public:
        typedef unsigned char ExcludeType;
        static const ExcludeType _resetValue = ((ExcludeType)(~((ExcludeType)0)));

        unsigned int _numChars;
	    unsigned int _counter;
	    unsigned int _tablePaddedLen;
	    ExcludeType* _table;
	    bool _anySet;

        Exclude(unsigned int numChars)
        {
	        _tablePaddedLen = sizeof(ExcludeType)*numChars;
	        _tablePaddedLen = (_tablePaddedLen + 31)&(~31);
        	
	        _table = new ExcludeType[_tablePaddedLen];        	
	        _numChars = numChars;

	        Reset();
        }

        ~Exclude()
        {
            delete[] _table;
        }

        void Reset()
        {     
            for (unsigned int i = 0; i < _tablePaddedLen; ++i)
                _table[i] = 0;
	        _counter = 1;
	        _anySet = false;
        }

        void Clear()
        {
            _counter++;
	        _anySet = false;
	        if ( _counter == _resetValue )
		        Reset();
        }

        bool IsEmpty()
        {
            return ! _anySet;
        }

        void Set(int sym)
        {
            _table[sym] = _counter;
	        _anySet = true;
        }

        bool IsExcluded(int sym)
        {
            return _table[sym] == _counter;
        }
    };
}