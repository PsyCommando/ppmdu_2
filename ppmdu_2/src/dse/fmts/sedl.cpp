#include <dse/fmts/sedl.hpp>
#include <dse/fmts/dse_fmt_common.hpp>

#include <fstream>
using namespace std;

namespace DSE
{
    //
    //
    //
    template<class _InIt>
    class SEDL_Parser
    {
    public:
        using iterator_t = _InIt;

        SEDL_Parser(_InIt beg, _InIt end)noexcept :m_itbeg(beg), m_itend(end), m_version(eDSEVersion::VInvalid){}

        operator DSE::SoundEffectSequences()
        {
            SEDL_Header hdr = ParseHeader(m_itbeg);
            if (hdr.version != static_cast<uint16_t>(eDSEVersion::V415))
                throw std::runtime_error("Unsupported SEDL header version : " + std::to_string(hdr.version) + "!");
            m_version = (eDSEVersion)hdr.version;
            
            //Grab the seq header, so we get a list of all the sequences in the file
            iterator_t ithdrend       = std::next(m_itbeg, SEDL_Header::size());
            iterator_t itafterseqinfo = std::next(ithdrend, seq_chunk_header::OffsetTableBeg);
            seq_chunk_header seqchunk = ParseSeqChunk(ithdrend);

            if (seqchunk.magicn != static_cast<uint32_t>(DSE::eDSEChunks::seq))
            {
                assert(false); //TODO: Implement handling for games that don't use the seq chunk
            }

            //Grab the sequences. Offsets in the table begin at the start of the offset table
            std::vector<MusicSequence> sequences = ParseSequences(itafterseqinfo, seqchunk);

            //Skip past the seqchunk to grab the mcrl and bnkl chunks
            iterator_t itmcrlbeg = std::next(itafterseqinfo, seqchunk.chunklen);
            mcrl_chunk_header mcrl_hdr;
            mcrl_hdr.ReadFromContainer(itmcrlbeg, m_itend);
            McrlChunkContents mcrlcnt = ParseMcrlChunk(itmcrlbeg, mcrl_hdr);

            //Go to the bnkl chunk
            iterator_t itbnklbeg = std::next(itmcrlbeg, mcrl_hdr.chunklen + 16);
            bnkl_chunk_header bnkl_hdr;
            bnkl_hdr.ReadFromContainer(itbnklbeg, m_itend);
            BnklChunkContents bnklcnt = ParseBnklChunk(itbnklbeg, bnkl_hdr);

            DSE::DSE_MetaDataSEDL metadata;
            metadata.setFromHeader(hdr);
            return DSE::SoundEffectSequences(std::move(sequences), std::move(mcrlcnt), std::move(bnklcnt), std::move(metadata));
        }

        SEDL_Header ParseHeader(iterator_t itread)
        {
            SEDL_Header hdr;
            hdr.ReadFromContainer(itread, m_itend);
            return hdr;
        }

        seq_chunk_header ParseSeqChunk(iterator_t itread)
        {
            seq_chunk_header seqhdr;
            seqhdr.ReadFromContainer(itread, m_itend);
            return seqhdr;
        }

        std::vector<MusicSequence> ParseSequences(iterator_t itread, const seq_chunk_header& seqhdr)
        {
            std::vector<MusicSequence> sequences;
            for (uint16_t seqoffsets : seqhdr.seqoffsets)
            {
                iterator_t itseqtbl = std::next(itread, seqoffsets);
                sequences.push_back(ParseASequence(itseqtbl));
            }
            return sequences;
        }

        MusicSequence ParseASequence(iterator_t itread)
        {
            seqinfo_table seqinfo;
            std::vector<MusicTrack> trks;

            if (m_version == eDSEVersion::V415)
            {
                SeqInfoChunk_v415 seq415;
                itread = seq415.ReadFromContainer(itread, m_itend);
                seqinfo = seq415;
            }
            else if (m_version == eDSEVersion::V402)
            {
                SeqInfoChunk_v402 seq402;
                itread = seq402.ReadFromContainer(itread, m_itend);
                seqinfo = seq402;
            }
            else
                assert(false);
            trks.reserve(seqinfo.nbtrks);

            iterator_t iteoc = DSE::FindNextChunk(itread, m_itend, DSE::eDSEChunks::eoc); //We stop at either m_itend or the next eoc chunk
            iterator_t ittrk = std::next(itread, seqinfo.nextoff);

            try
            {
                for (; ittrk != iteoc; ittrk = DSE::FindNextChunk(ittrk, iteoc, DSE::eDSEChunks::trk))
                {
                    trks.push_back(ParseTrack(ittrk, iteoc));
                }
            }
            catch (const std::exception& e)
            {
                stringstream sstr;
                sstr << e.what() << " Caught exception while parsing SEDL track # " << trks.size() << "! ";
                throw std::runtime_error(sstr.str());
            }

            return MusicSequence(std::move(trks), std::move(seqinfo));
        }

