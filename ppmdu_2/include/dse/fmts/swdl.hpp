#ifndef SWDL_HPP
#define SWDL_HPP
/*
swdl.hpp
2015/05/20
psycommando@gmail.com
Description: Utilities for handling Pokemon Mystery Dungeon: Explorers of Sky/Time/Darkness's .swd files.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <dse/dse_common.hpp>
#include <dse/containers/dse_preset_bank.hpp>

#include <utils/utility.hpp>

#include <cstdint>
#include <vector>
#include <array>
#include <string>

#ifdef USE_PPMDU_CONTENT_TYPE_ANALYSER
    #include <types/content_type_analyser.hpp>
    namespace filetypes
    {
        extern const ContentTy CnTy_SWDL; //Content ID db handle 
    };
#endif

namespace DSE
{
//====================================================================================================
//  Forward Declarations
//====================================================================================================
    struct SWDL_Header_v402;
    struct SWDL_Header_v415;

//====================================================================================================
//  Constants
//====================================================================================================
    static const uint32_t SWDL_MagicNumber     = static_cast<uint32_t>(eDSEContainers::swdl);//0x7377646C; //"swdl"
    static const uint32_t SWDL_ChunksDefParam1 = 0x4150000;
    static const uint32_t SWDL_ChunksDefParam2 = 0x10;

    static const uint16_t SWDL_Version415      = 0x415;
    static const uint16_t SWDL_Version402      = 0x402;

//====================================================================================================
// SWDL_HeaderData
//====================================================================================================

    /// <summary>
    /// A version independant struct to contain SWDL header data.
    /// </summary>
    struct SWDL_HeaderData
    {
        static const uint32_t FNameLen = 16;
        uint32_t unk18           = 0;
        uint32_t flen            = 0;
        uint16_t version         = 0;
        uint8_t  bankid_low      = 0;
        uint8_t  bankid_high     = 0;
        uint32_t unk3            = 0;
        uint32_t unk4            = 0;

        uint16_t year            = 0;
        uint8_t  month           = 0;
        uint8_t  day             = 0;
        uint8_t  hour            = 0;
        uint8_t  minute          = 0;
        uint8_t  second          = 0;
        uint8_t  centisec        = 0;
        std::array<char, FNameLen> fname{0xFFi8};

        //Common 
        uint16_t nbwavislots     = 0;
        uint16_t nbprgislots     = 0;

        //v415 only
        uint16_t unk14           = 0;
        uint32_t pcmdlen         = 0;
        uint16_t wavilen         = 0;
        uint16_t unk17           = 0;

        //v402 only
        uint8_t nbkeygroups      = 0;

        SWDL_HeaderData & operator=( const SWDL_Header_v402 & other );
        SWDL_HeaderData & operator=( const SWDL_Header_v415 & other );
        operator SWDL_Header_v402();
        operator SWDL_Header_v415();
    };

//====================================================================================================
// SWDL_Header_v402
//====================================================================================================

    /// <summary>
    /// The header of the version 0x402 SWDL file.
    /// </summary>
    struct SWDL_Header_v402
    {
        static const uint32_t Size     = 80;
        static const uint32_t FNameLen = 16;

        static const uint16_t DefVersion = SWDL_Version402;
        static const uint32_t DefUnk10   = 0xFFFFFF00;
        static const uint32_t DefUnk13   = 0x10;

        static const uint32_t NbPadBytes = 7;
        static const uint8_t  PadBytes   = 0xFF;

        static unsigned int size() { return Size; }

        uint32_t magicn          = 0;
        uint32_t unk18           = 0;
        uint32_t flen            = 0;
        uint16_t version         = 0;
        uint8_t  unk1            = 0;
        uint8_t  unk2            = 0;
        uint32_t unk3            = 0;
        uint32_t unk4            = 0;

        uint16_t year            = 0;
        uint8_t  month           = 0;
        uint8_t  day             = 0;
        uint8_t  hour            = 0;
        uint8_t  minute          = 0;
        uint8_t  second          = 0;
        uint8_t  centisec        = 0;

        std::array<char, FNameLen> fname{0};

        uint32_t unk10           = 0;
        uint32_t unk11           = 0;
        uint32_t unk12           = 0;
        uint32_t unk13           = 0;

        uint32_t unk15           = 0;
        uint16_t unk16           = 0;
        uint8_t  nbwavislots     = 0;
        uint8_t  nbprgislots     = 0;
        uint8_t  nbkeygroups     = 0;

        template<class _outit> _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes   ( SWDL_MagicNumber, itwriteto, false ); //Write constant magic number, to avoid bad surprises
            itwriteto = utils::WriteIntToBytes   ( unk18,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( flen,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( version,          itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk1,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk2,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk3,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk4,             itwriteto );

            itwriteto = utils::WriteIntToBytes   ( year,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( month,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( day,              itwriteto );
            itwriteto = utils::WriteIntToBytes   ( hour,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( minute,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( second,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( centisec,         itwriteto );

            itwriteto = utils::WriteStrToByteContainer( itwriteto,   fname.data(), fname.size() );

            itwriteto = utils::WriteIntToBytes   ( unk10,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk11,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk12,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk13,            itwriteto );

            itwriteto = utils::WriteIntToBytes   ( unk15,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk16,            itwriteto );
            
            itwriteto = utils::WriteIntToBytes   ( nbwavislots,      itwriteto );
            itwriteto = utils::WriteIntToBytes   ( nbprgislots,      itwriteto );
            itwriteto = utils::WriteIntToBytes   ( nbkeygroups,      itwriteto );
            
            //Put padding bytes
            itwriteto = std::fill_n( itwriteto, NbPadBytes, PadBytes );
            return itwriteto;
        }


        template<class _init> _init ReadFromContainer( _init itReadfrom, _init itEnd )
        {
            itReadfrom = utils::ReadIntFromBytes( magicn,       itReadfrom, itEnd, false ); //iterator is incremented
            itReadfrom = utils::ReadIntFromBytes( unk18,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( flen,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( version,      itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk1,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk2,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk3,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk4,         itReadfrom, itEnd );

            itReadfrom = utils::ReadIntFromBytes( year,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( month,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( day,          itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( hour,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( minute,       itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( second,       itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( centisec,     itReadfrom, itEnd );

            itReadfrom  = utils::ReadStrFromByteContainer( itReadfrom, fname.data(), FNameLen );

            itReadfrom = utils::ReadIntFromBytes( unk10,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk11,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk12,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk13,        itReadfrom, itEnd );

            itReadfrom = utils::ReadIntFromBytes( unk15,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk16,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( nbwavislots,  itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( nbprgislots,  itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( nbkeygroups,  itReadfrom, itEnd );
            
            if( itReadfrom == itEnd )
                throw std::runtime_error("Error SWDL_Header_v402::ReadFromContainer(): Reached end of file before parsing padding bytes!");

            if( *itReadfrom != PadBytes )
                std::clog << "<*>- Warning: Unexpected padding bytes 0x" <<std::hex <<static_cast<uint16_t>(*itReadfrom) <<std::dec <<" found at the end of a v402 SWDL header.\n";

            //Put padding bytes
            std::advance( itReadfrom, NbPadBytes ); //Skip padding at the end
            return itReadfrom;
        }

        operator SWDL_HeaderData()
        {
            SWDL_HeaderData smddat;
            smddat.unk18           = unk18;
            smddat.flen            = flen;
            smddat.version         = version;
            smddat.bankid_low      = unk1;
            smddat.bankid_high     = unk2;
            smddat.unk3            = unk3;
            smddat.unk4            = unk4;

            smddat.year            = year;
            smddat.month           = month;
            smddat.day             = day;
            smddat.hour            = hour;
            smddat.minute          = minute;
            smddat.second          = second;
            smddat.centisec        = centisec;

            std::copy( std::begin(fname), std::end(fname), std::begin(smddat.fname) );

            //Common 
            smddat.nbwavislots     = nbwavislots;
            smddat.nbprgislots     = nbprgislots;

            //v415 only
            //smddat.unk14           = ;
            //smddat.pcmdlen         = ;
            //smddat.wavilen         = ;
            //smddat.unk17           = ;

            //v402 only
            smddat.nbkeygroups     = nbkeygroups;
            return std::move(smddat);
        }
    };

//====================================================================================================
// SWDL_Header_v415
//====================================================================================================

    /// <summary>
    /// The SWDL header of the version 0x415 SWDL file.
    /// </summary>
    struct SWDL_Header_v415
    {
        static const uint32_t Size     = 80;
        static const uint32_t FNameLen = 16;

        static const uint16_t DefVersion = SWDL_Version415;
        static const uint32_t DefUnk10   = 0xAAAAAA00;
        static const uint32_t DefUnk13   = 0x10;

        static const uint32_t MaskNoPCMD = 0xFFFF0000;
        static const uint32_t ValNoPCMD  = 0xAAAA0000;


        static unsigned int size() { return Size; }

        uint32_t magicn          = 0;
        uint32_t unk18           = 0;
        uint32_t flen            = 0;
        uint16_t version         = 0;
        uint8_t  bankid_low      = 0;
        uint8_t  bankid_high     = 0;
        uint32_t unk3            = 0;
        uint32_t unk4            = 0;

        uint16_t year            = 0;
        uint8_t  month           = 0;
        uint8_t  day             = 0;
        uint8_t  hour            = 0;
        uint8_t  minute          = 0;
        uint8_t  second          = 0;
        uint8_t  centisec        = 0;

        std::array<char, FNameLen> fname{0xAAi8};
        uint32_t unk10           = 0;
        uint32_t unk11           = 0;

        uint32_t unk12           = 0;
        uint32_t unk13           = 0;
        uint32_t pcmdlen         = 0;
        uint16_t unk14           = 0;
        uint16_t nbwavislots     = 0;
        uint16_t nbprgislots     = 0;
        uint16_t unk17           = 0;
        uint16_t wavilen         = 0;


        template<class _outit> _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes   ( SWDL_MagicNumber, itwriteto, false ); //Write constant magic number, to avoid bad surprises
            itwriteto = utils::WriteIntToBytes   ( unk18,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( flen,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( version,          itwriteto );
            itwriteto = utils::WriteIntToBytes   ( bankid_low,       itwriteto );
            itwriteto = utils::WriteIntToBytes   ( bankid_high,      itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk3,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk4,             itwriteto );

            itwriteto = utils::WriteIntToBytes   ( year,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( month,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( day,              itwriteto );
            itwriteto = utils::WriteIntToBytes   ( hour,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( minute,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( second,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( centisec,         itwriteto );

            itwriteto = utils::WriteStrToByteContainer( itwriteto,   fname.data(), fname.size() );

            itwriteto = utils::WriteIntToBytes   ( unk10,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk11,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk12,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk13,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( pcmdlen,          itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk14,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( nbwavislots,      itwriteto );
            itwriteto = utils::WriteIntToBytes   ( nbprgislots,      itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk17,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( wavilen,          itwriteto );
            return itwriteto;
        }


        template<class _init> _init ReadFromContainer( _init itReadfrom, _init itEnd )
        {
            itReadfrom = utils::ReadIntFromBytes( magicn,       itReadfrom, itEnd, false ); //iterator is incremented
            itReadfrom = utils::ReadIntFromBytes( unk18,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( flen,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( version,      itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( bankid_low,   itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( bankid_high,  itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk3,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk4,         itReadfrom, itEnd );

            itReadfrom = utils::ReadIntFromBytes( year,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( month,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( day,          itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( hour,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( minute,       itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( second,       itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( centisec,     itReadfrom, itEnd );

            itReadfrom  = utils::ReadStrFromByteContainer( itReadfrom, fname.data(), FNameLen );

            itReadfrom = utils::ReadIntFromBytes( unk10,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk11,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk12,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk13,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( pcmdlen,      itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk14,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( nbwavislots,  itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( nbprgislots,  itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk17,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( wavilen,      itReadfrom, itEnd );
            return itReadfrom;
        }

        operator SWDL_HeaderData()
        {
            SWDL_HeaderData smddat;
            smddat.unk18           = unk18;
            smddat.flen            = flen;
            smddat.version         = version;
            smddat.bankid_low      = bankid_low;
            smddat.bankid_high     = bankid_high;
            smddat.unk3            = unk3;
            smddat.unk4            = unk4;

            smddat.year            = year;
            smddat.month           = month;
            smddat.day             = day;
            smddat.hour            = hour;
            smddat.minute          = minute;
            smddat.second          = second;
            smddat.centisec        = centisec;

            std::copy( std::begin(fname), std::end(fname), std::begin(smddat.fname) );

            //Common 
            smddat.nbwavislots     = nbwavislots;
            smddat.nbprgislots     = nbprgislots;

            //v415 only
            smddat.unk14           = unk14;
            smddat.pcmdlen         = pcmdlen;
            smddat.wavilen         = wavilen;
            smddat.unk17           = unk17;
            return std::move(smddat);
        }

    };

//====================================================================================================
// Functions
//====================================================================================================

    /// <summary>
    /// Reads only the SWDL header from the SWDL file at filepath, and returns the parsed header.
    /// </summary>
    /// <param name="filepath">Path to the file we should parse the header of.</param>
    /// <returns>The SWDL header for the file at filepath.</returns>
    SWDL_HeaderData ReadSwdlHeader(const std::string& filepath);

    /// <summary>
    /// Parse only the header of the SWDL file raw data within the given range, and returns the parsed header.
    /// </summary>
    /// <param name="itbeg">Begining of the SWDL file data to parse.</param>
    /// <param name="itend">End of the SWDL file data to parse.</param>
    /// <returns>The SWDL header parsed from the raw data.</returns>
    SWDL_HeaderData ReadSwdlHeader(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend);

    /// <summary>
    /// Parse a SWDL file, and returns the result into a PresetBank object.
    /// </summary>
    /// <param name="filepath">The path to the SWDL file to parse.</param>
    /// <returns>The parsed SWDL as a PresetBank object.</returns>
    PresetBank ParseSWDL( const std::string& filepath);

    /// <summary>
    /// Parse raw SWDL file data into a PresetBank object.
    /// </summary>
    /// <param name="itbeg">Start of the SWDL data to be parsed.</param>
    /// <param name="itend">End of the SWDL data to be parsed.</param>
    /// <returns>The parsed SWDL as a PresetBank object.</returns>
    PresetBank ParseSWDL(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend);

    /// <summary>
    /// Write a SWDL file from the given PresetBank, into a new SWDL file located at filepath.
    /// </summary>
    /// <param name="filepath">The path to the SWDL file that will be created.</param>
    /// <param name="out_audiodata">The PresetBank object to write into the new SWDL file.</param>
    void WriteSWDL( const std::string& filepath, const PresetBank& out_audiodata );

    /*
        GetDSEHeaderLen
            Returns the length of the SWDL header for the version of DSE specified
    */
    constexpr size_t GetDSEHeaderLen(eDSEVersion ver) //#FIXME: Is this needed? Because both versions have the same length?
    {
        if(ver == eDSEVersion::V415)
            return SWDL_Header_v415::Size;
        else if(ver == eDSEVersion::V402)
            return SWDL_Header_v402::Size;
        else
        {
            throw std::runtime_error("Error: GetDSEHeaderLen() : Bad DSE version!");
            return SWDL_Header_v415::Size;
        }
    }

};

//====================================================================================================
//Stream Operators
//====================================================================================================
std::ostream& operator<<(std::ostream& os, const DSE::SWDL_HeaderData& hdr);
std::ostream& operator<<(std::ostream& os, const DSE::SWDL_Header_v415& hdr);
std::ostream& operator<<(std::ostream& os, const DSE::SWDL_Header_v402& hdr);

#endif