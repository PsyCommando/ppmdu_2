#include <dse/fmts/smdl.hpp>
#include <dse/dse_sequence.hpp>
#include <dse/fmts/dse_fmt_common.hpp>

#include <utils/library_wide.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iterator>
#include <map>
#include <cassert>
using namespace std;

namespace DSE
{
    class SongChunk_v402;
    class SongChunk_v415;
};
std::ostream& operator<<(std::ostream& os, const DSE::SongChunk_v415& sd);
std::ostream& operator<<(std::ostream& os, const DSE::SongChunk_v402& sd);


namespace DSE
{
//====================================================================================================
// Constants
//====================================================================================================
    static const uint32_t EocParam1Default = TrkParam1Default;  //The default value for the parameter 1 value in the eoc chunk header!
    static const uint32_t EocParam2Default = TrkParam2Default;  //The default value for the parameter 2 value in the eoc chunk header!

//====================================================================================================
// Other Definitions
//====================================================================================================

    /************************************************************************
        SongChunk
            The raw song chunk.
            For some reasons, most of the data in this chunk rarely ever 
            changes in-between games or files.. Only the nb of channels and
            tracks does..
    ************************************************************************/
    class SongChunk_v415
    {
    public:
        static constexpr size_t size() { return 16 + SeqInfoChunk_v415::size(); }
        //Default Values
        static const uint32_t DefUnk1       = 0x1000000;
        static const uint32_t DefUnk2       = 0xFF10;
        static const uint32_t DefUnk3       = 0xFFFFFFB0;

        uint32_t          label   = (uint32_t)eDSEChunks::song;
        uint32_t          unk1    = DefUnk1;
        uint32_t          unk2    = DefUnk2;
        uint32_t          unk3    = DefUnk3;
        SeqInfoChunk_v415 seqinfo {}; //A song chunk is always followed with a seqinfo chunk

        //
        template<class _outit> _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( static_cast<uint32_t>(eDSEChunks::song), itwriteto, false ); //Force this, to avoid bad surprises
            itwriteto = utils::WriteIntToBytes( unk1,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk2,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk3,    itwriteto );
            itwriteto = seqinfo.WriteToContainer(itwriteto);
            return itwriteto;
        }

        //
        template<class _init> _init ReadFromContainer( _init itReadfrom, _init itPastEnd )
        {
            itReadfrom = utils::ReadIntFromBytes( label,    itReadfrom, itPastEnd, false ); //iterator is incremented
            itReadfrom = utils::ReadIntFromBytes( unk1,     itReadfrom, itPastEnd ); 
            itReadfrom = utils::ReadIntFromBytes( unk2,     itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( unk3,     itReadfrom, itPastEnd );
            itReadfrom = seqinfo.ReadFromContainer(itReadfrom, itPastEnd);
            return itReadfrom;
        }
    };

    /*****************************************************
        SongChunk_v402
            For DSE Version 0x402
    *****************************************************/
    class SongChunk_v402
    {
    public:
        static constexpr size_t size() { return 16 + SeqInfoChunk_v402::size(); }

        //Default Values
        static const uint32_t DefUnk1 = 0x1000000;
        static const uint32_t DefUnk2 = 0xFF10;
        static const uint32_t DefUnk3 = 0xFFFFFFB0;

