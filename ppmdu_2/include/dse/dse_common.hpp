#ifndef DSE_COMMON_HPP
#define DSE_COMMON_HPP
/*
dse_common.hpp
2015/06/06
psycommando@gmail.com
Description: Common data between several of the Procyon Studio Digital Sound Element sound driver.
*/
//#include <ppmdu/pmd2/pmd2_audio_data.hpp>
#include <utils/utility.hpp>
#include <cstdint>
#include <ctime>
#include <vector>
#include <array>
#include <string>
#include <map>
#include <cassert>
#include <limits>
#include <iostream>
#include <algorithm>

namespace pugi { class xml_node; };

namespace DSE
{
    enum struct eDSEChunks : uint32_t;
    enum struct eDSEContainers : uint32_t;
    constexpr eDSEChunks IntToChunkID(std::underlying_type_t<eDSEChunks> value);

//====================================================================================================
//  Typedefs
//====================================================================================================
    typedef uint16_t dsepresetid_t;
    typedef uint16_t bankid_t;
    typedef uint8_t  presetid_t;
    typedef uint16_t sampleid_t;
    typedef uint8_t  midinote_t;

//====================================================================================================
//  Constants
//====================================================================================================

    const bankid_t      InvalidBankID      = std::numeric_limits<bankid_t>::max();
    const presetid_t    InvalidPresetID    = std::numeric_limits<presetid_t>::max();
    const dsepresetid_t InvalidDSEPresetID = std::numeric_limits<dsepresetid_t>::max();
    const uint8_t       InvalidMIDIKey     = std::numeric_limits<uint8_t>::max(); //The default value for the MIDI key
    const uint8_t       DSE_MaxOctave      = 9; //The maximum octave value possible to handle

    const int8_t        DSE_DefaultPan = 64; //The default pan value
    const int8_t        DSE_DefaultVol = 127; //The default vol value

    extern const std::string DSE_SmplFmt_PCM16;
    extern const std::string DSE_SmplFmt_PCM8;
    extern const std::string DSE_SmplFmt_ADPCM4;
    extern const std::string DSE_SmplFmt_ADPCM3;
    extern const std::string DSE_SmplFmt_PSG;

    const int          NbMidiChannels = 16;
    const unsigned int NB_DSEChannels = 17;
    const unsigned int NB_DSETracks = 17;
    const unsigned int NB_DSEChunks = 11;
    const unsigned int NB_DSEContainers = 4;
    const uint32_t     SpecialChunkLen = 0xFFFFFFB0;   //Value some special chunks have as their length
    const int16_t      DSERootKey = 60;           //By default the root key for dse sequences is assumed to be 60 the MIDI standard's middle C, AKA C4
    const int8_t       DSEDefaultCoarseTune = -7;           //The default coarse tune value for splits and samples.

    /// <summary>
    /// The size of the ADPCM preamble in int32, the unit the 
    /// NDS uses to store the loop positions.
    /// Mainly here to make things more readable.
    /// </summary>
    extern const int SizeADPCMPreambleWords;

    /// <summary>
    /// Array containing all chunks labels
    /// </summary>
    extern const std::array<eDSEChunks, NB_DSEChunks>      DSEChunksList;

    /// <summary>
    /// Array containing all the DSE container's magic number.
    /// </summary>
    extern const std::array<eDSEContainers, NB_DSEContainers>  DSEContainerList;

    /// <summary>
    /// Enum containing the IDs of all chunks used by the Procyon DSE system for sequenced music and sound effects
    /// </summary>
    enum struct eDSEChunks : uint32_t
    {
        invalid,
        wavi = 0x77617669, //"wavi"
        prgi = 0x70726769, //"prgi"
        kgrp = 0x6B677270, //"kgrp"
        pcmd = 0x70636D64, //"pcmd"
        song = 0x736F6E67, //"song"
        trk  = 0x74726B20, //"trk\0x20"
        seq  = 0x73657120, //"seq\0x20"
        bnkl = 0x626E6B6C, //"bnkl"
        mcrl = 0x6D63726C, //"mcrl"
        eoc  = 0x656F6320, //"eoc\0x20"
        eod  = 0x656F6420, //"eod\0x20"
    };

    /// <summary>
    /// Enum containing the fourcc of all the known DSE file format headers.
    /// </summary>
    enum struct eDSEContainers : uint32_t
    {
        invalid,
        smdl = 0x736D646C,  //"smdl"
        swdl = 0x7377646C,  //"swdl"
        sedl = 0x7365646C,  //"sedl"
        sadl = 0x7361646C,  //"sadl"
    };

