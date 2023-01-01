#ifndef DSE_CONTAINERS_HPP
#define DSE_CONTAINERS_HPP
/*
dse_containers.hpp
2015/08/23
psycommando@gmail.com
Description: Several container objects for holding the content of loaded DSE files!
*/
#include <dse/dse_common.hpp>
#include <dse/dse_sequence.hpp>
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <future>
#include <set>

//# TODO: Move implementation into CPP !!

namespace DSE
{
    /*****************************************************************************************
        SampleBank
            This class is used to maintain references to sample data. 
            The samples in this are refered to by entries in a SampleMap instance.
    *****************************************************************************************/
    class SampleBank
    {
    public:
        typedef std::unique_ptr<std::vector<uint8_t>> smpl_t;            //Pointer to a vector of raw sample data
        typedef std::unique_ptr<DSE::WavInfo>         wavinfoptr_t;      //Pointer to wavinfo

        struct SampleBlock
        {
            wavinfoptr_t pinfo_;
            smpl_t       pdata_;

            inline bool isnull()const { return (pinfo_ == nullptr) && (pdata_ == nullptr); }

            SampleBlock()noexcept {}
            SampleBlock(smpl_t && smpl, wavinfoptr_t && wavi)noexcept
                :pinfo_(std::forward<wavinfoptr_t>(wavi)), pdata_(std::forward<smpl_t>(smpl))
            {}

            SampleBlock(const SampleBlock& other)noexcept
            {
                operator=(other);
            }

            SampleBlock& operator=(const SampleBlock& other)noexcept
            {
                if(other.pdata_)
                    pdata_ = smpl_t(new std::vector<uint8_t>(*other.pdata_));
                if (other.pinfo_)
                    pinfo_ = wavinfoptr_t(new DSE::WavInfo(*other.pinfo_));
                return *this;
            }

            SampleBlock( SampleBlock && other )noexcept
            {
                pinfo_.reset( other.pinfo_.release() );
                pdata_.reset( other.pdata_.release() );
            }

            SampleBlock & operator=( SampleBlock && other )noexcept
            {
                pinfo_.reset( other.pinfo_.release() );
                pdata_.reset( other.pdata_.release() );
                return *this;
            }

            SampleBlock merge(const SampleBlock& other)const
            {
                smpl_t newsmpl;
                wavinfoptr_t newwavi;
                if (!other.isnull())
                {
                    if (other.pdata_)
                        newsmpl = smpl_t(new std::vector<uint8_t>(*(other.pdata_)));
                    if (other.pinfo_)
                        newwavi = wavinfoptr_t(new DSE::WavInfo(*(other.pinfo_)));
                }
                if (!isnull())
                {
                    if (!newsmpl && pdata_)
                        newsmpl = smpl_t(new std::vector<uint8_t>(*pdata_));
                    if (!newwavi && pinfo_)
                        newwavi = wavinfoptr_t(new DSE::WavInfo(*pinfo_));
                }
                return SampleBlock(std::move(newsmpl), std::move(newwavi));
            }
        };
        typedef std::vector<SampleBlock>::iterator       iterator;
        typedef std::vector<SampleBlock>::const_iterator const_iterator;

        SampleBank()
        {}

        SampleBank( std::vector<SampleBlock> && smpls )noexcept
            :m_SampleData(std::forward<std::vector<SampleBlock>>(smpls))
        {}

        SampleBank( SampleBank && mv )noexcept
        {
            m_SampleData = std::move(mv.m_SampleData);
        }

        SampleBank & operator=( SampleBank && mv )noexcept
        {
            m_SampleData = std::move(mv.m_SampleData);
            return *this;
        }

        SampleBank( const SampleBank & other )
        {
            DoCopyFrom(other);
        }

        const SampleBank & operator=( const SampleBank & other )
        {
            DoCopyFrom(other);
            return *this;
        }

        inline iterator       begin()     { return m_SampleData.begin(); }
        inline const_iterator begin()const{ return m_SampleData.begin(); }
        inline iterator       end()       { return m_SampleData.end();   }
        inline const_iterator end()const  { return m_SampleData.end();   }
        inline sampleid_t     size()const { return NbSlots(); }