        uint32_t label   = static_cast<uint32_t>(eDSEChunks::song);
        uint32_t unk1    = DefUnk1;
        uint32_t unk2    = DefUnk2;
        uint32_t unk3    = DefUnk3;
        SeqInfoChunk_v402 seqinfo{};

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( static_cast<uint32_t>(eDSEChunks::song), itwriteto, false ); //Force this, to avoid bad surprises
            itwriteto = utils::WriteIntToBytes( unk1,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk2,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk3,    itwriteto );
            itwriteto = seqinfo.WriteToContainer(itwriteto);
            return itwriteto;
        }

        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itPastEnd )
        {
            itReadfrom = utils::ReadIntFromBytes( label,   itReadfrom, itPastEnd, false ); //Force this, to avoid bad surprises
            itReadfrom = utils::ReadIntFromBytes( unk1,    itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( unk2,    itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( unk3,    itReadfrom, itPastEnd );
            itReadfrom = seqinfo.ReadFromContainer(itReadfrom, itPastEnd);
            return itReadfrom;
        }

    };

//====================================================================================================
// SMDL_Parser
//====================================================================================================

    /*
        SMDL_Parser
            Takes a random access iterator as template param.
    */
    template<class _rait = vector<uint8_t>::const_iterator >
        class SMDL_Parser
    {
    public:
        typedef _rait rd_iterator_t;

        SMDL_Parser( const vector<uint8_t> & filedata )
            :m_itbeg(filedata.begin()), m_itend(filedata.end())
        {}

        SMDL_Parser( _rait itbeg, _rait itend )
            :m_itbeg(itbeg), m_itend(itend)
        {}

        operator MusicSequence()
        {
            m_song = seqinfo_table{};
            if( utils::LibWide().isLogOn() )
                clog << "=== Parsing SMDL ===\n";
            //Set our iterator
            m_itread = m_itbeg;

            //#FIXME: It might have been easier to just check for trk chunks and stop when none are left? We'd save a lot of iteration!!
            m_itEoC  = DSE::FindNextChunk( m_itbeg, m_itend, DSE::eDSEChunks::eoc ); //Our end is either the eoc chunk, or the vector's end

            //Get the headers
            ParseHeader();
            ParseSong();
            DSE::DSE_MetaDataSMDL meta( MakeMeta() );

            //Check version
            if( m_hdr.version == static_cast<uint16_t>(eDSEVersion::V415) )
            {
                //Parse tracks and return
                return MusicSequence( ParseAllTracks(), std::move(meta), std::move(m_song));
            }
            else if( m_hdr.version == static_cast<uint16_t>(eDSEVersion::V402) )
            {
                //Parse tracks and return
                return MusicSequence( ParseAllTracks(), std::move(meta), std::move(m_song));
            }
            else
            {
#ifdef _DEBUG
                cerr << "SMDL_Parser::operator MusicSequence() : Unsupported DSE version!!";
                assert(false);
#endif
                throw runtime_error( "SMDL_Parser::operator MusicSequence() : Unsupported DSE version!!" );
            }
        }

    private:

        //Parse the SMDL header
        inline void ParseHeader()
        {
            m_itread = m_hdr.ReadFromContainer( m_itread, m_itend );
            if( utils::LibWide().isLogOn() )
                clog << m_hdr;
        }

        //Parse the song chunk
        void ParseSong()
        {
            if( m_hdr.version == static_cast<uint16_t>(eDSEVersion::V402) )
            {
                SongChunk_v402 schnk;
                m_itread = schnk.ReadFromContainer( m_itread, m_itend );
                if( utils::LibWide().isLogOn() )
                    clog << schnk;
                m_song = schnk.seqinfo;
            }
            else if( m_hdr.version == static_cast<uint16_t>(eDSEVersion::V415) )
            {
                SongChunk_v415 schnk;
                m_itread = schnk.ReadFromContainer( m_itread, m_itend );
                if( utils::LibWide().isLogOn() )
                    clog << schnk;
                m_song = schnk.seqinfo;
            }
            else
            {
                stringstream sstr;
                sstr << "SMDL_Parser::operator MusicSequence() : DSE version 0x" <<hex <<m_hdr.version <<" is unsupported/unknown at the moment!";
                throw runtime_error( sstr.str() );
            }
        }

        DSE::DSE_MetaDataSMDL MakeMeta()
        {
            //Build Meta-info
            DSE::DSE_MetaDataSMDL meta;
            meta.createtime.year    = m_hdr.year;
            meta.createtime.month   = m_hdr.month;
            meta.createtime.day     = m_hdr.day;
            meta.createtime.hour    = m_hdr.hour;
            meta.createtime.minute  = m_hdr.minute;
            meta.createtime.second  = m_hdr.second;
            meta.createtime.centsec = m_hdr.centisec;
            meta.fname              = string( m_hdr.fname.data());
            meta.bankid_coarse      = m_hdr.bankid_low;
            meta.bankid_fine        = m_hdr.bankid_high;
            meta.origversion        = intToDseVer( m_hdr.version );
            return meta;
        }

        std::vector<MusicTrack> ParseAllTracks()
        {
            vector<MusicTrack> tracks;
            tracks.reserve( m_song.nbtrks );

            try
            {
                size_t cnttrk = 0;
                if( utils::LibWide().isLogOn() )
                    clog << "\t--- Parsing Tracks ---\n";
                while( m_itread != m_itEoC )
                {
                    //#1 - Find next track chunk
                    m_itread = DSE::FindNextChunk( m_itread, m_itEoC, DSE::eDSEChunks::trk );
                    if( m_itread != m_itEoC )
                    {
                        //Parse Track
                        tracks.push_back( ParseTrack() );
                        if( utils::LibWide().isLogOn() )
                            clog << "\t- Track " <<cnttrk <<", parsed " << tracks.back().size() <<" event(s)\n";
                        ++cnttrk;
                    }
                }

                if( utils::LibWide().isLogOn() )
                    clog << "\n\n";
            }
            catch( std::runtime_error & e )
            {
                stringstream sstr;
                sstr << e.what() << " Caught exception while parsing track # " <<tracks.size() <<"! ";
                throw std::runtime_error( sstr.str() );
            }

            return tracks;
        }

        MusicTrack ParseTrack()
        {
            DSE::ChunkHeader      hdr;
            hdr.ReadFromContainer(m_itread, m_itend); //Don't increment itread
            auto itend     = m_itread + (hdr.datlen + DSE::ChunkHeader::size());
            auto itpreread = m_itread;
            m_itread = itend; //move it past the chunk already

            //And skip padding bytes
            for( ;m_itread != m_itEoC && (*m_itread) == static_cast<uint8_t>(eTrkEventCodes::EndOfTrack); ++m_itread );

            auto parsed = DSE::ParseTrkChunk(itpreread, itend);

            MusicTrack mtrk;
            mtrk.SetMidiChannel( parsed.second.chanid );
            mtrk.getEvents() = std::move(parsed.first);

            return mtrk;
        }

    private:
        rd_iterator_t                   m_itbeg;
        rd_iterator_t                   m_itend;
        rd_iterator_t                   m_itread;
        rd_iterator_t                   m_itEoC;    //Pos of the "end of chunk" chunk
        SMDL_Header                     m_hdr;
        seqinfo_table                   m_song;
    };

//====================================================================================================
// SMDL_Writer
//====================================================================================================

    template<class _TargetTy>
        class SMDL_Writer;


    template<>
        class SMDL_Writer<std::ofstream>
    {
    public:
        typedef std::ofstream cnty;

        SMDL_Writer( cnty & tgtcnt, const MusicSequence & srcseq, eDSEVersion dseVersion = eDSEVersion::VDef )
            :m_tgtcn(tgtcnt), m_src(srcseq), m_version(dseVersion)
        {
        }

        void operator()()
        {
            std::ostreambuf_iterator<char> itout(m_tgtcn); 
            std::set<int>                  existingchan;        //Keep track of the nb of unique channels used by the tracks
            size_t                         nbwritten    = 0;

            //Reserve Header
            itout = std::fill_n( itout, SMDL_Header::size(), 0 );
            nbwritten += SMDL_Header::size();

            //Reserve Song chunk
            if( m_version == eDSEVersion::V402 )
            {
                itout = std::fill_n( itout, SongChunk_v402::size(), 0);
                nbwritten += SongChunk_v402::size();
            }
            else if( m_version == eDSEVersion::V415 )
            {
                itout = std::fill_n( itout, SongChunk_v415::size(), 0); //Size includes padding
                nbwritten += SongChunk_v415::size();
            }
            else
                throw std::runtime_error( "SMDL_Writer::operator()(): Invalid DSE version supplied!!" );

            if( utils::LibWide().isLogOn() )
            {
                clog << "-------------------------\n"
                     << "Writing SMDL\n"
                     << "-------------------------\n"
                     << m_src.printinfo() 
                     <<"\n"
                     ;
            }

            //Write tracks
            for( uint8_t i = 0; i < m_src.getNbTracks(); ++i )
            {
                if( m_src[i].empty() )
                    continue; //ignore empty tracks

                const DSE::MusicTrack & atrk = m_src[i];
                TrkPreamble preamble;
                preamble.trkid  = i;
                preamble.chanid = atrk.GetMidiChannel();
                preamble.unk1   = 0;
                preamble.unk2   = 0;
                uint32_t lenmod = WriteTrkChunk( itout, preamble, atrk.begin(), atrk.end(), atrk.size() );
                nbwritten += lenmod;

                //Write padding
                nbwritten += utils::AppendPaddingBytes(itout, nbwritten, 4, static_cast<uint8_t>(eTrkEventCodes::EndOfTrack));
                existingchan.insert( preamble.chanid );
            }

            //Write end chunk
            DSE::ChunkHeader eoc;
            eoc.label  = static_cast<uint32_t>(eDSEChunks::eoc);
            eoc.datlen = 0;
            eoc.param1 = EocParam1Default;
            eoc.param2 = EocParam2Default;
            itout      = eoc.WriteToContainer( itout );

            //Go back to write the header and song chunk!
            size_t flen = static_cast<size_t>(m_tgtcn.tellp());
            m_tgtcn.seekp(0); 
            itout = std::ostreambuf_iterator<char>(m_tgtcn);
            WriteHeader( itout, existingchan, flen );
        }

    private:

        void WriteHeader( std::ostreambuf_iterator<char> & itout, const std::set<int> & existingchan, size_t filelen )
        {
            //Header
            SMDL_Header smdhdr; 
            smdhdr.unk7        = 0;
            smdhdr.flen        = filelen;
            smdhdr.version     = DseVerToInt(m_version);
            smdhdr.bankid_low  = m_src.metadata().bankid_coarse;
            smdhdr.bankid_high = m_src.metadata().bankid_fine;
            smdhdr.unk3        = 0;
            smdhdr.unk4        = 0;
            smdhdr.year        = m_src.metadata().createtime.year;
            smdhdr.month       = m_src.metadata().createtime.month;
            smdhdr.day         = m_src.metadata().createtime.day;
            smdhdr.hour        = m_src.metadata().createtime.hour;
            smdhdr.minute      = m_src.metadata().createtime.minute;
            smdhdr.second      = m_src.metadata().createtime.second;
            smdhdr.centisec    = m_src.metadata().createtime.centsec;
            std::copy(m_src.metadata().fname.c_str(), m_src.metadata().fname.c_str() + std::min(m_src.metadata().fname.size(), smdhdr.fname.size() - 1), smdhdr.fname.data());
            smdhdr.unk5        = SMDL_Header::DefUnk5;
            smdhdr.unk6        = SMDL_Header::DefUnk6;
            smdhdr.unk8        = SMDL_Header::DefUnk8;
            smdhdr.unk9        = SMDL_Header::DefUnk9;
            smdhdr.WriteToContainer( itout ); //The correct magic number for SMDL is forced on write, whatever the value in smdhdr.magicn is.

            //Song Chunk
            if( m_version == eDSEVersion::V402 )
            {
                SongChunk_v402 songchnk{};
                songchnk.seqinfo         = m_src.seqinfo();
                auto highestchan         = std::max_element(existingchan.begin(), existingchan.end());
                songchnk.seqinfo.nbtrks  = static_cast<uint8_t>(m_src.getNbTracks());
                songchnk.seqinfo.nbchans = (*highestchan + 1);
                songchnk.seqinfo.mainvol = m_src.metadata().mainvol;
                songchnk.seqinfo.mainpan = m_src.metadata().mainpan;
                itout = songchnk.WriteToContainer(itout);
            }
            else if( m_version == eDSEVersion::V415 )
            {
                SongChunk_v415 songchnk{};
                songchnk.seqinfo         = m_src.seqinfo();
                auto highestchan         = std::max_element(existingchan.begin(), existingchan.end());
                songchnk.seqinfo.nbtrks  = static_cast<uint8_t>(m_src.getNbTracks());
                songchnk.seqinfo.nbchans = *highestchan + 1;
                itout = songchnk.WriteToContainer(itout);
            }
        }

    private:
        cnty                & m_tgtcn;
        const MusicSequence & m_src;
        eDSEVersion           m_version;
    };


//====================================================================================================
// Functions
//====================================================================================================

    MusicSequence ParseSMDL( const std::string & file )
    {
        if( utils::LibWide().isLogOn() )
        {
            clog << "================================================================================\n"
                 << "Parsing SMDL " <<file << "\n"
                 << "================================================================================\n";
        }
        return SMDL_Parser<>( utils::io::ReadFileToByteVector(file) );
    }

    MusicSequence ParseSMDL( std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend )
    {
        return SMDL_Parser<>( itbeg, itend );
    }

    void WriteSMDL( const std::string & file, const MusicSequence & seq )
    {
        std::ofstream outf(file, std::ios::out | std::ios::binary );

        if( !outf.is_open() || outf.bad() )
            throw std::runtime_error( "WriteSMDL(): Couldn't open output file " + file );

        SMDL_Writer<std::ofstream>(outf, seq)();
    }
};

