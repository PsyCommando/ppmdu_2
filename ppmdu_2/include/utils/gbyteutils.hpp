#ifndef G_BYTE_UTILS_H
#define G_BYTE_UTILS_H
/*
gbyteutils.h
18/05/2014
psycommando@gmail.com

Description:
A bunch of simple tools for doing common tasks when manipulating bytes.
*/

//!#TODO: this needs a serious cleanup!!
#include <vector>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <cassert>
#include <stdexcept>
#include <cmath>

namespace utils 
{

    typedef uint8_t byte;

    /*********************************************************************************************
        conditional_value
            If _BoolExpr is true,  _OPTION_A will be contained in value. 
            If its false, it will be _OPTION_B instead.
    *********************************************************************************************/
    template< bool _BoolExpr, class T, T _OPTION_A, T _OPTION_B>
        struct conditional_value
    {
        static const T value = (_BoolExpr)? _OPTION_A : _OPTION_B;
    };


    /*********************************************************************************************
        do_exponent_of_2_
            Computes 2^n, where n is _Exponent, and store it into its static "value" member.
            Very handy for computing bitmasks and such at compile time.
    *********************************************************************************************/
    template<unsigned long long _Exponent> struct do_exponent_of_2_
    {
        static const unsigned long long value = do_exponent_of_2_< (_Exponent - 1u) >::value * 2;
    };

    template<> struct do_exponent_of_2_<0>
    {
        static const unsigned long long value = 1;
    };

    /*********************************************************************************************
        WriteIntToBytes
            Tool to write integer values into a byte vector!
            Returns the new pos of the iterator after the operation.
    *********************************************************************************************/
    template<class T, class _outit>
        inline _outit WriteIntToBytes( T val, _outit itout, bool basLittleEndian = true )
    {
        static_assert( std::numeric_limits<T>::is_integer, "WriteIntToBytes() : Type T is not an integer!" );

        ////#FIXME: Why is this even necessary?
        //auto lambdaShiftAssign = [&val]( unsigned int shiftamt )->uint8_t
        //{
        //    T tempshift = 0;
        //    tempshift = ( val >> (shiftamt * 8) ) & 0xFF;
        //    return tempshift & 0xFF;
        //};

        if( basLittleEndian )
        {
            for( unsigned int i = 0; i < sizeof(T); ++i, ++itout )
            {
                T tempshift = 0;
                tempshift = ( val >> (i * 8) ) & 0xFF;
                (*itout) = tempshift & 0xFF;
            }
        }
        else
        {
            for( int i = (sizeof(T)-1); i >= 0 ; --i, ++itout )
            {
                T tempshift = 0;
                tempshift = ( val >> (i * 8) ) & 0xFF;
                (*itout) = tempshift & 0xFF;
            }
        }

        return itout;
    }

    /*********************************************************************************************
        WriteIntToBytes
            Alias for WriteIntToBytes, while we transition.
        
        #TODO: finish renaming everything !
    *********************************************************************************************/
    //template<class T, class _outit>
    //    inline _outit WriteIntToBytes( T val, _outit itout, bool basLittleEndian = true )
    //{
    //    return WriteIntToBytes( val, itout, basLittleEndian );
    //}

    /*********************************************************************************************
        ReadIntFromBytes
            Tool to read integer values from a byte vector!
            ** The iterator's passed as input, has its position changed !!
    *********************************************************************************************/
    //template<class T, class _init> 
    //    inline T ReadIntFromBytes( _init & itin, bool basLittleEndian = true ) //#TODO : Need to make sure that the iterator is really incremented!
    //{
    //    static_assert( std::numeric_limits<T>::is_integer, "ReadIntFromBytes() : Type T is not an integer!" );
    //    T out_val = 0;

    //    auto lambdaShiftBitOr = [&itin, &out_val]( unsigned int shiftamt )
    //    {
    //        T tmp = (*itin);
    //        out_val |= ( tmp << (shiftamt * 8) ) & ( 0xFF << (shiftamt*8) );
    //    };

    //    if( basLittleEndian )
    //    {
    //        for( unsigned int i = 0; i < sizeof(T); ++i, ++itin )
    //            lambdaShiftBitOr(i);
    //    }
    //    else
    //    {
    //        for( int i = (sizeof(T)-1); i >= 0; --i, ++itin )
    //            lambdaShiftBitOr(i);
    //    }

