#ifndef DSE_MUSIC_SEQUENCE_HPP
#define DSE_MUSIC_SEQUENCE_HPP
#include <dse/dse_common.hpp>
#include <dse/containers/dse_music_track.hpp>

namespace DSE
{
    /*****************************************************************************************
        MusicSequence
            Contains data for a single musical sequence from the PMD2 games.
            Contains events, and music parameters, along with sample information and mapping.
    *****************************************************************************************/
    class MusicSequence
    {
    public:
        MusicSequence()noexcept
        {}

        MusicSequence(std::vector< MusicTrack >&& tracks)noexcept
            :m_tracks(tracks)
        {}

        MusicSequence(std::vector< MusicTrack >&& tracks, DSE::seqinfo_table&& sinfo)noexcept
            :m_tracks(tracks), m_seqinfo(sinfo)
        {}

        MusicSequence(std::vector< MusicTrack >&& tracks, DSE::DSE_MetaDataSMDL&& meta)noexcept
            :m_meta(meta), m_tracks(tracks)
        {}

        MusicSequence(std::vector< MusicTrack >&& tracks, DSE::DSE_MetaDataSMDL&& meta, DSE::seqinfo_table&& sinfo)noexcept
            :m_meta(meta), m_tracks(tracks), m_seqinfo(sinfo)
        {}

        DSE::DSE_MetaDataSMDL&       metadata()                                     { return m_meta; }
        const DSE::DSE_MetaDataSMDL& metadata()const                                { return m_meta; }
        void                         setMetadata(const DSE::DSE_MetaDataSMDL& data) { m_meta = data; }
        void                         setMetadata(DSE::DSE_MetaDataSMDL&& data)      { m_meta = data; }

        DSE::seqinfo_table&       seqinfo()                                   { return m_seqinfo; }
        const DSE::seqinfo_table& seqinfo()const                              { return m_seqinfo; }
        void                      setSeqinfo(const DSE::seqinfo_table& sinfo) { m_seqinfo = sinfo; }
        void                      setSeqinfo(DSE::seqinfo_table&&      sinfo) { m_seqinfo = sinfo; }

        size_t getNbTracks()const { return m_tracks.size(); }
        void setNbTracks(size_t newsize) { m_tracks.resize(newsize); }

        void setTracks(std::vector<MusicTrack>&& trks) { m_tracks = trks; }
        void setTrack(size_t trkid, MusicTrack&& trk)  { m_tracks[trkid] = trk; }

        MusicTrack& track(size_t index) { return m_tracks[index]; }
        const MusicTrack& track(size_t index)const { return m_tracks[index]; }

        MusicTrack& operator[](size_t index) { return m_tracks[index]; }
        const MusicTrack& operator[](size_t index)const { return m_tracks[index]; }

        //Print statistics on the music sequence
        std::string printinfo()const;

        //Makes it possible to iterate through the tracks using a foreach loop
        typedef std::vector<MusicTrack>::iterator       iterator;
        typedef std::vector<MusicTrack>::const_iterator const_iterator;

        inline iterator       begin() { return move(m_tracks.begin()); }
        inline const_iterator begin()const { return move(m_tracks.begin()); }
        inline iterator       end() { return move(m_tracks.end()); }
        inline const_iterator end()const { return move(m_tracks.end()); }
        inline bool           empty()const { return m_tracks.empty(); }

    private:
        DSE::DSE_MetaDataSMDL    m_meta;
        DSE::seqinfo_table       m_seqinfo;
        std::vector<MusicTrack>  m_tracks;
    };
};

#endif // !DSE_MUSIC_SEQUENCE_HPP