//
// Stream Operator Def
//
/***********************************************************************
    operator<< SongChunk_v415
***********************************************************************/
std::ostream& operator<<(std::ostream& os, const DSE::SongChunk_v415& sd)
{
    os << "\t-- Song Chunk v0x415 --\n"
        << showbase << hex << uppercase
        << "\tLabel        : " << sd.label << "\n"
        << "\tUnk1         : " << sd.unk1 << "\n"
        << "\tUnk2         : " << sd.unk2 << "\n"
        << "\tUnk3         : " << sd.unk3 << "\n"
        << dec << nouppercase << noshowbase
        << sd.seqinfo
        << "\n"
        ;
    return os;
}

/***********************************************************************
    operator<< SongChunk_v402
***********************************************************************/
std::ostream& operator<<(std::ostream& os, const DSE::SongChunk_v402& sd)
{
    os << "\t-- Song Chunk v0x402 --\n"
        << showbase << hex << uppercase
        << "\tLabel        : " << sd.label << "\n"
        << "\tUnk1         : " << sd.unk1 << "\n"
        << "\tUnk2         : " << sd.unk2 << "\n"
        << "\tUnk3         : " << sd.unk3 << "\n"
        << dec << nouppercase << noshowbase
        << sd.seqinfo
        << "\n"
        ;
    return os;
}