    //    return out_val;
    //}

    /*********************************************************************************************
        ReadIntFromBytes
            Tool to read integer values from a byte container!
            
            #NOTE :The iterator is passed by copy here !! And the incremented iterator is returned!
    *********************************************************************************************/
    //template<class T, class _init> 
    //    inline _init ReadIntFromBytes( T & dest, _init itin, bool basLittleEndian = true ) //#TODO : Need to make sure that the iterator is really incremented!
    //{
    //    dest = ReadIntFromBytes<typename T>( itin, basLittleEndian );
    //    return itin;
    //}

    template<class T, class _init> inline T ReadIntFromBytes(_init && itin, _init && itend, bool basLittleEndian = true)
    {
        _init _it(itin);
        _init _ite(itend);
        return ReadIntFromBytes<T, _init>(_it, _ite, basLittleEndian);
    }

    /*********************************************************************************************
        ReadIntFromBytes
            Tool to read integer values from a byte vector!
            ** The iterator's passed as input, has its position changed !!
    *********************************************************************************************/
    template<class T, class _init> 
        inline T ReadIntFromBytes( _init & itin, _init itend, bool basLittleEndian = true )
    {
        static_assert( std::numeric_limits<T>::is_integer, "ReadIntFromBytes() : Type T is not an integer!" );
        T out_val = 0;

        if( basLittleEndian )
        {
            unsigned int i = 0;
            for( ; (itin != itend) && (i < sizeof(T)); ++i, ++itin )
            {
                T tmp = (*itin);
                out_val |= ( tmp << (i * 8) ) & ( 0xFF << (i*8) );
            }

            if( i != sizeof(T) )
            {
#ifdef _DEBUG
                assert(false);
#endif
                throw std::runtime_error( "ReadIntFromBytes(): Not enough bytes to read from the source container!" );
            }
        }
        else
        {
            int i = (sizeof(T)-1);
            for( ; (itin != itend) && (i >= 0); --i, ++itin )
            {
                T tmp = (*itin);
                out_val |= ( tmp << (i * 8) ) & ( 0xFF << (i*8) );
            }

            if( i != -1 )
            {
#ifdef _DEBUG
                assert(false);
#endif
                throw std::runtime_error( "ReadIntFromBytes(): Not enough bytes to read from the source container!" );
            }
        }
        return out_val;
    }

    /*********************************************************************************************
        ReadIntFromBytes
            Tool to read integer values from a byte container!
            
            #NOTE :The iterator is passed by copy here !! And the incremented iterator is returned!
    *********************************************************************************************/
    template<class T, class _init> 
        inline _init ReadIntFromBytes( T & dest, _init itin, _init itend, bool basLittleEndian = true ) 
    {
        dest = ReadIntFromBytes<T,_init>( itin, itend, basLittleEndian );
        return itin;
    }

    /*********************************************************************************************
        ChangeValueOfASingleByte
            Allows to change the value of a single byte in a larger type! 
            As if it was an array.
    *********************************************************************************************/
    //template< class T >
    //    inline T ChangeValueOfASingleByte( T containinginteger, uint8_t newvalue, uint32_t byteoffset  )
    //{
    //    static_assert( std::is_pod<T>::value, "ChangeValueOfASingleByte(): Tried to change a byte on a non-POD type!!" );
    //    T mask = 0xFFu,
    //      val  = newvalue;

    //    mask = (mask << (sizeof(T) - byteoffset) * 8); //Am I over cautious ?
    //    val  = (val  << (sizeof(T) - byteoffset) * 8);

    //    return ( (val & mask) | containinginteger );
    //}

    /*********************************************************************************************
        ChangeValueOfASingleBit
    *********************************************************************************************/
    //template< class T >
    //    inline T ChangeValueOfASingleBit( T containinginteger, uint8_t newvalue, uint32_t offsetrighttoleft  )
    //{
    //    static_assert( std::is_pod<T>::value, "ChangeValueOfASingleBit(): Tried to change a bit on a non-POD type!!" );
    //    if( offsetrighttoleft >= (sizeof(T) * 8) )
    //        throw std::overflow_error("ChangeValueOfASingleBit(): Offset too big for integer type specified!");