        SampleBank merge(const SampleBank& other)const 
        {
            SampleBank sbnk;
            const sampleid_t maxsmpls = std::max(m_SampleData.size(), other.m_SampleData.size());
            sbnk.m_SampleData.resize(maxsmpls);

            for (sampleid_t cntsmpls = 0; cntsmpls < maxsmpls; ++cntsmpls)
            {
                SampleBlock newblk;

                if (cntsmpls < other.m_SampleData.size() && cntsmpls < m_SampleData.size())
                    newblk = m_SampleData[cntsmpls].merge(other.m_SampleData[cntsmpls]);
                else if (cntsmpls < other.m_SampleData.size() && !(other.m_SampleData[cntsmpls].isnull()))
                {
                    newblk = other.m_SampleData[cntsmpls];
                }
                else if (cntsmpls < m_SampleData.size() && !(m_SampleData[cntsmpls].isnull()))
                {
                    newblk = m_SampleData[cntsmpls];
                }
                sbnk.m_SampleData[cntsmpls] = std::move(newblk);
            }
            return sbnk;
        }

    public:
        //Info

        //Nb of sample slots with or without data
        inline sampleid_t                   NbSlots()const     { return m_SampleData.size(); } 

        //Access
        bool                                IsInfoPresent      (sampleid_t index )const { return sampleInfo(index) != nullptr; }
        bool                                IsDataPresent      (sampleid_t index )const { return sample(index)     != nullptr; }

        inline DSE::SampleBank::SampleBlock* sampleBlock(sampleid_t index)
        {
            if (m_SampleData.size() > index)
                return &(m_SampleData[index]);
            else
                return nullptr;
        }

        inline const DSE::SampleBank::SampleBlock* sampleBlock(sampleid_t index)const
        {
            if (m_SampleData.size() > index)
                return &(m_SampleData[index]);
            else
                return nullptr;
        }


        inline DSE::WavInfo* sampleInfo(sampleid_t index)
        { 
            if( m_SampleData.size() > index )
                return m_SampleData[index].pinfo_.get(); 
            else
                return nullptr;
        }

        inline const DSE::WavInfo * sampleInfo(sampleid_t index)const
        { 
            if( m_SampleData.size() > index )
                return m_SampleData[index].pinfo_.get(); 
            else
                return nullptr;
        }


        inline std::vector<uint8_t>* sample(sampleid_t index)
        { 
            if( m_SampleData.size() > index )
                return m_SampleData[index].pdata_.get(); 
            else 
                return nullptr;
        }

        inline const std::vector<uint8_t> * sample(sampleid_t index)const
        { 
            if( m_SampleData.size() > index )
                return m_SampleData[index].pdata_.get(); 
            else 
                return nullptr;
        }

        void setSampleData(sampleid_t index, std::vector<uint8_t> && data)
        {
            if(m_SampleData.size() <= index)
                m_SampleData.resize(index + 1); //Make sure we're big enough

            if (m_SampleData.size() > index)
                m_SampleData[index].pdata_.reset(new std::vector<uint8_t>(data));
        }

        inline std::vector<uint8_t>       * operator[](sampleid_t index)      { return sample(index); }
        inline const std::vector<uint8_t> * operator[](sampleid_t index)const { return sample(index); }

    private:

        //Copy the content pointed by the pointers, and not just the pointers themselves !
        void DoCopyFrom( const SampleBank & other )
        {
            m_SampleData.resize( other.m_SampleData.size() );

            for(sampleid_t i = 0; i < other.m_SampleData.size(); ++i  )
            {
                if( other.m_SampleData[i].pdata_ != nullptr )
                    m_SampleData[i].pdata_.reset( new std::vector<uint8_t>( *(other.m_SampleData[i].pdata_) ) ); //Copy each objects and make a pointer

                if( other.m_SampleData[i].pinfo_ != nullptr )
                    m_SampleData[i].pinfo_.reset( new DSE::WavInfo( *(other.m_SampleData[i].pinfo_) ) ); //Copy each objects and make a pointer
            }
        }

