#include <dse/dse_interpreter.hpp>
#include <utils/poco_wrapper.hpp>
#include <dse/dse_conversion.hpp>

#include <jdksmidi/world.h>
#include <jdksmidi/track.h>
#include <jdksmidi/multitrack.h>
#include <jdksmidi/filereadmultitrack.h>
#include <jdksmidi/fileread.h>
#include <jdksmidi/fileshow.h>
#include <jdksmidi/filewritemultitrack.h>

#include <Poco/Path.h>

#include <list>
#include <iomanip>
#include <functional>
#include <numeric>
#include <string>
#include <cmath>
#include <deque>
#include <set>
#include <stack>
#include <algorithm>

#ifndef AUDIOUTIL_VER
    #define AUDIOUTIL_VER "Poochyena"
#endif

using namespace std;
using namespace utils;

namespace DSE
{
    const std::string UtilityID           = "ExportedWith: ppmd_audioutil.exe ver" AUDIOUTIL_VER;
    const std::string TXT_LoopStart       = "LoopStart";
    const std::string TXT_LoopEnd         = "LoopEnd";
    const std::string TXT_HoldNote        = "HoldNote";
    const std::string TXT_DSE_Event       = "DSE_Event"; //Marks DSE events that have no MIDI equivalents
    const std::string TXT_RepeatMark      = "RepeatMark"s;
    const std::string TXT_RepeatFromMark  = "RepeatFromMark"s;
    const std::string TXT_AfterRepeatMark = "AfterRepeatMark"s;


//======================================================================================
//  DSESequenceToMidi_Improv
//======================================================================================
#if 0
    /*
        Improved version of the DSE MIDI Sequence converter.

        This version will work based on MIDI channels
        It will process all DSE tracks together at the same time. So it will be much easier
        to process the effect of an event, and pick an output MIDI channel for it.

        We should also keep preset/program states, so we can keep track of polyphony.
        And then, we need to see what presets are playing at the same time on other channels,
        and "prioritize" some based on their keygroup priority. I suspect it involves making
        one channel louder than the other by a certain amount of decibel!
    */
    class DSESequenceToMidi_Improv
    {
    public:
        typedef uint32_t ticks_t;

        /*
            Holds data about a note event. What time it began, what time it ends on. And all the required
            data to make a midi event.

            Those are used to keep track of the notes played at the same time.
        */
        class NoteEvent
        {
        public:
            NoteEvent(midinote_t noteid, int8_t velocity, uint8_t midichan, ticks_t begtime, ticks_t endtime);

            //This creates a MIDI event from the data contained in the note!
            operator jdksmidi::MIDITimedBigMessage()const;

            //This returns whether the note is finished playing from the time passed in parameters.
            bool IsNoteFinished(ticks_t time)const;

            //This changes the endtime value to the time in ticks specified!
            void CutNoteNow(ticks_t ticks);

            //The midi note to play
            midinote_t NoteID()const;

            //Get or set the target midi channel
            uint8_t MidiChan()const;
            void    MidiChan(uint8_t midichan);

        private:
        };

        /*
            Holds state data for a single dse program.
        */
        class PrgState
        {
        public:

        private:
            deque<NoteEvent> m_notes;
        };

        /*
            Holds state data for a single channel.
        */
        class ChannelState
        {
        public:

        private:
        };

        /*
            Holds the state of a DSE sequencer track
        */
        class TrackState
        {
        public:

            //This makes a DSE tracks determine if it needs to handle an event, and if it does handle it.
            void Process(ticks_t now);

            ticks_t NextEvent()const;

            size_t EventIndex()const;

        private:
            ticks_t m_nextev;   //The time in ticks when the next event will be processed
            size_t  m_evindex;  //The index of the DSE event being processed
        };

        /*
            Contains the sequencer-wide state variables.
        */
        struct SequencerState
        {
            ticks_t            m_globalTicks;  //The ticks the Sequencer is at.
            vector<TrackState> m_TrkState;     //State of all the DSE tracks
            deque<PrgState>    m_prgstates;    //State of all the DSE programs
        };


    private:
        //This calls the Process() method of all tracks
        void ProcessATick();


    private:
        SequencerState        m_CurState;     //Current state of the sequencer
        SequencerState        m_LoopBegState; //Saved state of the sequencer at the beginning of the loop
        const MusicSequence* m_seq;          //The Music sequence we're working on

    };
#endif

};