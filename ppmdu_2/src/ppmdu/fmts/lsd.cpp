#include <ppmdu/fmts/lsd.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <utils/utility.hpp>

using namespace std;
using namespace utils;

namespace filetypes
{

    //
    template<class _init>
        lsddata_t _ParseLSD( _init beg, _init end )
    {
        const uint16_t nbentries = ReadIntFromBytes<uint16_t>( beg, end ); // beg is incremented
        const int      diff      = distance( beg, end );

        assert(diff > 0);
        if(static_cast<unsigned int>(diff) < (nbentries * LSDStringLen))
        {
            stringstream sstr;
            sstr << "_ParseLSD(): Error, lsd file is too short for the nb of strings specified in the header!";
            throw runtime_error( sstr.str() );
        }

        lsddata_t out(nbentries); 

        //Fill up all the strings
        for( size_t i = 0; i < nbentries; ++i )
        {
            for( auto & achar : out[i] )
                achar = ReadIntFromBytes<char>( beg, end ); // beg is incremented
        }

        return move(out);
    }

    //
    template<class _outit>
        _outit _WriteLSD( _outit itwrite, const lsddata_t & data )
    {
        itwrite = WriteIntToBytes( static_cast<uint16_t>(data.size()), itwrite );

        for( const auto & entry : data )
        {
            itwrite = std::copy( entry.begin(), entry.end(), itwrite );
        }

        return itwrite;
    }


    /*
    */
    lsddata_t ParseLSD( const std::string & fpath )
    {
        std::vector<uint8_t> fdata = move(utils::io::ReadFileToByteVector(fpath));
        return move(_ParseLSD( fdata.begin(), fdata.end() ));
    }

    /*
    */
    void WriteLSD( const lsddata_t & lsdcontent, const std::string & fpath )
    {
        ofstream outf( fpath, ios::out| ios::binary );

        if( !outf.is_open() || outf.bad() )
        {
            stringstream sstr;
            sstr << "WriteLSD(): Error, couldn't open the target file \"" <<fpath <<"\"!";
            throw runtime_error( sstr.str() );
        }

        ostreambuf_iterator<char> oit(outf);
        _WriteLSD( oit, lsdcontent );
    }

};