/***********************************************************************
    operator<< SMDL_Header
***********************************************************************/
std::ostream& operator<<(std::ostream& os, const DSE::SMDL_Header& hdr)
{
    os << "\t-- SMDL Header --\n"
        << showbase
        << hex << uppercase
        << "\tmagicn       : " << hdr.magicn << "\n"
        << "\tUnk7         : " << hdr.unk7 << "\n"
        << dec << nouppercase
        << "\tFile lenght  : " << hdr.flen << " bytes\n"
        << hex << uppercase
        << "\tVersion      : " << hdr.version << "\n"
        << "\tBank ID Low  : " << static_cast<short>(hdr.bankid_low) << "\n"
        << "\tBank ID High : " << static_cast<short>(hdr.bankid_high) << "\n"
        << "\tUnk3         : " << hdr.unk3 << "\n"
        << "\tUnk4         : " << hdr.unk4 << "\n"
        << dec << nouppercase
        << "\tYear         : " << hdr.year << "\n"
        << "\tMonth        : " << static_cast<short>(hdr.month) << "\n"
        << "\tDay          : " << static_cast<short>(hdr.day) << "\n"
        << "\tHour         : " << static_cast<short>(hdr.hour) << "\n"
        << "\tMinute       : " << static_cast<short>(hdr.minute) << "\n"
        << "\tSecond       : " << static_cast<short>(hdr.second) << "\n"
        << "\tCentisec     : " << static_cast<short>(hdr.centisec) << "\n"
        << "\tFile Name    : " << string(begin(hdr.fname), end(hdr.fname)) << "\n"
        << hex << uppercase
        << "\tUnk5         : " << hdr.unk5 << "\n"
        << "\tUnk6         : " << hdr.unk6 << "\n"
        << "\tUnk8         : " << hdr.unk8 << "\n"
        << "\tUnk9         : " << hdr.unk9 << "\n"
        << dec << nouppercase
        << noshowbase
        << "\n"
        ;
    return os;
}