    private:
        std::vector<SampleBlock> m_SampleData;
    };

    /*****************************************************************************************
        KeygroupList
            Encapsulate and validate keygroups.
    *****************************************************************************************/
    class KeyGroupList
    {
    public:
        typedef std::vector<DSE::KeyGroup>::iterator            iterator;
        typedef std::vector<DSE::KeyGroup>::const_iterator      const_iterator;
        typedef std::vector<DSE::KeyGroup>::reference           reference;
        typedef std::vector<DSE::KeyGroup>::const_reference     const_reference;

        KeyGroupList()
        {}

        KeyGroupList( const std::vector<DSE::KeyGroup> & cpy )
            :m_groups(cpy)
        {}

        inline size_t             size()const       { return m_groups.size();  }
        inline bool               empty()const      { return m_groups.empty(); }

        inline iterator           begin()           { return m_groups.begin(); }
        inline const_iterator     begin()const      { return m_groups.begin(); }
        inline iterator           end()             { return m_groups.end(); }
        inline const_iterator     end()const        { return m_groups.end(); }

        inline reference       & front()            { return m_groups.front(); }
        inline const_reference & front()const       { return m_groups.front(); }
        inline reference       & back()             { return m_groups.back(); }
        inline const_reference & back()const        { return m_groups.back(); }

        //Validated Kgrp access!
        inline DSE::KeyGroup & operator[]( size_t kgrp )
        {
            if( kgrp < size() )
                return m_groups[kgrp];
            else
                return m_groups.front(); //Return the gloabal keygroup if the kgrp number is invalid (This might differ in the way the DSE engine handles out of range kgrp values)
        }

        inline const DSE::KeyGroup & operator[]( size_t kgrp )const 
        {
            if( kgrp < size() )
                return m_groups[kgrp];
            else
                return m_groups.front(); //Return the gloabal keygroup if the kgrp number is invalid (This might differ in the way the DSE engine handles out of range kgrp values)
        }

        //Access to raw vector
        inline std::vector<DSE::KeyGroup>       & GetVector()       { return m_groups; }
        inline const std::vector<DSE::KeyGroup> & GetVector()const  { return m_groups; }

    private:
        std::vector<DSE::KeyGroup>  m_groups;
    };


    /*****************************************************************************************
        ProgramBank
            Contains the entries for each program contained in a SWD file, along with the
            file's keygroup table!
    *****************************************************************************************/
    class ProgramBank
    {
    public:
        typedef std::unique_ptr<DSE::ProgramInfo> ptrprg_t;

        ProgramBank()
        {}

        ~ProgramBank()
        {}

        ProgramBank( std::vector<ptrprg_t> && prgminf, std::vector<DSE::KeyGroup> && kgrp )
            :m_prgminfoslots(std::forward<std::vector<ptrprg_t>>(prgminf)),
             m_Groups(kgrp)
        {}

        ProgramBank( ProgramBank && mv )
        {
            operator=(std::forward<ProgramBank>(mv));
        }

        ProgramBank & operator=( ProgramBank&& mv )
        {
            m_prgminfoslots = std::move(mv.m_prgminfoslots);
            m_Groups        = std::move(mv.m_Groups);
            return *this;
        }

        ProgramBank(const ProgramBank& other)
        {
            operator=(other);
        }

        ProgramBank& operator=(const ProgramBank& other)
        {
            m_prgminfoslots.reserve(other.m_prgminfoslots.size());
            for (const ptrprg_t& ptr : other.m_prgminfoslots)
            {
                ptrprg_t newp;
                //The program ptrs may be null
                if (ptr)
                    newp = ptrprg_t(new DSE::ProgramInfo(*ptr)); 
                m_prgminfoslots.push_back(std::move(newp));
            }
            m_Groups = other.m_Groups;
            return *this;
        }