    //    //Clean the bit then OR the value!
    //    return ( (0x01u & newvalue) << offsetrighttoleft ) | (containinginteger & ( ~(0x01u << offsetrighttoleft) ) );
    //}

    /*********************************************************************************************

        * offsetrighttoleft : offset of the bit to isolate from right to left.
    *********************************************************************************************/
    template< class T >
        inline T GetBit( T containinginteger, uint32_t offsetrighttoleft  )
    {
        static_assert( std::is_scalar<T>::value, "GetBit(): Tried to get a bit on a non-scalar type!!" );
        if( offsetrighttoleft >= (sizeof(T) * 8) )
            throw std::overflow_error("GetBit(): Offset too big for integer type specified!");

        //Isolate, then shift back to position 1 !
        return (containinginteger & ( (0x01u << offsetrighttoleft) ) ) >> offsetrighttoleft;
    }

    /*********************************************************************************************
        IsolateBits
            Isolate some adjacent bits inside a value of type T, 
            and shift them back to offset 0. 

            * nbbits    : Nb of bits to isolate. Used to make the bitmask.
            * bitoffset : Offset of the bits from the right to the left. From the last bit. 

            Ex1 : we want those bits 0000 1110, the params are ( 0xE, 3, 1 ),  returns 0000 0111
            Ex2 : we want those bits 0011 0000, the params are ( 0x30, 2, 4 ), returns 0000 0011
    *********************************************************************************************/
    template<class T>
        inline T IsolateBits( T src, unsigned int nbBits, unsigned int bitoffset )
    {
        static_assert( std::is_scalar<T>::value, "IsolateBits(): Tried to isolate bits of a non-scalar type!!" );
        T mask = static_cast<T>( ( pow( 2, nbBits ) - 1u ) ); //Subtact one to get the correct mask
        return ( ( src & (mask << bitoffset) ) >> bitoffset );
    }

    /*********************************************************************************************
        Helper method to get whether a certain bit's state as a boolean, 
        instead of only isolating it.
    *********************************************************************************************/
    template< class T >
        inline bool IsBitOn( T containinginteger, uint32_t offsetrighttoleft  ) 
    { 
        return GetBit( containinginteger, offsetrighttoleft ) > 0; 
    }

    /*
        WriteStrToByteContainer
            Write a c string to a byte container, via iterator.
            strl is the length of the string!
    */
    template<class _outit> 
        _outit WriteStrToByteContainer( _outit itwhere, const char * str, size_t strl )
    {
        //#FIXME: The static assert below is broken with non-backinsert iterators
        //static_assert( typename std::is_same<typename _init::container_type::value_type, uint8_t>::type::value, "WriteStrToByteContainer: Target container's value_type can't be assigned bytes!" );
        
        //#FIXME: Highly stupid... If we cast the values inside the string to the target type, that means we could easily
        //        go out of bound if the target container isn't containing bytes..
        //return std::copy_n( reinterpret_cast<const typename _outit::container_type::value_type*>(str), strl,  itwhere );

        for( size_t i = 0; i < strl; ++i, ++itwhere )
        {
            (*itwhere) = str[i];/*static_cast<typename _outit::container_type::value_type>(str[i]);*/
        }
        return itwhere;
    }

    /*
        WriteStrToByteContainer
            Write a c string to a byte container, via iterator.
            strl is the length of the string!
    */
    template<class _init> 
        _init ReadStrFromByteContainer( _init itread,  char * str, size_t strl )
    {
        //#FIXME: The static assert below is broken with non-backinsert iterators
        //static_assert( typename std::is_same<typename _init::container_type::value_type, uint8_t>::type::value, "WriteStrToByteContainer: Target container's value_type can't be assigned bytes!" );
        
        for( size_t i = 0; i < strl; ++i, ++itread )
            str[i] = *itread;
        return itread;

        //return std::copy_n( , strl, reinterpret_cast<const typename _init::container_type::value_type*>(str) );
    }

    template<class _init> 
        _init ReadStrFromByteContainer( _init itread, _init itend, char * str, size_t strl )
    {
        //#FIXME: The static assert below is broken with non-backinsert iterators
        //static_assert( typename std::is_same<typename _init::container_type::value_type, uint8_t>::type::value, "WriteStrToByteContainer: Target container's value_type can't be assigned bytes!" );
        
        for( size_t i = 0; (i < strl) && (itend != itread); ++i, ++itread )
            str[i] = *itread;
        return itread;

        //return std::copy_n( , strl, reinterpret_cast<const typename _init::container_type::value_type*>(str) );
    }