//ppmdu's type analysis system
#ifdef USE_PPMDU_CONTENT_TYPE_ANALYSER
    #include <types/content_type_analyser.hpp>

    //################################### Filetypes Namespace Definitions ###################################
    namespace filetypes
    {
        const ContentTy CnTy_SMDL {"smdl"}; //Content ID db handle

    //========================================================================================================
    //  smdl_rule
    //========================================================================================================
        /*
            smdl_rule
                Rule for identifying a SMDL file. With the ContentTypeHandler!
        */
        class smdl_rule : public IContentHandlingRule
        {
        public:
            smdl_rule():m_myID(0){}
            ~smdl_rule(){}

            //Returns the value from the content type enum to represent what this container contains!
            virtual cnt_t getContentType()const
            {
                return filetypes::CnTy_SMDL;
            }

            //Returns an ID number identifying the rule. Its not the index in the storage array,
            // because rules can me added and removed during exec. Thus the need for unique IDs.
            //IDs are assigned on registration of the rule by the handler.
            virtual cntRID_t getRuleID()const                          { return m_myID; }
            virtual void                      setRuleID( cntRID_t id ) { m_myID = id; }

            //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
            //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
            //virtual ContentBlock Analyse( vector<uint8_t>::const_iterator   itdatabeg, 
            //                              vector<uint8_t>::const_iterator   itdataend );
            virtual ContentBlock Analyse( const analysis_parameter & parameters )
            {
                DSE::SMDL_Header headr;
                ContentBlock cb;

                //Read the header
                headr.ReadFromContainer( parameters._itdatabeg, parameters._itdataend );

                //build our content block info 
                cb._startoffset          = 0;
                cb._endoffset            = headr.flen;
                cb._rule_id_that_matched = getRuleID();
                cb._type                 = getContentType();

                return cb;
            }

            //This method is a quick boolean test to determine quickly if this content handling
            // rule matches, without in-depth analysis.
            virtual bool isMatch(  vector<uint8_t>::const_iterator   itdatabeg, 
                                   vector<uint8_t>::const_iterator   itdataend,
                                   const std::string               & filext)
            {
                return (utils::ReadIntFromBytes<uint32_t>(itdatabeg, itdataend, false) == DSE::SMDL_MagicNumber);
            }

        private:
            cntRID_t m_myID;
        };

    //========================================================================================================
    //  smdl_rule_registrator
    //========================================================================================================
        /*
            smdl_rule_registrator
                A small singleton that has for only task to register the smdl_rule!
        */
        template<> RuleRegistrator<smdl_rule> RuleRegistrator<smdl_rule>::s_instance;

    };
#endif