        ProgramBank merge(const ProgramBank& other)const
        {
            ProgramBank pbank;
            const size_t maxprgm = std::max(other.m_prgminfoslots.size(), m_prgminfoslots.size());
            pbank.m_prgminfoslots.resize(maxprgm);

            for (size_t i = 0; i < maxprgm; ++i)
            {
                ptrprg_t newp;
                if (i < other.m_prgminfoslots.size() && other.m_prgminfoslots[i])
                {
                    newp = ptrprg_t(new DSE::ProgramInfo(*(other.m_prgminfoslots[i])));
                }
                else if (i < m_prgminfoslots.size() && m_prgminfoslots[i])
                {
                    newp = ptrprg_t(new DSE::ProgramInfo(*(m_prgminfoslots[i])));
                }
                pbank.m_prgminfoslots[i] = ptrprg_t(newp.release());
            }

            const size_t maxkgrp = std::max(other.m_Groups.size(), m_Groups.size());
            pbank.m_Groups.GetVector().resize(maxkgrp);
            for (size_t cntkgrp = 0; cntkgrp < maxkgrp; ++cntkgrp)
            {
                KeyGroup grp;
                if (cntkgrp < other.m_Groups.size())
                    grp = other.m_Groups[cntkgrp];
                else if (cntkgrp < m_Groups.size())
                    grp = m_Groups[cntkgrp];
                else
                    continue;
                pbank.m_Groups[cntkgrp] = grp;
            }
            return pbank;
        }

        ptrprg_t                          & operator[]( size_t index )         { return m_prgminfoslots[index]; }
        const ptrprg_t                    & operator[]( size_t index )const    { return m_prgminfoslots[index]; }

        std::vector<ptrprg_t>             & PrgmInfo()      { return m_prgminfoslots; }
        const std::vector<ptrprg_t>       & PrgmInfo()const { return m_prgminfoslots; }

        KeyGroupList                      & Keygrps()       { return m_Groups;        }
        const KeyGroupList                & Keygrps()const  { return m_Groups;        }

    private:
        std::vector<ptrprg_t>       m_prgminfoslots;
        KeyGroupList                m_Groups;
    };

    /*****************************************************************************************
        PresetBank
            Is the combination of a SampleBank, and an ProgramBank.
            Or just an instrument bank if samples are not available
    *****************************************************************************************/
    class PresetBank
    {
    public:

        typedef std::shared_ptr<ProgramBank>   ptrprg_t;
        typedef std::weak_ptr<ProgramBank>     wptrprg_t;

        typedef std::shared_ptr<SampleBank>       ptrsmpl_t;
        typedef std::weak_ptr<SampleBank>         wptrsmpl_t;

        PresetBank()
        {}

        PresetBank( DSE::DSE_MetaDataSWDL && meta, std::unique_ptr<ProgramBank> && pInstrument, std::unique_ptr<SampleBank>  && pSmpl )
            :m_pPrgbnk(std::move(pInstrument)), m_pSamples(std::move(pSmpl)), m_meta(std::move(meta))
        {}

        PresetBank( DSE::DSE_MetaDataSWDL && meta, std::unique_ptr<ProgramBank> && pInstrument )
            :m_pPrgbnk(std::move(pInstrument)), m_pSamples(nullptr), m_meta(std::move(meta))
        {}

        PresetBank( DSE::DSE_MetaDataSWDL && meta, std::unique_ptr<SampleBank> && pSmpl )
            :m_pPrgbnk(nullptr), m_pSamples(std::move(pSmpl)), m_meta(std::move(meta))
        {}

        PresetBank( PresetBank && mv )
        {
            operator=(std::forward<PresetBank>(mv));
        }

        PresetBank & operator=( PresetBank && mv )
        {
            m_pPrgbnk  = std::move( mv.m_pPrgbnk  );
            m_pSamples = std::move( mv.m_pSamples );
            m_meta     = std::move( mv.m_meta     );
            return *this;
        }