    /*
        WriteStrToByteContainer
            Write a std::string as a null-terminated string into a byte container!

            If is the string has only a \0 character, only that character will be written!
    */
    template<class _outit> 
        _outit WriteStrToByteContainer( _outit itwhere, const std::string & str )
    {
        //if( str.empty() )
        //{
        //    (*itwhere) = '\0';
        //    ++itwhere;
        //    return itwhere;
        //}
        //else
            return WriteStrToByteContainer( itwhere, str.c_str(), str.size()+1 );
    }

    /*
        ReadCStrFromBytes
            Reads a null terminated 8bits c-string.

            -> beg is modified!
    */
    template<class _init>
        std::string ReadCStrFromBytes( _init & beg, _init pastend )
    {
        std::string result;

        for( ;beg != pastend && (*beg) != 0; ++beg )
            result.push_back(*beg);

        if( beg == pastend )
            throw std::runtime_error("String went past expected end!");

        return result;
    }


    /************************************************************************************
        safestrlen
            Count the length of a string, and has a iterator check
            to ensure it won't loop into infinity if it can't find a 0.
    ************************************************************************************/
    template<typename init_t>
        inline size_t safestrlen( init_t beg, init_t pastend )
    {
        using namespace std;
        size_t cntchar = 0;
        for(; beg != pastend && (*beg) != 0; ++cntchar, ++beg );

        if( beg == pastend )
            throw runtime_error("String went past expected end!");

        return cntchar;
    }

    /************************************************************************************
        FetchString
            Fetchs a null terminated C-String from a file offset.
    ************************************************************************************/
    template<typename _init>
        std::string FetchString( uint32_t fileoffset, _init itfbeg, _init itfend )
    {
        using namespace std;
        auto    itstr = itfbeg;
        std::advance( itstr,  fileoffset );
        size_t  strlength = safestrlen(itstr, itfend);
        string  dest;
        dest.resize(strlength);

        for( size_t cntchar = 0; cntchar < strlength; ++cntchar, ++itstr )
            dest[cntchar] = (*itstr);

        return dest;
    }


    /*********************************************************************************************
        CalculateLengthPadding
            Calculates the length of padding required to align an address/length
            on the given value!
            
            Parameters:
                - length : The length to align.
                - alignon: The number by which the length should be divisible by.

            Return:
                The number of padding bytes necessary to align the given length on
                "alignon".
    *********************************************************************************************/
    template<typename _SizeTy>
        inline _SizeTy CalculateLengthPadding( _SizeTy length, _SizeTy alignon )
    {
        return ( (length % alignon) != 0 )? (alignon - (length % alignon)) : 0;
    }

    /*********************************************************************************************
        CalculatePaddedLengthTotal
            Calculate the nb of padding bytes needed to align a given length on 
            the divisor specified, and add those to the length specified. Returning
            the total amount of bytes after adding the padding bytes.
            
            Parameters:
                - length : The length to align.
                - alignon: The number by which the length should be divisible by.

            Return:
                The total amount of bytes after adding the padding bytes.
    *********************************************************************************************/
    template<typename _SizeTy>
        inline _SizeTy CalculatePaddedLengthTotal( _SizeTy length, _SizeTy alignon )
    {
        return CalculateLengthPadding(length, alignon) + length;
    }

    /*********************************************************************************************
        AppendPaddingBytes
            This function takes a back insert iterator and the length of the container to append padding
            to, along with the length to align on to determine how much padding is needed.

            Return the nb of padding bytes that were inserted!
    *********************************************************************************************/
    template<class _backinit>
        size_t AppendPaddingBytes( _backinit itinsertat, size_t lentoalign, size_t alignon, const uint8_t PadByte = 0 )
    {
        size_t lenpadding = CalculateLengthPadding(lentoalign, alignon);
        for( size_t ctpad = 0; ctpad < lenpadding; ++ctpad, ++itinsertat )
            itinsertat = PadByte;
        return lenpadding;
    }

};

#endif