    /// <summary>
    /// Enum of the possible sample format used in the DSE engine.
    /// </summary>
    enum struct eDSESmplFmt : uint16_t
    {
        invalid    = std::numeric_limits<std::underlying_type_t<eDSESmplFmt>>::max(),
        pcm8       = 0x000,
        pcm16      = 0x100,
        ima_adpcm4 = 0x200,
        ima_adpcm3 = 0x300, //#FIXME: Fix this because PSG and adpcm3 have the same id
        //psg       = 0x300,
    };

    /// <summary>
    /// Enum with all the known DSE versions.
    /// </summary>
    enum struct eDSEVersion : uint16_t
    {
        V402 = 0x402,   //Used in Luminous Arc
        V415 = 0x415,   //Used in Pokemon Mystery Dungeon2, Zombi Daisuki, Inayusha Eleven, Professor Layton, etc..
        VDef = V415,
        VInvalid = 0,
    };

//====================================================================================================
// Structs
//====================================================================================================

    /****************************************************************************************
        DateTime
            Format used to store a date + time stamp used by all DSE formats.
    ****************************************************************************************/
    struct DateTime
    {
        uint16_t year;
        uint8_t  month;
        uint8_t  day;
        uint8_t  hour;
        uint8_t  minute;
        uint8_t  second;
        uint8_t  centsec; //100th of a second ? We don't really know what this is for..

        inline DateTime()
            :year(0), month(0), day(0), hour(0), minute(0), second(0), centsec(0)
        {}

        inline DateTime( const std::tm & src )
        {
            //http://en.cppreference.com/w/cpp/chrono/c/tm
            year    = src.tm_year + 1900; //tm_year counts the nb of years since 1900
            month   = src.tm_mon;
            day     = src.tm_mday-1;      //tm_mday begins at 1, while the time in the DSE timestamp begins at 0!
            hour    = src.tm_hour;
            minute  = src.tm_min;
            second  = (src.tm_sec == 60)? 59 : src.tm_sec; //We're not dealing with leap seconds...
            centsec = 0;
        }

        //Convert into the standard std::tm calendar time format 
        inline operator std::tm()const
        {
            std::tm result{};
            result.tm_year  = year - 1900;
            result.tm_mon   = month;
            result.tm_mday  = day + 1;
            result.tm_hour  = hour;
            result.tm_min   = minute;
            result.tm_sec   = second;
            result.tm_isdst = -1; //No info available
            return std::move(result);
        }

        inline operator std::string()const
        {
            std::tm myt = *this;
            return (std::stringstream() << std::put_time(&myt, "%Y/%m/%e-%Hh%Mm%Ss")).str();
        }

        inline void SetTimeToNow()
        {
            std::time_t t  = std::time(nullptr);
            std::tm     ti{};
#if __cplusplus > 202002L //C++23 gets rid of warnings and other annoyance you get for using localtime() on MSVC
            if(localtime_r(&t, &ti) == nullptr)
#else
            //Handling on GNU likes
            std::tm * tmptr = localtime(&t);
            if (tmptr != nullptr)
                ti = *tmptr; //Make a copy, since the pointer locatime() returns is static and not thread safe
            else
#endif
                std::clog << "<!>- DSE::DateTime::SetTimeToNow(): localtime_r returned an error while getting the current date!\n";
            //http://en.cppreference.com/w/cpp/chrono/c/tm
            year    = ti.tm_year + 1900; //tm_year counts the nb of years since 1900
            month   = ti.tm_mon;
            day     = ti.tm_mday-1;      //tm_mday begins at 1, while the time in the DSE timestamp begins at 0!
            hour    = ti.tm_hour;
            minute  = ti.tm_min;
            second  = (ti.tm_sec == 60)? 59 : ti.tm_sec; //We're not dealing with leap seconds...
        }
    };

    /// <summary>
    /// Represents a DSE envelope used in the SWDL file format!
    /// </summary>
    struct DSEEnvelope
    {
        typedef int16_t timeprop_t;
        typedef int8_t  volprop_t;

        int8_t     envmulti = 1;        //The envelope multiplier
        volprop_t  atkvol   = 0;
        timeprop_t attack   = 0;
        timeprop_t hold     = 0;
        timeprop_t decay    = 0;
        volprop_t  sustain  = 0x7F;
        timeprop_t decay2   = 0;
        timeprop_t release  = 0;
    };

    /// <summary>
    /// Format for chunks headers used in Procyon's Digital Sound Element SWDL, SMDL, SEDL format.
    /// They all follow the same 16 bytes format.
    /// </summary>
    struct ChunkHeader
    {
        static const uint32_t Size          = 16; //Length of the header
        static const uint32_t OffsetDataLen = 12; //Offset from the start of the header where the length of the chunk is stored
        uint32_t label  = 0;
        uint32_t param1 = 0;
        uint32_t param2 = 0;
        uint32_t datlen = 0;