        PresetBank merge(const PresetBank& other)const
        {
            PresetBank pbank;
            pbank.metadata(m_meta);

            //Merge, copy, or not the program bank
            if(m_pPrgbnk && other.m_pPrgbnk)
                pbank.prgmbank(std::unique_ptr<ProgramBank>(new ProgramBank(m_pPrgbnk->merge(*other.m_pPrgbnk))));
            else if(m_pPrgbnk)
                pbank.prgmbank(std::unique_ptr<ProgramBank>(new ProgramBank(*m_pPrgbnk)));
            else if (other.m_pPrgbnk)
                pbank.prgmbank(std::unique_ptr<ProgramBank>(new ProgramBank(*other.m_pPrgbnk)));

            //Merge, copy or not the samples
            if (m_pSamples && other.m_pSamples)
                pbank.smplbank(std::unique_ptr<SampleBank>(new SampleBank(m_pSamples->merge(*other.m_pSamples))));
            else if(m_pSamples)
                pbank.smplbank(std::unique_ptr<SampleBank>(new SampleBank(*m_pSamples)));
            else if(other.m_pSamples)
                pbank.smplbank(std::unique_ptr<SampleBank>(new SampleBank(*other.m_pSamples)));

            return pbank;
        }


        DSE::DSE_MetaDataSWDL       & metadata()                                                 { return m_meta; }
        const DSE::DSE_MetaDataSWDL & metadata()const                                            { return m_meta; }
        void                      metadata( const DSE::DSE_MetaDataSWDL & data )                 { m_meta = data; }
        void                      metadata( DSE::DSE_MetaDataSWDL && data )                      { m_meta = data; }
        
        //Returns a weak_ptr to the samplebank
        wptrsmpl_t                smplbank()                                                { return m_pSamples; }
        const wptrsmpl_t          smplbank()const                                           { return m_pSamples; }
        void                      smplbank( ptrsmpl_t && samplesbank)                       { m_pSamples = std::move(samplesbank); }
        //void                      smplbank( SampleBank  * samplesbank)                       { m_pSamples = samplesbank; }
        void                      smplbank( std::unique_ptr<SampleBank>  && samplesbank)     { m_pSamples = std::move(samplesbank); }

        //Returns a weak_ptr to the program bank
        wptrprg_t                prgmbank()                                                { return m_pPrgbnk; }
        const wptrprg_t          prgmbank()const                                           { return m_pPrgbnk; }
        void                      prgmbank( ptrprg_t    && bank)                           { m_pPrgbnk = std::move(bank); }
        //void                      prgmbank( ProgramBank * bank)                           { m_pPrgbnk.reset(bank); }
        void                      prgmbank( std::unique_ptr<ProgramBank>     && bank)       { m_pPrgbnk = std::move(bank); }

    private:
        //Can't copy
        PresetBank( const PresetBank& );
        PresetBank& operator=( const PresetBank& );

        DSE::DSE_MetaDataSWDL m_meta;
        ptrprg_t              m_pPrgbnk;  //A program bank may not be shared by many
        ptrsmpl_t             m_pSamples; //A sample bank may be shared by many
    };

    /*****************************************************************************************
        MusicTrack
            Represent a single track of DSE events within a music sequence!
    *****************************************************************************************/
    class MusicTrack
    {
    public:
        typedef std::vector<DSE::TrkEvent>::iterator       iterator;
        typedef std::vector<DSE::TrkEvent>::const_iterator const_iterator;


        MusicTrack() {}

        DSE::TrkEvent       & operator[]( size_t index )      {return m_events[index];}
        const DSE::TrkEvent & operator[]( size_t index )const {return m_events[index];}

        iterator       begin()      { return m_events.begin(); }
        const_iterator begin()const { return m_events.begin(); }

        iterator       end()      { return m_events.end(); }
        const_iterator end()const { return m_events.end(); }

        bool   empty        ()const       { return m_events.empty();  }
        size_t size         ()const       { return m_events.size();   }
        
        void   reserve      ( size_t sz ) { m_events.reserve(sz);     }
        void   resize       ( size_t sz ) { m_events.resize(sz);      } 
        void   shrink_to_fit()            { m_events.shrink_to_fit(); }

        void push_back( DSE::TrkEvent && ev ) { m_events.push_back(ev); }
        void push_back( DSE::TrkEvent ev )    { m_events.push_back(std::move(ev)); }

