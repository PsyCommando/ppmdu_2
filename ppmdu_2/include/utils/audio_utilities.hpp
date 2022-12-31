#ifndef AUDIO_UTILITIES_HPP
#define AUDIO_UTILITIES_HPP
/*
audio_utilities.hpp
2015/11/09
psycommando@gmail.com
Description: 
    A set of functions used often when dealing with audio.
*/
#include <vector>
#include <cstdint>
#include <utils/utility.hpp>

namespace utils
{
    /*
        A few typedefs to pave the way for making specialized types and containers, with minimal code re-writing.
    */

    //Sample formats
    typedef int16_t pcm16s_t;
    typedef int8_t  pcm8s_t;
    typedef uint8_t adpcm4_t; 

    //Sample containers
    typedef std::vector<pcm16s_t> pcm16ssmpls_t;
    typedef std::vector<pcm8s_t>  pcm8ssmpls_t;
    typedef std::vector<adpcm4_t> adpcm4smpls_t;


//
//
//
    template<class _backinsit>
        void InterlacePCM16( const std::vector<std::vector<pcm16s_t>> & chanbuff, _backinsit itout )
    {
        //Make sure all channel buffers are the same length
        size_t longest    = 0;
        //bool   allsamelen = true;

        for( size_t cntchan = 0; cntchan < chanbuff.size(); ++cntchan )
        {
            if( longest < chanbuff[cntchan].size() )
            {
                //if( longest != 0 )
                //    allsamelen = false;
                longest = chanbuff[cntchan].size();
            }
        }

        //If not all the same length, add some zeros to the shorter ones
        //if( !allsamelen )
        //{
        //    for( size_t cntchan = 0; cntchan < chanbuff.size(); ++cntchan )
        //    {
        //        if( (longest - chanbuff[cntchan].size()) != 0 )
        //        { 
        //            auto itblock = std::back_inserter( chanbuff[cntchan] );
        //            std::fill_n( itblock, (longest - chanbuff[cntchan].size()), 0 ); //Fill with zeros
        //        }
        //    }
        //}


        //Interlace samples 
        for( size_t cntsample = 0; cntsample < longest; ++cntsample )
        {
            for( size_t cntchan = 0; cntchan < chanbuff.size(); ++cntchan )
            {
                if( cntsample < chanbuff[cntchan].size() )
                    (*itout) = chanbuff[cntchan][cntsample];
                else
                    (*itout) = 0; //Insert zero samples to pad valid samples
                ++itout;
            }
        }
    }


//=============================================================================================
//  Simple Sample Parsers
//=============================================================================================

    /*********************************************************************************************
        RawPCM16Parser
            Parse PCM16 samples from the vector of raw bytes, and put them into the target vector
            and specified type.
    *********************************************************************************************/
    template<class _OutSmplTy, class _init, class _backinsit>
        void RawPCM16Parser2BackIns( _init itbegraw, _init itendraw, _backinsit itdest )
    {
        for(; itbegraw != itendraw; ++itdest )
            (*itdest) = utils::ReadIntFromBytes<_OutSmplTy>(itbegraw, itendraw); //Iterator is incremented
    }

    //Iterator range variant
    template<class _OutSmplTy, class _init>
        std::vector<_OutSmplTy> RawPCM16Parser( _init itbegraw, _init itendraw )
    {
        std::vector<_OutSmplTy> out;
        const auto len = std::distance( itbegraw, itendraw );
        if( len % 2 != 0 )
        {
            throw std::runtime_error( "RawPCM16Parser(): Raw data size not a multiple of 2!" );
        }
        out.reserve(len/2);

        RawPCM16Parser2BackIns<_OutSmplTy>( itbegraw, itendraw, std::back_inserter(out) );
        return out;
    }

    //Vector variant
    template<class _OutSmplTy>
        std::vector<_OutSmplTy> RawVector2PCM16Parser( const std::vector<uint8_t> & raw )
    {
        return RawPCM16Parser<_OutSmplTy>(raw.begin(), raw.end());
    }

    /*********************************************************************************************
        RawPCM8Parser
            Parse PCM8 samples from the vector of raw bytes, and put them into the target vector
            and specified type.
    *********************************************************************************************/
    template<class _OutSmplTy, class _init, class _backinsit>
        void RawPCM8Parser( _init itbegraw, _init itendraw, _backinsit itdest )
    {
        for(; itbegraw != itendraw; ++itbegraw, ++itdest )
            (*itdest) = static_cast<_OutSmplTy>(*itbegraw);
    }

    //Iterator range variant
    template<class _OutSmplTy, class _init>
        std::vector<_OutSmplTy> RawPCM8Parser( _init itbegraw, _init itendraw )
    {
        std::vector<_OutSmplTy> out;
        const auto len = std::distance( itbegraw, itendraw );
        out.reserve(len*2);
        RawPCM8Parser<_OutSmplTy>( itbegraw, itendraw, std::back_inserter(out) );
        return out;
    }

    //Vector variant
    template<class _OutSmplTy>
        std::vector<_OutSmplTy> RawPCM8Parser( const std::vector<uint8_t> & raw )
    {
        return RawPCM8Parser<_OutSmplTy>( raw.begin(), raw.end() );
    }

    /*
        PCM8RawBytesToPCM16Vec
            Take a vector of raw pcm8 samples, and put them into a pcm16 vector!
            Do extra processing to handle sample sign compared to the conversion from raw uint8.
    */
    std::vector<int16_t> PCM8RawBytesToPCM16Vec(const std::vector<uint8_t>* praw);
    std::vector<uint8_t> PCM16VecToRawBytes(const std::vector<int16_t>* vec);

    /*
    RawBytesToPCM16Vec
        Take a vector of 16 bits signed pcm samples as raw bytes, and put them into a vector of
        signed 16 bits integers!
    */
    std::vector<int16_t> RawBytesToPCM16Vec(const std::vector<uint8_t>* praw);
};

#endif