        MusicTrack ParseTrack(iterator_t& itread, iterator_t iteoc)
        {
            DSE::ChunkHeader      hdr;
            hdr.ReadFromContainer(itread, iteoc); //Don't increment itread
            auto itend = itread + (hdr.datlen + DSE::ChunkHeader::size());
            auto itpreread = itread;
            itread = itend; //move it past the chunk already

            //And skip padding bytes
            for (; itread != iteoc && (*itread) == static_cast<uint8_t>(eTrkEventCodes::EndOfTrack); ++itread);

            auto parsed = DSE::ParseTrkChunk(itpreread, itend);

            MusicTrack mtrk;
            mtrk.SetMidiChannel(parsed.second.chanid);
            mtrk.setEvents(std::move(parsed.first));

            return mtrk;
        }

        McrlChunkContents ParseMcrlChunk(iterator_t itread, const mcrl_chunk_header& hdr)
        {
            McrlChunkContents cnt;
            for (uint16_t ptr : hdr.ptrtbl)
            {
                iterator_t itentry = std::next(itread, ptr);
                McrlChunkContents::mcrlentry entry;
                entry.Read(itentry, m_itend);
                cnt.AddEntry(entry);
            }
            return cnt;
        }

        BnklChunkContents ParseBnklChunk(iterator_t itread, const bnkl_chunk_header & hdr)
        {
            BnklChunkContents cnt;
            iterator_t itchunkend = std::next(itread, hdr.chunklen);
            for (uint16_t ptr : hdr.ptrtbl)
            {
                iterator_t itentry = std::next(itread, ptr);
                BnklChunkContents::bnklentry entry;
                entry.Read(itentry, itchunkend);
                cnt.AddEntry(entry);
            }
            return cnt;
        }

    private:
        iterator_t m_itbeg;
        iterator_t m_itend;
        eDSEVersion m_version;
    };

    //
    //
    //
    class SEDL_Writer
    {
    public:
        SEDL_Writer(const DSE::SoundEffectSequences& seqref)noexcept :m_seqref(seqref) {}

        void operator()(std::ostreambuf_iterator<char> itout)
        {
            //Keep track of where the sequences start and their seq_info table
        }

    private:
        const DSE::SoundEffectSequences& m_seqref;
    };

    //
    // Functions
    //
    DSE::SoundEffectSequences ParseSEDL(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend)
    {
        return SEDL_Parser<decltype(itbeg)>(itbeg, itend);
    }

    void WriteSEDL(const std::string& path, const DSE::SoundEffectSequences& seq)
    {
        std::ofstream outf(path, std::ios::out | std::ios::binary);

        if (!outf.is_open() || outf.bad())
            throw std::runtime_error("WriteSMDL(): Couldn't open output file " + path);

        SEDL_Writer(seq).operator()(std::ostreambuf_iterator<char>(outf));
    }
};

#ifdef USE_PPMDU_CONTENT_TYPE_ANALYSER
    #include <types/content_type_analyser.hpp>

    namespace filetypes
    {
        const ContentTy CnTy_SEDL {"sedl"}; //Content ID db handle

    //========================================================================================================
    //  sedl_rule
    //========================================================================================================
        /*
            sedl_rule
                Rule for identifying a SMDL file. With the ContentTypeHandler!
        */
        class sedl_rule : public IContentHandlingRule
        {
        public:
            sedl_rule(){}
            ~sedl_rule(){}

            //Returns the value from the content type enum to represent what this container contains!
            virtual cnt_t getContentType()const
            {
                return filetypes::CnTy_SEDL;
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
                DSE::SEDL_Header headr;
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
                                   const std::string & filext)
            {
                return (utils::ReadIntFromBytes<uint32_t>(itdatabeg, itdataend, false) == DSE::SEDL_MagicNumber);
            }

        private:
            cntRID_t m_myID;
        };

    //========================================================================================================
    //  sedl_rule_registrator
    //========================================================================================================
        /*
            sedl_rule_registrator
                A small singleton that has for only task to register the sedl_rule!
        */
        template<> RuleRegistrator<sedl_rule> RuleRegistrator<sedl_rule>::s_instance;
    };
#endif