        /*
            Get the DSE events for this track
        */
        std::vector<DSE::TrkEvent>       & getEvents()      { return m_events; }
        const std::vector<DSE::TrkEvent> & getEvents()const { return m_events; }
        void setEvents(const std::vector<DSE::TrkEvent> & events) { m_events = events; }
        void setEvents(std::vector<DSE::TrkEvent>&&       events) { m_events = events; }

        /*
            Get or set the MIDI channel that was assigned to this track.
        */
        void    SetMidiChannel( uint8_t chan ) { m_midichan = chan; }
        uint8_t GetMidiChannel()const          { return m_midichan; }

    private:
        uint8_t                    m_midichan; //The channel of the track
        std::vector<DSE::TrkEvent> m_events;
    };

    /*****************************************************************************************
        MusicSequence
            Contains data for a single musical sequence from the PMD2 games.
            Contains events, and music parameters, along with sample information and mapping.
    *****************************************************************************************/
    class MusicSequence
    {
    public:
        MusicSequence()
        {}

        MusicSequence(std::vector< MusicTrack >&& tracks, PresetBank* presets = nullptr)
                    :m_tracks(tracks), m_pPresetBank(presets)
        {}

        MusicSequence(std::vector< MusicTrack >&& tracks, DSE::seqinfo_table&& sinfo, PresetBank* presets = nullptr)
            :m_tracks(tracks), m_pPresetBank(presets), m_seqinfo(sinfo)
        {}

        MusicSequence(std::vector< MusicTrack >&& tracks, DSE::DSE_MetaDataSMDL&& meta)
            :m_meta(meta), m_tracks(tracks)
        {}

        MusicSequence( std::vector< MusicTrack > && tracks, 
                       DSE::DSE_MetaDataSMDL     && meta,
                       DSE::seqinfo_table &&        sinfo,
                       PresetBank                 * presets = nullptr )
            :m_meta(meta), m_tracks(tracks), m_pPresetBank(presets), m_seqinfo(sinfo)
        {}

        PresetBank * presetbnk()                                             { return m_pPresetBank; }
        void         presetbnk( PresetBank * ptrbnk )                        { m_pPresetBank = ptrbnk; }

        DSE::DSE_MetaDataSMDL       & metadata()                                 { return m_meta; }
        const DSE::DSE_MetaDataSMDL & metadata()const                            { return m_meta; }
        void                      metadata( const DSE::DSE_MetaDataSMDL & data ) { m_meta = data; }
        void                      metadata( DSE::DSE_MetaDataSMDL && data )      { m_meta = data; }

        DSE::seqinfo_table&       seqinfo()                                { return m_seqinfo; }
        const DSE::seqinfo_table& seqinfo()const                           { return m_seqinfo; }
        void                      seqinfo(const DSE::seqinfo_table& sinfo) { m_seqinfo = sinfo; }
        void                      seqinfo(DSE::seqinfo_table&       sinfo) { m_seqinfo = sinfo; }

        size_t                             getNbTracks()const                { return m_tracks.size(); }

        MusicTrack      & track( size_t index )             { return m_tracks[index]; }
        const MusicTrack & track( size_t index )const        { return m_tracks[index]; }

        MusicTrack      & operator[]( size_t index )        { return m_tracks[index]; }
        const MusicTrack & operator[]( size_t index )const   { return m_tracks[index]; }

        //Print statistics on the music sequence
        std::string printinfo()const;

        //Makes it possible to iterate through the tracks using a foreach loop
        typedef std::vector<MusicTrack>::iterator       iterator;
        typedef std::vector<MusicTrack>::const_iterator const_iterator;

        inline iterator       begin()      { return move(m_tracks.begin()); }
        inline const_iterator begin()const { return move(m_tracks.begin()); }
        inline iterator       end()        { return move(m_tracks.end()); }
        inline const_iterator end()const   { return move(m_tracks.end()); }
        inline bool           empty()const { return m_tracks.empty(); }

    private:
        DSE::DSE_MetaDataSMDL    m_meta;
        DSE::seqinfo_table       m_seqinfo;
        std::vector<MusicTrack>  m_tracks;
        PresetBank             * m_pPresetBank; //Maybe should be a shared ptr? //#REMOVEME: This is redundant
    };

