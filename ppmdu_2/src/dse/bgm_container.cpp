#include <dse/bgm_container.hpp>
#include <ppmdu/fmts/sir0.hpp>
#include <ppmdu/fmts/smdl.hpp>
#include <ppmdu/fmts/swdl.hpp>
#include <ppmdu/fmts/sedl.hpp>
#include <sstream>
#include <fstream>
#include <array>
#include <iostream>
#include <types/content_type_analyser.hpp>
#include <utils/library_wide.hpp>

using namespace std;
using namespace filetypes;

namespace DSE
{
//==========================================================================================
//
//==========================================================================================

    /*
        Returns the offsets of the swdl and smdl in order.
    */
    template<class _init>
        std::array<uint32_t,2> ReadOffsetsSubHeader( _init itread, _init itpastend, uint32_t subhdroffset )
    {
        std::array<uint32_t,2> offsets = {0};
        std::advance( itread, subhdroffset );
        itread = utils::ReadIntFromBytes( offsets[0], itread, itpastend);
        itread = utils::ReadIntFromBytes( offsets[1], itread, itpastend);
        return offsets;
    }

//==========================================================================================
//  Functions
//==========================================================================================

    /*
        IsBgmContainer
            Pass a path to a file, and the function will return whether or not its a bgm container.
            
            It will actually check to see if its a SIR0 container, then it will attempt to look at the
            sub-header of the SIR0 wrapper, and if there are 2 pointers, one to a swdl and another to a smdl, 
            then it will return true!
            It won't do any kind of validation on the smdl and swdl though. It only checks for magic numbers.

    */
    bool IsBgmContainer( const std::string & filepath )
    {
        ifstream infile( filepath, ios::in | ios::binary );
        istreambuf_iterator<char> itread(infile); //The iterator doesn't need to be updated on istreams
        istreambuf_iterator<char> itend; //End of file is default iterator
        
        if( infile.is_open() && infile.good() )
        {
            sir0_header hdr;

            hdr.ReadFromContainer(itread, itend);
            if( hdr.magic == sir0_header::MAGIC_NUMBER )
            {
                infile.seekg(0);
                auto offsets = ReadOffsetsSubHeader(itread, itend, hdr.subheaderptr );

                //SWDL_HeaderData swdhdr;
                //SMDL_Header smdhdr;
                infile.seekg(offsets[0]);
                uint32_t swdlrmagic = {};
                itread = utils::ReadIntFromBytes(swdlrmagic, itread, itend, false);
                

                //swdhdr.ReadFromContainer( istreambuf_iterator<char>(infile) );

                infile.seekg(offsets[1]);
                uint32_t smdlrmagic = {};
                itread = utils::ReadIntFromBytes(smdlrmagic, itread, itend, false);
                //smdhdr.ReadFromContainer( istreambuf_iterator<char>(infile) );

                //if( swdhdr.magicn == SWDL_MagicNumber && smdhdr.magicn == SMDL_MagicNumber )
                //    return true;

                if( swdlrmagic == SWDL_MagicNumber && smdlrmagic == SMDL_MagicNumber )
                    return true;
            }
        }
        else
            throw runtime_error( "IsBgmContainer() : Failed to open file " + filepath );

        return false;
    }

    std::pair<PresetBank, MusicSequence> ReadBgmContainer(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend, std::string filepath)
    {
        sir0_header hdr;
        hdr.ReadFromContainer(itbeg, itend);

        if (hdr.magic != sir0_header::MAGIC_NUMBER)
            throw runtime_error("ReadBgmContainer() : File is missing SIR0 header!");

        auto     offsets = ReadOffsetsSubHeader(itbeg, itend, hdr.subheaderptr);
        uint32_t magicn1 = 0;
        uint32_t magicn2 = 0;
        utils::ReadIntFromBytes(magicn1, itbeg + offsets[0], itend, false);
        utils::ReadIntFromBytes(magicn2, itbeg + offsets[1], itend, false);

        size_t smdloffset = 0;
        size_t swdloffset = 0;

        //Check that the swdl and smdl containers are in the right order, or adapt if they're inverted
        if (magicn1 == SWDL_MagicNumber && magicn2 == SMDL_MagicNumber)
        {
            //If in this order
            swdloffset = offsets[0];
            smdloffset = offsets[1];
        }
        else if (magicn2 == SWDL_MagicNumber && magicn1 == SMDL_MagicNumber)
        {
            //If in inverted order
            swdloffset = offsets[1];
            smdloffset = offsets[0];

            if (utils::LibWide().isLogOn() && !filepath.empty())
                clog << "<!>- SWDL and SMDL containers were inverted in the bgm container \"" << filepath << "\" !\n";
        }
        else
        {
            stringstream sstrerror;

            //throw runtime_error( "ReadBgmContainer(): Bgm container doesn't contain" );
            sstrerror << "ReadBgmContainer(): Bgm container has unexpected content!";

            if (magicn2 == SMDL_MagicNumber)
                sstrerror << " SMDL header present! ";
            else
                sstrerror << " SMDL header missing! ";

            if (magicn1 == SWDL_MagicNumber)
                sstrerror << "SWDL header present! ";
            else
                sstrerror << "SWDL header missing! ";

            if (magicn2 == SEDL_MagicNumber)
                sstrerror << "Found unexpected SEDL header! " << "Bgm container is unexpected sound effect container!";
            else
                sstrerror << "Found unknown header magic number: " << showbase << hex << magicn2 << dec << noshowbase << "!\n";
            throw runtime_error(sstrerror.str());
        }

        auto itbegswdl = itbeg + swdloffset;
        auto itendswdl = itbeg + smdloffset;
        auto itbegsmdl = itendswdl;
        auto itendsmdl = itbeg + hdr.subheaderptr;

        return move(make_pair(move(ParseSWDL(itbegswdl, itendswdl)), move(ParseSMDL(itbegsmdl, itendsmdl))));
    }

    /*
        ReadBgmContainer
            Read the bgm container's content into a preset bank and a musicsequence.
    */
    std::pair<PresetBank, MusicSequence> ReadBgmContainer( const std::string & filepath )
    {
        vector<uint8_t> fdata(utils::io::ReadFileToByteVector(filepath));
        return ReadBgmContainer(fdata.begin(), fdata.end(), filepath);
    }

    /*
        WriteBgmContainer
            Writes a bgm container from a music sequence and a preset bank.

            - filepath      : The filename + path to output to. Extension included.
            - presbnk       : A preset bank to be exported.
            - mus           : A music sequence to be exported.
            - nbpadcontent  : The nb of bytes of padding to put between the SIR0 header and the 
                              header of the first sub-container. Usually 16 to 64
            - alignon       : Extra padding will be added between sections and at the end of the file
                              so that section start on offsets divisible by that.

        The SWDL is written first, then the SMDL, and the pointers in the subheader are in that order too !
    */
    void WriteBgmContainer( const std::string   & filepath, 
                            const PresetBank    & presbnk, 
                            const MusicSequence & mus,
                            size_t                nbpadcontent,
                            size_t                alignon )
    {
    }
};