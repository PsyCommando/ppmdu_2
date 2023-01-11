#ifndef DSE_MUSIC_TRACK_HPP
#define DSE_MUSIC_TRACK_HPP
#include <dse/dse_common.hpp>
#include <dse/dse_sequence.hpp>

namespace DSE
{
    /*****************************************************************************************
        MusicTrack
            Represent a single track of DSE events within a music sequence!
    *****************************************************************************************/
    class MusicTrack
    {
    public:
        typedef std::vector<DSE::TrkEvent>::iterator       iterator;
        typedef std::vector<DSE::TrkEvent>::const_iterator const_iterator;

        MusicTrack()noexcept :m_midichan(0) {}

        DSE::TrkEvent& operator[](size_t index) { return m_events[index]; }
        const DSE::TrkEvent& operator[](size_t index)const { return m_events[index]; }

        iterator       begin() { return m_events.begin(); }
        const_iterator begin()const { return m_events.begin(); }

        iterator       end() { return m_events.end(); }
        const_iterator end()const { return m_events.end(); }

        bool   empty()const { return m_events.empty(); }
        size_t size()const { return m_events.size(); }

        void   reserve(size_t sz) { m_events.reserve(sz); }
        void   resize(size_t sz) { m_events.resize(sz); }
        void   shrink_to_fit() { m_events.shrink_to_fit(); }

        void push_back(DSE::TrkEvent&& ev) { m_events.push_back(ev); }

        /*
            Get the DSE events for this track
        */
        std::vector<DSE::TrkEvent>& getEvents() { return m_events; }
        const std::vector<DSE::TrkEvent>& getEvents()const { return m_events; }
        void setEvents(const std::vector<DSE::TrkEvent>& events) { m_events = events; }
        void setEvents(std::vector<DSE::TrkEvent>&& events) { m_events = events; }

        /*
            Get or set the MIDI channel that was assigned to this track.
        */
        void    SetMidiChannel(uint8_t chan) { m_midichan = chan; }
        uint8_t GetMidiChannel()const { return m_midichan; }

    private:
        uint8_t                    m_midichan; //The channel of the track
        std::vector<DSE::TrkEvent> m_events;
    };
};

#endif // !DSE_MUSIC_TRACK_HPP