    /*****************************************************************************************
    *****************************************************************************************/
    struct McrlChunkContents
    {
        struct mcrlentry
        {
            uint16_t    unk12 = 0;
            std::string unkstr;

            template<class _init> _init Read(_init itbeg, _init itend)
            {
                _init itentrybeg = itbeg;
                unk12 = utils::ReadIntFromBytes<uint16_t>(itbeg, itend);
                if (unk12 == 0xFFFF)
                {
                    //If the entry is 0xFFFF there's nothing to parse
                    return itbeg;
                }

                uint16_t entrylen = utils::ReadIntFromBytes<uint16_t>(itbeg, itend);
                _init itendstr = std::next(itentrybeg, entrylen);
                unkstr = std::string(itbeg, itendstr);
                return itbeg;
            }

            template<class _outit> _outit Write(_outit itout)
            {
                itout = utils::WriteIntToBytes(unk12, itout);
                if (unk12 == 0xFFFF)
                    return itout; //Nothing else to do in this case

                uint16_t totallen = (sizeof(uint16_t) * 2) + unkstr.length();
                totallen += (totallen % 2); //Add to the count a padding byte if neccessary to align us on 2 bytes
                itout = utils::WriteIntToBytes(totallen, itout); //Put the computed entry length

                //Next put the string
                itout = utils::WriteStrToByteContainer(itout, unkstr);

                //And the padding
                utils::AppendPaddingBytes(itout, unkstr.length(), 2); //Since the string is the only thing that changes in size, use it to calc padding
                return itout;
            }
        };

        void AddEntry(mcrlentry && entry)
        {
            mcrldata.push_back(entry);
        }

        void AddEntry(mcrlentry entry)
        {
            mcrldata.push_back(std::move(entry));
        }

        inline bool empty()const {return mcrldata.empty();}

        std::vector<mcrlentry> mcrldata;
    };

    /*****************************************************************************************
    *****************************************************************************************/
    struct BnklChunkContents
    {
        struct bnklentry
        {
            uint16_t unk17 = 0;
            std::vector<uint16_t> bankids;

            template<class _init> _init Read(_init itbeg, _init itend)
            {
                bankids.resize(0);
                _init itentrybeg = itbeg;
                uint16_t nbbankids = utils::ReadIntFromBytes<uint16_t>(itbeg, itend);
                unk17 = utils::ReadIntFromBytes<uint16_t>(itbeg, itend);

                for (int cntbank = 0; cntbank < nbbankids; ++cntbank)
                    bankids.push_back(utils::ReadIntFromBytes<uint16_t>(itbeg, itend));
                return itbeg;
            }

            template<class _outit> _outit Write(_outit itout)
            {
                itout = utils::WriteIntToBytes(static_cast<uint16_t>(bankids.size()), itout);
                itout = utils::WriteIntToBytes(unk17, itout);
                for (uint16_t bankid : bankids)
                    itout = utils::WriteIntToBytes(bankid, itout);
                return itout;
            }
        };

        void AddEntry(bnklentry&& entry)
        {
            bnkldata.push_back(entry);
        }

        void AddEntry(bnklentry entry)
        {
            bnkldata.push_back(std::move(entry));
        }

        inline bool empty()const { return bnkldata.empty(); }

        std::vector<bnklentry> bnkldata;
    };

    /*****************************************************************************************
    *****************************************************************************************/
    /// <summary>
    /// Stores the contents of a SEDL file
    /// </summary>
    class SoundEffectSequences
    {
    public:
        SoundEffectSequences()
        {}

        SoundEffectSequences(std::vector<MusicSequence>&& sequences, DSE::DSE_MetaDataSEDL&& meta)
            :m_meta(meta), m_sequences(sequences)
        {}

        SoundEffectSequences(std::vector<MusicSequence> && sequences, McrlChunkContents && mcrl, BnklChunkContents && bnkl, DSE::DSE_MetaDataSEDL && meta)
            :m_meta(meta), m_sequences(sequences), m_mcrl(mcrl), m_bnkl(bnkl)
        {}