        static unsigned int size      ()      { return 16; } //Get the size of the structure in bytes
        bool                hasLength ()const { return (datlen != SpecialChunkLen); } //Returns whether this chunk has a valid data length
        eDSEChunks          GetChunkID()const { return IntToChunkID( label ); } //Returns the enum value representing this chunk's identity, judging from the label

        //Write the structure using an iterator to a byte container
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( label,  itwriteto, false );
            itwriteto = utils::WriteIntToBytes( param1, itwriteto );
            itwriteto = utils::WriteIntToBytes( param2, itwriteto );
            itwriteto = utils::WriteIntToBytes( datlen, itwriteto );
            return itwriteto;
        }

        //Read the structure from an iterator on a byte container
        template<class _init>
            _init ReadFromContainer(  _init itReadfrom, _init itpastend )
        {
            label   = utils::ReadIntFromBytes<decltype(label)> (itReadfrom, itpastend, false ); //iterator is incremented
            param1  = utils::ReadIntFromBytes<decltype(param1)>(itReadfrom, itpastend);
            param2  = utils::ReadIntFromBytes<decltype(param2)>(itReadfrom, itpastend);
            datlen  = utils::ReadIntFromBytes<decltype(datlen)>(itReadfrom, itpastend);
            return itReadfrom;
        }
    };

    /// <summary>
    /// Entry from the "wavi" chunk in a swdl file. 
    /// This holds DSE version independent information on samples.
    /// </summary>
    struct WavInfo
    {
        sampleid_t  id         = 0;
        int8_t      ftune      = 0; 
        int8_t      ctune      = 0;
        uint8_t     rootkey    = DSERootKey;
        int8_t      ktps       = 0;
        int8_t      vol        = 0x7F;
        int8_t      pan        = 0x40;
        eDSESmplFmt smplfmt    = eDSESmplFmt::invalid;
        bool        smplloop   = false;
        uint32_t    smplrate   = 0; //Sampling rate of the sample
        uint32_t    smplpos    = 0;
        uint32_t    loopbeg    = 0; //Loop start in int32 (based on the resulting PCM16)
        uint32_t    looplen    = 0; //Length of the sample in int32
        
        //Envelope
        uint8_t     envon      = 1;
        DSEEnvelope env;
    };

    /*****************************************************************************************
        KeyGroup
            Contains info on note-stealing priority and voice range, polyphony, and maybe more.
    *****************************************************************************************/
    struct KeyGroup
    {
        static const uint32_t SIZE     = 8; //bytes
        static const uint8_t  DefPoly  = 0xFF;
        static const uint8_t  DefPrio  = 0x08;
        static const uint8_t  DefVcHi  = 0xF;
        static const uint8_t  DEF_IGNORED = 0xAA; //Value of all bytes when the keygroup is ignored/filler
        static uint32_t size() {return SIZE;}

        uint16_t id       = DEF_IGNORED | (DEF_IGNORED << 8);; //Init to filler
        uint8_t  poly     = DEF_IGNORED;
        uint8_t  priority = DEF_IGNORED;
        uint8_t  vclow    = DEF_IGNORED;
        uint8_t  vchigh   = DEF_IGNORED;
        uint8_t  unk50    = DEF_IGNORED;
        uint8_t  unk51    = DEF_IGNORED;

        bool isFiller()const
        {
            return (id == (DEF_IGNORED | (DEF_IGNORED << 8))) &&
                poly == DEF_IGNORED &&
                priority == DEF_IGNORED &&
                vclow == DEF_IGNORED &&
                vchigh == DEF_IGNORED &&
                unk50 == DEF_IGNORED &&
                unk51 == DEF_IGNORED;
        }

        void makeFiller()
        {
            id       = DEF_IGNORED | (DEF_IGNORED << 8);
            poly     = DEF_IGNORED;
            priority = DEF_IGNORED;
            vclow    = DEF_IGNORED;
            vchigh   = DEF_IGNORED;
            unk50    = DEF_IGNORED;
            unk51    = DEF_IGNORED;
        }

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( id,       itwriteto );
            itwriteto = utils::WriteIntToBytes( poly,     itwriteto );
            itwriteto = utils::WriteIntToBytes( priority, itwriteto );
            itwriteto = utils::WriteIntToBytes( vclow,    itwriteto );
            itwriteto = utils::WriteIntToBytes( vchigh,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk50,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk51,    itwriteto );
            return itwriteto;
        }


        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itpastend )
        {
            itReadfrom = utils::ReadIntFromBytes( id,       itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( poly,     itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( priority, itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( vclow,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( vchigh,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk50,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk51,    itReadfrom, itpastend );
            return itReadfrom;
        }
    };

    /// <summary>
    /// Contains details on how to setup one of the 4 LFOs 
    /// available per program presets in the dse engine.
    /// </summary>
    struct LFOTblEntry
    {
        static const uint32_t SIZE = 16; //bytes
        static uint32_t size() { return SIZE; }

        enum struct eLFODest : uint8_t
        {
            None   = 0,
            Pitch  = 1,
            Volume = 2,
            Pan    = 3,
            UNK_4  = 4,
        };

        enum struct eLFOWaveShape : uint8_t
        {
            None = 0,
            Square = 1,
            Triangle = 2,
            Sinus = 3,
            UNK4 = 4,
            Saw = 5,
            Noise = 6,
            Random = 7,
        };

        uint8_t  unk34  = 0;
        uint8_t  unk52  = 0;
        uint8_t  dest   = 0;
        uint8_t  wshape = (uint8_t)eLFOWaveShape::Square;
        uint16_t rate   = 0;
        uint16_t unk29  = 0;
        uint16_t depth  = 0;
        uint16_t delay  = 0;
        uint16_t unk32  = 0;
        uint16_t unk33  = 0;

        /*
            Return true if the LFO's fields have non-default values.
        */
        inline bool isLFONonDefault()const
        {
            return (unk52 != 0 || dest != 0 || wshape != 1 || rate != 0 ||
                        unk29 != 0 || depth != 0 || delay != 0 || unk32 != 0 || 
                        unk33 != 0 );
        }

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( unk34,  itwriteto );
            itwriteto = utils::WriteIntToBytes( unk52,  itwriteto );
            itwriteto = utils::WriteIntToBytes( dest,   itwriteto );
            itwriteto = utils::WriteIntToBytes( wshape, itwriteto );
            itwriteto = utils::WriteIntToBytes( rate,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk29,  itwriteto );
            itwriteto = utils::WriteIntToBytes( depth,  itwriteto );
            itwriteto = utils::WriteIntToBytes( delay,  itwriteto );
            itwriteto = utils::WriteIntToBytes( unk32,  itwriteto );
            itwriteto = utils::WriteIntToBytes( unk33,  itwriteto );
            return itwriteto;
        }


        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itpastend )
        {
            itReadfrom = utils::ReadIntFromBytes( unk34,  itReadfrom, itpastend  );
            itReadfrom = utils::ReadIntFromBytes( unk52,  itReadfrom, itpastend  );
            itReadfrom = utils::ReadIntFromBytes( dest,   itReadfrom, itpastend  );
            itReadfrom = utils::ReadIntFromBytes( wshape, itReadfrom, itpastend  );
            itReadfrom = utils::ReadIntFromBytes( rate,   itReadfrom, itpastend  );
            itReadfrom = utils::ReadIntFromBytes( unk29,  itReadfrom, itpastend  );
            itReadfrom = utils::ReadIntFromBytes( depth,  itReadfrom, itpastend  );
            itReadfrom = utils::ReadIntFromBytes( delay,  itReadfrom, itpastend  );
            itReadfrom = utils::ReadIntFromBytes( unk32,  itReadfrom, itpastend  );
            itReadfrom = utils::ReadIntFromBytes( unk33,  itReadfrom, itpastend  );
            return itReadfrom;
        }
    };

    /// <summary>
    /// Data on a particular sample mapped to this program preset.
    /// Associates key ranges, and etc to a specific sample.
    /// </summary>
    struct SplitEntry
    {
        uint8_t     id        = 0; //split id, second byte
        uint8_t     bendrange = 0;
        uint8_t     unk25     = 1;
        int8_t      lowkey    = 0;
        int8_t      hikey     = 0x7F;
        int8_t      lowkey2   = 0;
        int8_t      hikey2    = 0x7F;
        int8_t      lovel     = 0;
        int8_t      hivel     = 0x7F;
        int8_t      lovel2    = 0;
        int8_t      hivel2    = 0x7F;
        sampleid_t  smplid    = 0;
        int8_t      ftune     = 0;
        int8_t      ctune     = 0;
        int8_t      rootkey   = DSERootKey;
        int8_t      ktps      = 0;
        uint8_t     smplvol   = 0x7F;
        uint8_t     smplpan   = 0x40;
        uint8_t     kgrpid    = 0;
        uint8_t     envon     = 1;
        DSEEnvelope env;
    };

    /// <summary>
    /// Contains data for a single dse program preset.
    /// This is a generic version independent version of the program info structure.
    /// </summary>
    struct ProgramInfo
    {
        uint16_t id      = 0;
        uint8_t  prgvol  = 0x7F;
        uint8_t  prgpan  = 0x40;
        uint8_t  unkpoly = 0;
        uint16_t unk4    = 0;
        uint8_t  padbyte = 0;

        std::vector<LFOTblEntry> m_lfotbl; //Should be 4 usually
        std::vector<SplitEntry>  m_splitstbl;
    };

    /// <summary>
    /// Table before a group of trk chunks belonging to the same sequence.
    /// Used in both SEDL and SMDL.
    /// </summary>
    struct seqinfo_table
    {
        uint16_t unk30   = 0;
        uint16_t nextoff = 0;
        uint16_t unk16   = 0;
        uint8_t  nbtrks  = 0; //Nb of track chunks in the sequence.
        uint8_t  nbchans = 0; //Number of midi/output tracks.
        uint8_t  unk19   = 0;
        uint8_t  unk20   = 0;
        uint8_t  unk21   = 0;
        uint8_t  unk22   = 0;
        uint32_t unk23   = 0;
        uint16_t unk24   = 0;
        uint16_t unk25   = 0;
        uint16_t unk26   = 0;
        uint16_t unk27   = 0;
        uint16_t unk28   = 0;
        uint8_t  unk29   = 0;
        uint8_t  unk31   = 0;
        uint32_t unk12   = 0;
        //0x402 specific stuff since dunno what to do with those
        uint8_t  unk5    = 0;
        uint8_t  unk6    = 0;
        uint16_t unk7    = 0;
        uint32_t unk8    = 0;

        void WriteXml(pugi::xml_node seqnode)const;
        void ParseXml(pugi::xml_node seqnode);
    };


    /// <summary>
    /// Base class for the header data contained in all DSE files.
    /// </summary>
    struct DSE_MetaData
    {
        DSE_MetaData()
            :bankid_coarse(0), bankid_fine(0),createtime(), origversion(eDSEVersion::VDef), origloadorder(0)
        {}

        uint8_t     bankid_coarse; //The low byte of the bank id
        uint8_t     bankid_fine;   //The high byte of the bank id
        std::string fname;         //Internal filename
        DateTime    createtime;    //Time this was created on
        std::string origfname;     //The original filename, in the file system. If there was one. Differs from the internal name. Used to match pairs names during export.
        eDSEVersion origversion;
        size_t      origloadorder; //The order the pair this file is part of was loaded! Used for matching pair names during export.

        bankid_t get_bank_id()const
        {
            bankid_t out = bankid_coarse | (bankid_fine << 8);
            return out;
        }

        void set_bank_id(bankid_t bankid)
        {
            bankid_coarse = static_cast<uint8_t>(bankid & 0xFF);
            bankid_fine   = static_cast<uint8_t>((bankid >> 8) & 0xFF);
        }

        //Returns the part before the file extension for the original file name
        std::string get_original_file_name_no_ext()const
        {
            size_t befext = origfname.find_last_of('.');
            if ((befext != std::string::npos) && (befext != 0)) //Don't remove if the filename starts with .
                return std::string(origfname.c_str(), origfname.c_str() + befext);
            return origfname;
        }

        virtual void WriteXml(pugi::xml_node dsei)const;
        virtual void ParseXml(pugi::xml_node dsei);
    };

    /************************************************************************
        DSE_MetaDataSMDL
            Header data specific to the DSE SMD format.
    ************************************************************************/
    struct DSE_MetaDataSMDL : public DSE_MetaData
    {
        DSE_MetaDataSMDL()
            :DSE_MetaData(), mainvol(127), mainpan(64) //#TODO: Replace with constants!
        {}
        //More to be added
        //v0x402 specifics
        int8_t      mainvol;
        int8_t      mainpan;

        void WriteXml(pugi::xml_node dsei)const override;
        void ParseXml(pugi::xml_node dsei)override;
    };

    /************************************************************************
        DSE_MetaDataSWDL
    ************************************************************************/
    struct DSE_MetaDataSWDL : public DSE_MetaData
    {
        DSE_MetaDataSWDL()
            :DSE_MetaData(), nbwavislots(0), nbprgislots(0),unk17(0)
        {}

        uint16_t nbwavislots;
        uint16_t nbprgislots;
        uint16_t unk17;

        void WriteXml(pugi::xml_node dsei)const override;
        void ParseXml(pugi::xml_node dsei)override;
    };

    /************************************************************************
        DSE_MetaDataSEDL
    ************************************************************************/
    struct SEDL_Header;
    struct DSE_MetaDataSEDL : public DSE_MetaData
    {
        DSE_MetaDataSEDL()
            :DSE_MetaData(), unk5(0), unk6(0), unk7(0), unk8(0)
        {}

        void setFromHeader(const DSE::SEDL_Header& hdr);

        uint16_t unk5;
        uint16_t unk6;
        uint16_t unk7;
        uint16_t unk8;

        void WriteXml(pugi::xml_node dsei)const override;
        void ParseXml(pugi::xml_node dsei)override;
    };

//====================================================================================================
// Functions
//====================================================================================================

    /// <summary>
    /// Converts the integer value for a given sample format into a enum value for the matching format.
    /// </summary>
    /// <param name="val">The integer value to convert.</param>
    /// <returns>The matching sample format enum value, or eDSESmplFmt::invalid.</returns>
    eDSESmplFmt IntToDSESmplFmt(std::underlying_type_t<eDSESmplFmt> val);

    /// <summary>
    /// Returns a human readable string representing the sample format passed as argument.
    /// </summary>
    /// <param name="fmt">The sample format to return a string for.</param>
    /// <returns>The string matching the sample format passed as argument.</returns>
    std::string DseSmplFmtToString(eDSESmplFmt fmt);

    /// <summary>
    /// Returns the matching sample format enum value for the given string.
    /// </summary>
    /// <param name="str">The string to match to a sample format.</param>
    /// <returns>The sample format enum value that matched the string passed as argument, or eDSESmplFmt::invalid.</returns>
    eDSESmplFmt StringToDseSmplFmt(const char* str);

    /// <summary>
    /// Converts an integer value into a dse version enum value.
    /// </summary>
    /// <param name="val">The integer value to match to a dse version.</param>
    /// <returns>Returns the version that matched, or eDSEVersion::invalid.</returns>
    constexpr eDSEVersion intToDseVer(std::underlying_type_t<eDSEVersion> val)
    {
        using in_type = std::underlying_type_t<eDSEVersion>;
        if (val == static_cast<in_type>(eDSEVersion::V402))
            return eDSEVersion::V402;
        if (val == static_cast<in_type>(eDSEVersion::V415))
            return eDSEVersion::V415;
        else
            return eDSEVersion::VInvalid;
    }

    /// <summary>
    /// Returns the integer value for the given dse version enum value.
    /// </summary>
    /// <param name="ver">The dse version enum value to convert.</param>
    /// <returns>The integer value matching the dse version passed as argument.</returns>
    constexpr std::underlying_type_t<eDSEVersion> DseVerToInt(eDSEVersion ver)
    {
        return static_cast<std::underlying_type_t<eDSEVersion>>(ver);
    }

    /// <summary>
    /// Find a matching dse chunk enum value for the integer value passed as argument.
    /// </summary>
    /// <param name="value">The integer value to match to a dse chunk enum value.</param>
    /// <returns>The matching dse chunk enum value, or .eDSEChunks::invalid</returns>
    constexpr eDSEChunks IntToChunkID(std::underlying_type_t<eDSEChunks> value)
    {
        using in_type = std::underlying_type_t<eDSEChunks>;
        for (const eDSEChunks cid : DSEChunksList)
        {
            if (value == static_cast<in_type>(cid))
                return cid;
        }
        return eDSEChunks::invalid;
    }

    /// <summary>
    /// Returns the integer value matching the dse chunk header id.
    /// </summary>
    /// <param name="id">The dse chunk header id to return the underlying value for.</param>
    /// <returns>The integer value of the dse header chunk enum value passed as argument.</returns>
    constexpr std::underlying_type_t<eDSEChunks> ChunkIDToInt(eDSEChunks id)
    {
        return static_cast<std::underlying_type_t<eDSEChunks>>(id);
    }

    /// <summary>
    /// Returns the matching dse container header enum value for the given integer value.
    /// </summary>
    /// <param name="value">The integer value to match to a dse container header id enum value.</param>
    /// <returns>The dse container header id enum value matching the given integer value, or eDSEContainers::invalid.</returns>
    constexpr eDSEContainers IntToContainerMagicNum(std::underlying_type_t<eDSEContainers> value)  //Return eDSEContainers::invalid, if invalid ID !
    {
        using in_type = std::underlying_type_t<eDSEContainers>;
        for (auto cid : DSEContainerList)
        {
            if (value == static_cast<in_type>(cid))
                return cid;
        }
        return eDSEContainers::invalid;
    }


    /*
        This converts the pitch value used for samples pitch correction in SWDL files, into semitones.
        #TODO: Confirm this is correct.
    */
    //inline int16_t DSESamplePitchToSemitone( int16_t dsesmplpitch )
    //{
    //    static const int16_t NbUnitPerSemitone = 250;
    //    return ( dsesmplpitch / NbUnitPerSemitone );
    //}

    /*
        This converts the pitch value used for samples pitch correction in SWDL files, into cents(1/100th of a semitone).
        #TODO: Confirm this is correct.
    */
    //inline int16_t DSESamplePitchToCents( int16_t dsesmplpitch )
    //{
    //    //static const double NbUnitPerSemitone = 250.0;
    //    //double result = ( static_cast<double>(dsesmplpitch) / NbUnitPerSemitone ) * 100.0;
    //    //return static_cast<int16_t>( lround(result) );
    //    return dsesmplpitch;
    //    //return static_cast<int16_t>( lround( (static_cast<double>(dsesmplpitch) / 2.5) ) );
    //}

    /*
        This converts the pitch value used for pitch bend events (0xD7) into semitones.
        #TODO: Fix
    */
    //static int16_t DSEPitchBendToSemitone( int16_t dsepitchbend )
    //{
    //    assert(false); //pitch bend range can be changed in the swd !!! So this doesn't work!
    //    static const int16_t NbUnitPerSemitone = 500;
    //    return ( dsepitchbend / NbUnitPerSemitone );
    //}

    /*
        This converts the pitch value used for pitch bend events (0xD7) into cents(1/100th of a semitone).
    */
    //static int16_t DSEPitchBendToCents( int16_t dsepitchbend )
    //{
    //    
    //    static const double NbUnitPerSemitone = 500.0;
    //    double result = ( static_cast<double>(dsepitchbend) / NbUnitPerSemitone ) * 100.0;
    //    return static_cast<int16_t>( lround(result) );
    //}

    /*
    */
    //inline int8_t DseCtuneToSemitones( int8_t ctune )
    //{
    //    return ctune;

    //    //Since -7 is basically no pitch shift, we need to work around that..
    //    //if( ctune != DSEDefaultCoarseTune )
    //    //{
    //    //    if( ctune < 0 )
    //    //    {
    //    //        abs(abs(ctune) - abs(DSEDefaultCoarseTune));
    //    //        return ctune + (DSEDefaultCoarseTune * -1);
    //    //    }
    //    //    else
    //    //        return ctune + (DSEDefaultCoarseTune * -1);
    //    //}
    //    //else
    //    //    return 0;
    //}

    /*
        DSEEnveloppeDurationToMSec
            This turns an envelope's duration parameter(0-127) into miliseconds.

            The multiplier is usually 1
    */
    int32_t DSEEnveloppeDurationToMSec( int8_t param, int8_t multiplier );

    /*
        DSEEnveloppeVolumeTocB
            Turns an envelope's volume param(0-127) into centibels.
    */
    int32_t DSEEnveloppeVolumeTocB( int8_t param );

    //--------------------
    //  Lengths
    //--------------------

    /*
        DSESampleOffsetToBytes
            Sample loop position and loop lenght are stored in nb of 32 bits integers. Not in bytes or samples.
            This functions handles the conversion.
    */
    constexpr uint32_t DSESampleLoopOffsetToBytes( uint32_t loopoffset )
    {
        return (loopoffset * sizeof(uint32_t));
    }

//====================================================================================================
// Utility Functions
//====================================================================================================

    constexpr int8_t ClampInt8From0(int value)
    {
        return (int8_t)std::clamp(value, (int)0, (int)std::numeric_limits<int8_t>::max());
    }

    constexpr int8_t ClampDSEVolume(int vol)
    {
        return ClampInt8From0(vol);
    }

    constexpr int8_t ClampDSEPan(int pan)
    {
        return ClampInt8From0(pan);
    }

    constexpr int8_t ClampInt8(int value)
    {
        return (int8_t)std::clamp(value, (int)std::numeric_limits<int8_t>::min(), (int)std::numeric_limits<int8_t>::max());
    }

    constexpr uint8_t ClampUInt8(unsigned int value)
    {
        return (uint8_t)std::clamp(value, (unsigned int)std::numeric_limits<uint8_t>::min(), (unsigned int)std::numeric_limits<uint8_t>::max());
    }

    constexpr uint16_t ClampUInt16(unsigned int value)
    {
        return (uint16_t)std::clamp(value, (unsigned int)std::numeric_limits<uint16_t>::min(), (unsigned int)std::numeric_limits<uint16_t>::max());
    }

    constexpr int16_t ClampInt16(int value)
    {
        return (int16_t)std::clamp(value, (int)std::numeric_limits<int16_t>::min(), (int)std::numeric_limits<int16_t>::max());
    }

    /************************************************************************
        DSE_ChunkIDLookup
            This singleton's "Find" static method returns the first chunk id 
            whose's highest byte matches the specified byte.
    ************************************************************************/
    class DSE_ChunkIDLookup
    {
    public:
        static std::vector<eDSEChunks> Find( uint8_t highbyte )
        {
            static DSE_ChunkIDLookup s_instance; //creates it when first called
            auto lambdaFId = [&highbyte]( const std::pair<uint8_t,eDSEChunks>& val )->bool
            { 
                return (val.first == highbyte); 
            };

            std::vector<eDSEChunks> possiblematches;

            for( auto itfound = s_instance.m_lutbl.find(highbyte);  //Search once, if no match will not loop once.
                 itfound != s_instance.m_lutbl.end(); 
                 itfound = std::find_if( ++itfound, s_instance.m_lutbl.end(), lambdaFId ) ) //If we had a match search again, one slot after our last match
            {
                if( itfound->second != eDSEChunks::invalid )
                    possiblematches.push_back(itfound->second);
            }

            return possiblematches;
        }

    private:
        //Build the quick lookup table
        DSE_ChunkIDLookup()
        {
            for( eDSEChunks id : DSEChunksList )
                m_lutbl.insert( std::make_pair( static_cast<uint8_t>((static_cast<uint32_t>(id) >> 24) & 0xFF) , id ) ); //Isolate highest byte
        }

        //No copy, no move
        DSE_ChunkIDLookup( const DSE_ChunkIDLookup & );
        DSE_ChunkIDLookup( DSE_ChunkIDLookup && );
        DSE_ChunkIDLookup & operator=( const DSE_ChunkIDLookup & );
        DSE_ChunkIDLookup & operator=( DSE_ChunkIDLookup && );

        std::map<uint8_t, eDSEChunks> m_lutbl;
    };

    /************************************************************************
        FindNextChunk
            Find the start of the next chunk that has the specified chunk id.

            If the chunk is not found, the function returns "end".

            NOTE: "beg" must be aligned on 4 bytes!
    ************************************************************************/
    template<class _init>
        _init FindNextChunk( _init beg, _init end, eDSEChunks chnkid )
    {
        //search
        while( beg != end ) 
        {
            //check if we possibly are at the beginning of a chunk, looking for its highest byte.
            std::vector<eDSEChunks> possibleid = DSE_ChunkIDLookup::Find( *beg ); 
            size_t             skipsize = 4; //Default byte skip size on each loop (The NDS makes 4 bytes aligned reads)

            //Check all found results
            for( auto & potential : possibleid )
            {
                //Check if its really the chunk's header start, or just a coincidence
                uint32_t actualid = 0;
                utils::ReadIntFromBytes(actualid, beg, end, false); //Make sure we're not incrementing beg!!!
                

                if( actualid == static_cast<uint32_t>(chnkid) ) //Check if we match the chunk we're looking for
                    return beg;
                else if( actualid == static_cast<uint32_t>(potential) ) //If it actually matches another chunk's id
                {
                    _init backup = beg;

                    //Read the chunk's size and skip if possible
                    std::advance( beg, ChunkHeader::OffsetDataLen );
                    uint32_t chnksz = utils::ReadIntFromBytes<uint32_t>(beg, end);

                    if( chnksz != DSE::SpecialChunkLen ) //Some chunks have an invalid length that is equal to this value.
                    {
                        //Then attempt to skip
                        //try
                        //{
                            skipsize = chnksz;  //We have to do this like so, because some chunks may use bogus sizes 
                                                // for some mindblowingly odd reasons.. It shouldn't happen too often though..
                        //}
                        //catch(std::exception &)
                        //{
                        //    beg = backup; //Restore iterator to last valid state if fails
                        //}
                    }
                    else
                        skipsize = 0; //otherwise, just continue on, without further incrementing the iterator

                    break; //After all this we know its an actual chunk, just kick us out of the loop, as other matches don't matter anymore!
                }
            }

            //Skip the required ammount of bytes
            if( skipsize != 4 )
                std::advance( beg, skipsize ); //Advance is much faster, and here we're certain we're not supposed to go out of bounds.
            else
                for( int cnt = 0; cnt < 4 && beg != end; ++cnt, ++beg ); //SMDL files chunk headers are always 4 bytes aligned
        }

        //Return Result
        return beg;
    }


    /*
        FindEndChunk
        

            Use this to locate the proper end chunk depending on the DSE container type!
    */
    template<class _init>
        _init FindEndChunk( _init beg, _init end, eDSEContainers cnty )
    {
        switch(cnty)
        {
            case eDSEContainers::smdl:
            {
                beg = FindNextChunk( beg, end, eDSEChunks::eoc );
                break;
            }
            case eDSEContainers::swdl:
            case eDSEContainers::sedl:
            {
                beg = FindNextChunk( beg, end, eDSEChunks::eod );
                break;
            }
            case eDSEContainers::sadl:
            {
                std::cerr << "<!>- Error : FindEndChunk() : SADL doesn't have a end chunk!\n";
                assert(false);
            }
            default:
            {
                std::cerr << "<!>- Error : FindEndChunk() : Invalid DSE container type!\n";
                assert(false);
            }
        };

        return beg;
    }

};

//
//Stream operators
//
std::ostream& operator<<(std::ostream& strm, const DSE::eDSEContainers cnty);
std::stringstream& operator<<(std::stringstream& ss, const DSE::DateTime& obj);
std::ostream& operator<<(std::ostream& os,   const DSE::DateTime&      obj);
std::istream& operator>>(std::istream& is,   DSE::DateTime&            obj);
std::ostream& operator<<(std::ostream& strm, const DSE::KeyGroup&      other);
std::ostream& operator<<(std::ostream& strm, const DSE::ProgramInfo&   other);
std::ostream& operator<<(std::ostream& strm, const DSE::WavInfo&       other);

#endif