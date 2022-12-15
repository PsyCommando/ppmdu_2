#ifndef COLOR_HPP
#define COLOR_HPP
/*
color.hpp
19/05/2014
psycommando@gmail.com
Description:


No crappyrights. All wrongs reversed !
*/
#include <algorithm>
#include <vector>
#include <array>
#include <cstdint>
#include <utils/utility.hpp>

namespace gimg
{
//==================================================================
// Classes
//==================================================================
    class colorRGB24;

    /*
        color_container
            A generic class to represent a color made up of several components.
            It also offers facilities to get a RGB24 representation of the color.
    */
    template< class _COLOR_DATA_T, unsigned int _NB_COMPONENTS, class _RGB_24_T = colorRGB24 >
        class color_container
    {
    public:
        typedef _COLOR_DATA_T                                           colordata_t;
        typedef color_container<_COLOR_DATA_T,_NB_COMPONENTS,_RGB_24_T> myty_t;

        static const unsigned int NB_COMPONENTS = _NB_COMPONENTS;

        color_container() { std::fill( m_colcomponents.begin(), m_colcomponents.end(), 0 ); }
        color_container( const myty_t & other ){ m_colcomponents = other.m_colcomponents; }
        virtual ~color_container(){}

        inline static unsigned int GetNbComponents()                     { return NB_COMPONENTS;          }
        inline colordata_t &       operator[]( unsigned int index )      { return m_colcomponents[index]; }
        inline const colordata_t & operator[]( unsigned int index )const { return m_colcomponents[index]; }


        inline myty_t & operator=( const myty_t & other ) 
        { 
            m_colcomponents = other.m_colcomponents;
            return *this;
        }

        // -- Virtual stuff --
        virtual _RGB_24_T       getAsRGB24  ()const                             = 0;
        virtual void            setFromRGB24( uint8_t r, uint8_t g, uint8_t b ) = 0;
        virtual void            setFromRGB24( const colorRGB24 & col )          = 0;

        template<class _outit> inline _outit WriteAsRawByte( _outit itwhere, bool blittleendianorder = true )const //#TODO: is "blittleendianorder" a good name ? what about "binvertendian" ?
        {
            for( unsigned int i = 0; i < NB_COMPONENTS; ++i  )
                itwhere = utils::WriteIntToBytes( m_colcomponents[i], itwhere, blittleendianorder );
            return itwhere;
        }

        template<class _init> inline _init ReadAsRawByte( _init itwhere, _init itpastend, bool blittleendianorder = true ) //#TODO: is "blittleendianorder" a good name ? what about "binvertendian" ?
        {
            for( unsigned int i = 0; i < NB_COMPONENTS; ++i  )
                m_colcomponents[i] = utils::ReadIntFromBytes<colordata_t>( itwhere, itpastend, blittleendianorder );
            return itwhere;
        }

        inline static unsigned int getSizeRawBytes()
        {
            return NB_COMPONENTS * sizeof(colordata_t);
        }

    protected:
        std::array<colordata_t, NB_COMPONENTS> m_colcomponents;
    };


    //=========================================
    // RGB-24bits Format
    //=========================================
    class colorRGB24 : public color_container<uint8_t, 3, colorRGB24>
    {
    public:
        colordata_t & red, & green, & blue; //Aliases

        colorRGB24();
        colorRGB24( colordata_t r, colordata_t g, colordata_t b );
        colorRGB24( const colorRGB24 & other );
        colorRGB24 & operator=( const colorRGB24 & other );
        
        //Overrides
        ~colorRGB24(){}
        colorRGB24 getAsRGB24  ()const;
        void       setFromRGB24( uint8_t r, uint8_t g, uint8_t b );
        void       setFromRGB24( const colorRGB24 & col );
    };

    //=========================================
    // RGBX-32bits Format
    //=========================================
    class colorRGBX32 : public color_container<uint8_t, 4, colorRGB24>
    {
    public:
        colordata_t & _red, & _green, & _blue, & _x; //Aliases

        colorRGBX32();
        colorRGBX32( colordata_t r, colordata_t g, colordata_t b, colordata_t x );
        colorRGBX32( const colorRGBX32 & other );
        colorRGBX32 & operator=( const colorRGBX32 & other );
        
        //Overrides
        ~colorRGBX32(){}
        colorRGB24       getAsRGB24  ()const;
        void             setFromRGB24( uint8_t r, uint8_t g, uint8_t b );
        void             setFromRGB24( const colorRGB24 & col );
    };
};

#endif