        inline DSE::DSE_MetaDataSEDL& metadata() { return m_meta; }
        inline const DSE::DSE_MetaDataSEDL& metadata()const { return m_meta; }

        DSE::DSE_MetaDataSEDL       m_meta;
        std::vector<MusicSequence>  m_sequences;
        McrlChunkContents           m_mcrl;
        BnklChunkContents           m_bnkl;
    };


    /*****************************************************************************************
    *****************************************************************************************/
    /// <summary>
    /// Wrapper around a large set of PresetBanks to allow much more easily indexing and accessing its contents.
    /// </summary>
    class PresetDatabase
    {
    public:
        using sampledata_t = std::vector<uint8_t>;
        using bankname_t = std::string;

        //Wrapper for adding new banks to the loader so we can set them up properly
        DSE::PresetBank& AddBank(DSE::PresetBank&& bnk);

        PresetDatabase();

    public:
        //Helpers
        //Retrieve the last overriden sample data for the given sample and bank
        sampledata_t* getSampleForBank(sampleid_t smplid, const bankname_t& bnkid);

        //Retrieve the last overriden sample info for the given sample and bank
        DSE::WavInfo* getSampleInfoForBank(sampleid_t smplid, const bankname_t& bnkid);

        //Retrieve the sample block for a given sample id
        SampleBank::SampleBlock* getSampleBlockFor(sampleid_t smplid, const bankname_t& bnkid);

        inline const SampleBank::SampleBlock* getSampleBlockFor(sampleid_t smplid, const bankname_t& bnkid)const
        {
            return const_cast<PresetDatabase*>(this)->getSampleBlockFor(smplid, bnkid);
        }

        inline const DSE::WavInfo* getSampleInfoForBank(sampleid_t smplid, const bankname_t& bnkid)const
        {
            return const_cast<PresetDatabase*>(this)->getSampleInfoForBank(smplid, bnkid);
        }

        inline const sampledata_t* getSampleForBank(sampleid_t smplid, const bankname_t& bnkid)const
        {
            return const_cast<PresetDatabase*>(this)->getSampleForBank(smplid, bnkid);
        }

        inline sampleid_t getNbSampleBlocks()const
        {
            return m_nbSampleBlocks;
        }

    public:
        //Std access stuff
        typedef std::unordered_map<bankname_t, PresetBank>::iterator       iterator;
        typedef std::unordered_map<bankname_t, PresetBank>::const_iterator const_iterator;

        inline size_t size()const { return m_banks.size(); }
        inline bool empty()const { return m_banks.empty(); }
        inline bool contains(const bankname_t bankid)const { return m_banks.contains(bankid); }

        inline iterator begin() { return m_banks.begin(); }
        inline const_iterator begin()const { return m_banks.begin(); }

        inline iterator end() { return m_banks.end(); }
        inline const_iterator end()const { return m_banks.end(); }

        inline PresetBank& operator[](const bankname_t& index) { return m_banks.at(index); }
        inline const PresetBank& operator[](const bankname_t& index)const { return m_banks.at(index); }

        inline iterator find(const bankname_t& bankid) { return m_banks.find(bankid); }
        inline const_iterator find(const bankname_t& bankid)const { return m_banks.find(bankid); }

        inline PresetBank& at(const bankname_t& bankid) { return m_banks.at(bankid); }
        inline const PresetBank& at(const bankname_t& bankid)const { return m_banks.at(bankid); }
    
    private:
        std::unordered_map<std::string, PresetBank>                         m_banks;          //Map of all our banks by a string id, either the original filename, or something else in the case where there's no filename
        std::map<sampleid_t, std::map<bankname_t, SampleBank::SampleBlock>> m_sampleMap;      //Allow us to grab the latest overriden sample entry by id and bank id.
        std::map<presetid_t, std::set<bankname_t>>                          m_preset2banks;   //List what loaded banks actually define a given preset
        std::map<sampleid_t, std::set<bankname_t>>                          m_sample2banks;   //List in what banks a given sample id is defined
        sampleid_t                                                          m_nbSampleBlocks; //The Maximum number of sample blocks contained throughout all banks
    };

};

#endif