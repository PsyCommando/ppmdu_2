#include <dse/dse_interpreter.hpp>
#include <dse/dse_to_xml.hpp>

#include <utils/parse_utils.hpp>

#include <jdksmidi/world.h>
#include <jdksmidi/track.h>
#include <jdksmidi/multitrack.h>
#include <jdksmidi/filereadmultitrack.h>
#include <jdksmidi/fileread.h>
#include <jdksmidi/fileshow.h>
#include <jdksmidi/filewritemultitrack.h>

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/JSON/Parser.h>

#include <stack>

#include <pugixml.hpp>

using namespace std;
using namespace jdksmidi;
using namespace utils;
using namespace pugi;

namespace DSE
{
    //======================================================================================
    //  MIDIToDSE
    //======================================================================================
        /*
            MIDIToDSE
                Convert a MIDI file into a DSE Sequence.

                **WIP**
        */
    class MIDIToDSE
    {
        typedef unsigned long long ticks_t;
        MusicSequence                   m_seqOut;
        std::unique_ptr<MIDIMultiTrack> m_miditrks;
        std::map<uint8_t, MusicTrack>   m_dsetracks;
        std::map<uint8_t, TrkState>     m_dsetrkstates;
    public:

        /*
            - bNoDeltaTimeRounding : If this is true, the program won't try to round delta-time between midi events to the
                                     closest matches in the DSE system.
        */
        MIDIToDSE(const string& srcmidi, bool bNoDeltaTimeRounding = true)
            :m_srcpath(srcmidi), m_bIsAccurateTime(bNoDeltaTimeRounding)
        {}

        /****************************************************************************************
        ****************************************************************************************/
        MusicSequence operator()()
        {
            return std::forward<MusicSequence>(operator()(MusicSequence()));
        }

        MusicSequence operator()(MusicSequence&& startseq)
        {
            using namespace jdksmidi;
            m_seqOut = startseq;
            m_miditrks.reset(new MIDIMultiTrack);
            m_dsetracks.clear();
            m_dsetrkstates.clear();

            if (utils::LibWide().isLogOn())
            {
                clog << "------------------------\n"
                    << "Converting MIDI to DSE\n"
                    << "------------------------\n"
                    ;
            }

            //#1 - Load the MIDI
            MIDIFileReadStreamFile rs(m_srcpath.c_str());
            MIDIFileReadMultiTrack track_loader(m_miditrks.get());
            MIDIFileRead           reader(&rs, &track_loader);

            if (!reader.Parse()) //Apparently handling errors with exceptions is too much to ask to jdksmidi :P
                throw runtime_error("JDKSMIDI: File parsing failed. Reason not specified..");

            if (utils::LibWide().isLogOn())
            {
                clog << "MIDI loaded! :\n"
                    << "\t" << m_srcpath << "\n"
                    << "\t\t" << "NbTracks : " << m_miditrks->GetNumTracksWithEvents() << "\n"
                    << "\t\t" << "NbEvents : " << m_miditrks->GetNumEvents() << "\n"
                    << "\n";
                ;
            }

            //#2 - Convert the MIDI to a sequence
            if(m_seqOut.metadata().fname.empty())
                m_seqOut.metadata().fname = utils::GetBaseNameOnly(m_srcpath);
            if(m_seqOut.metadata().origfname.empty())
                m_seqOut.metadata().origfname = Poco::Path::transcode(Poco::Path(m_seqOut.metadata().fname).setExtension("smd").toString()); //#TODO: Maybe try grabbing the original fname from the midi's metadata or something?
            m_seqOut.metadata().createtime.SetTimeToNow();

            ConvertMIDI();
            m_miditrks.release();
            return std::move(m_seqOut);
        }

    private:
        /****************************************************************************************
        ****************************************************************************************/
        void ConvertMIDI()
        {
            using namespace jdksmidi;
            

            //Determine if multi-tracks
            const bool ismultitrk = m_miditrks->GetNumTracksWithEvents() > 1;

            if (ismultitrk)
                throw std::runtime_error("Input midi is multi-track format, which is unsupported!");

            //If single track, use the MIDI channel for each events to place them onto a specific track
            ConvertFromSingleTrackMidi();

            //Convert our track map into a track vector
            std::vector<MusicTrack> passedtrk;
            passedtrk.reserve(NB_DSETracks);
            uint8_t cntchan = 0;
            for (auto& entry : m_dsetracks)
            {
                entry.second.SetMidiChannel(cntchan++);
                if (((size_t)entry.first + 1) > passedtrk.size())
                    passedtrk.resize((size_t)entry.first + 1);
                passedtrk[entry.first] = std::move(entry.second);
            }
            m_seqOut.setTracks(std::move(passedtrk));
        }

        /****************************************************************************************
            ConvertFromSingleTrackMidi
        ****************************************************************************************/
        void ConvertFromSingleTrackMidi()
        {
            using namespace jdksmidi;
            const MIDITrack* mtrack = m_miditrks->GetTrack(0);
            if (mtrack == nullptr)
                throw std::runtime_error("ConvertFromSingleTrackMidi(): JDKSMIDI: jdksmidi returned a null track ! wtf..");

            //Maintain a global tick count
            ticks_t                ticks = 0;
            const int              nbev = mtrack->GetNumEvents();

            //Iterate through events
            for (int cntev = 0; cntev < nbev; ++cntev)
            {
                const MIDITimedBigMessage* ptrev = mtrack->GetEvent(cntev);
                if (ptrev == nullptr)
                    continue;

                try
                {
                    HandleSingleTrackEvent(*ptrev, ticks);
                }
                catch (...)
                {
                    std::string txtbuff;
                    txtbuff.resize(256); //The MsgToText function requires a buffer of 64 characters. Because its really dumb..
                    ptrev->MsgToText(&txtbuff.front());
                    stringstream sstr;
                    sstr << "Error parsing midi message at " << std::to_string(ptrev->GetTime()) << "midi clk, channel #" << (int)(ptrev->GetChannel()) << ", (0x"
                        << uppercase << hex << setw(2) << setfill('0') << std::to_string((int)(ptrev->GetByte1())) << ", 0x" << setw(2) << std::to_string((int)(ptrev->GetByte2())) << "): "
                        << txtbuff;
                    std::throw_with_nested(std::runtime_error(sstr.str()));
                }
            }
        }

        /****************************************************************************************
            The handling of single track events differs slightly
            Global ticks is the nb of ticks since the beginning of the single track.
            Its used to properly pad events with silences if required, and properly
            calculate the delta time of each events on each separate tracks.
        ****************************************************************************************/
        void HandleSingleTrackEvent(const jdksmidi::MIDITimedBigMessage& mev, ticks_t& globalticks)
        {
            ticks_t     evtglobaltick = mev.GetTime(); //The global absolute tick of the event
            unsigned char channel = (!mev.IsSystemExclusive() && !mev.IsMetaEvent())? mev.GetChannel() : 0; //Meta and sysex don't really have a channel
            TrkState& curstate = m_dsetrkstates[channel];
            MusicTrack& curtrk = m_dsetracks[channel];

            //Insert a pause if needed
            ticks_t delta = (evtglobaltick >= curstate.ticks_)? (evtglobaltick - curstate.ticks_) : 0; //If the event time is 0, we wanna avoid overflows
            if (delta != 0)
                HandlePauses(mev, curstate, curtrk, globalticks, delta);

            //After the pause was handled, deal with the event
            if (mev.IsSystemExclusive())
                HandleSysExEvents(mev, globalticks);
            else if (mev.IsMetaEvent())
                HandleMetaEvents(mev, globalticks);
            else
                HandleEvent(mev, curstate, curtrk, globalticks);
        }


        /****************************************************************************************
            Handles the pause event insertion logic, based on the delta time an event happens at!
        ****************************************************************************************/
        void HandlePauses(const jdksmidi::MIDITimedBigMessage& mev,
            TrkState& state,
            MusicTrack& trk,
            ticks_t& globalticks,
            ticks_t                               delta)
        {
            //Check if we can handle the pause with a simple fixed duration pause event first!
            if (delta <= static_cast<ticks_t>(DSE::eTrkDelays::_half))
            {
                DSE::TrkEvent pauseev;
                auto          itfound = TicksToTrkDelayID.find(static_cast<uint8_t>(delta));
                ticks_t       newdelta = delta;

                //If we have an exact match, go for that!
                if (itfound != TicksToTrkDelayID.end())
                {
                    pauseev.evcode = DSE::TrkDelayToEvID.at(itfound->second);
                    newdelta = itfound->first;
                }
                else if (isAccurateTime()) //Otherwise, depending on our time conversion strategy, pick the closest time
                {
                    HandleLongPauseEvents(mev, state, trk, globalticks, delta);
                    return; //Just return if we handled it that way!
                }
                else
                {
                    auto closesttrkdelay = DSE::FindClosestTrkDelayID(static_cast<uint8_t>(delta));
                    pauseev.evcode = DSE::TrkDelayToEvID.at(closesttrkdelay.first);
                    newdelta = static_cast<uint8_t>(closesttrkdelay.first);
                }

                //Find out what delta we went for in the end!
                //Add the pause to the track state and global tick count
                globalticks += newdelta;
                state.ticks_ += newdelta;
                state.lastpause_ = newdelta;
                trk.push_back( std::move(pauseev) );
            }
            else
                HandleLongPauseEvents(mev, state, trk, globalticks, delta); //If delta time is too long, use a longer pause event!
        }

        /****************************************************************************************
        ****************************************************************************************/
        void HandleLongPauseEvents(const jdksmidi::MIDITimedBigMessage& mev,
            TrkState& state,
            MusicTrack& trk,
            ticks_t& globalticks,
            ticks_t                               delta)
        {

            if (delta == state.lastpause_) //Check if our last pause is the exact same duration, and use that if possible
            {
                DSE::TrkEvent pauseev;
                pauseev.evcode = static_cast<uint8_t>(DSE::eTrkEventCodes::RepeatLastPause);

                //Add the pause to the track state and global tick count
                globalticks += state.lastpause_;
                state.ticks_ += state.lastpause_;

                trk.push_back(std::move(pauseev));
            }
            else if ((delta < ((ticks_t)state.lastpause_ + std::numeric_limits<uint8_t>::max())) && (delta > state.lastpause_)) //Check if our last pause is shorter than the required delay, so we can just add to it
            {
                DSE::TrkEvent addpauseev;
                addpauseev.evcode = static_cast<uint8_t>(DSE::eTrkEventCodes::AddToLastPause);
                addpauseev.params.push_back(static_cast<uint8_t>((delta - state.lastpause_) & 0xFF));

                //Add the pause to the track state and global tick count
                globalticks += (ticks_t)state.lastpause_ + addpauseev.params.front();
                state.ticks_ += state.lastpause_ + addpauseev.params.front();
                state.lastpause_ = state.lastpause_ + addpauseev.params.front();
                trk.push_back(std::move(addpauseev));
            }
            else if (delta < std::numeric_limits<uint8_t>::max())
            {
                //Otherwise make a short pause event if the delay fits within a short pause
                DSE::TrkEvent shortpauseev;
                shortpauseev.evcode = static_cast<uint8_t>(DSE::eTrkEventCodes::Pause8Bits);
                shortpauseev.params.push_back(static_cast<uint8_t>(delta & 0xFF));

                //Add the pause to the track state and global tick count
                globalticks += delta;
                state.ticks_ += delta;
                state.lastpause_ = delta;
                trk.push_back(std::move(shortpauseev));
            }
            else if (delta < std::numeric_limits<uint16_t>::max())
            {
                //Otherwise make a long pause event
                DSE::TrkEvent longpauseev;
                longpauseev.evcode = static_cast<uint8_t>(DSE::eTrkEventCodes::Pause16Bits);
                longpauseev.params.push_back(static_cast<uint8_t>(delta & 0xFF));
                longpauseev.params.push_back(static_cast<uint8_t>((delta >> 8) & 0xFF));

                //Add the pause to the track state and global tick count
                globalticks += delta;
                state.ticks_ += delta;
                state.lastpause_ = delta;
                trk.push_back(std::move(longpauseev));
            }
            else 
            {
                //Make several pauses in a row!
                unsigned long long nbpauses = delta / std::numeric_limits<uint16_t>::max();
                if( ( delta % std::numeric_limits<uint16_t>::max() ) != 0 )
                    ++nbpauses;

                unsigned long long pauseleft = delta; //The nb of ticks to pause for remaining

                for( unsigned long long i = 0; i < nbpauses; ++nbpauses )
                {
                    if( pauseleft < numeric_limits<uint8_t>::max() )
                    {
                        //Use short pause
                        DSE::TrkEvent shortpauseev;
                        shortpauseev.evcode = static_cast<uint8_t>(DSE::eTrkEventCodes::Pause8Bits);
                        shortpauseev.params.push_back( static_cast<uint8_t>( pauseleft & 0xFF ) );

                        //Add the pause to the track state and global tick count
                        globalticks    += pauseleft;
                        state.ticks_   += pauseleft;
                        state.lastpause_ = pauseleft;
                        trk.push_back(std::move(shortpauseev) );

                        pauseleft = 0;
                    }
                    else
                    {
                        //Pick the longest pause we can
                        uint16_t curpause = pauseleft & numeric_limits<uint16_t>::max();

                        DSE::TrkEvent longpauseev;
                        longpauseev.evcode = static_cast<uint8_t>(DSE::eTrkEventCodes::Pause16Bits);
                        longpauseev.params.push_back( static_cast<uint8_t>(  curpause       & 0xFF ) );
                        longpauseev.params.push_back( static_cast<uint8_t>( (curpause >> 8) & 0xFF ) );

                        //Add the pause to the track state and global tick count
                        globalticks    += curpause;
                        state.ticks_   += curpause;
                        state.lastpause_ = curpause;
                        trk.push_back(std::move(longpauseev) );

                        //Use long pause
                        pauseleft -= curpause;
                    }
                }
            }
        }

        /****************************************************************************************
        ****************************************************************************************/
        void HandleEvent(const jdksmidi::MIDITimedBigMessage& mev, TrkState& state, MusicTrack& trk, ticks_t& globalticks)
        {
            using namespace jdksmidi;
            if (mev.IsControlChange())
                HandleControlChanges(mev, state, trk);
            else if (mev.IsNoteOn())
                HandleNoteOn(mev, state, trk);
            else if (mev.IsNoteOff())
                HandleNoteOff(mev, state, trk);
            else
            {
                std::string txtbuff;
                txtbuff.resize(64); //The MsgToText function requires a buffer of 64 characters. Because its really dumb..
                clog << "<!>- Ignored midi event: \n" << mev.MsgToText(&txtbuff.front()) << "\n";
            }
        }

        /****************************************************************************************
        ****************************************************************************************/
        void HandleNoteOn(const jdksmidi::MIDITimedBigMessage& mev, TrkState& state, MusicTrack& trk)
        {
            NoteOnData noteoninf;
            uint8_t      vel = mev.GetVelocity();
            uint8_t      note = mev.GetNote();
            uint8_t      targetoctave = (note / NbMidiKeysInOctave);
            int8_t       octavediff = state.octave_ - targetoctave;
            noteoninf.noteonnum = trk.size();
            noteoninf.noteonticks = mev.GetTime();
            state.noteson_.push_back(std::move(noteoninf));

            //See if we need to insert a setoctave event before this note!
            if (state.octave_ == -1 ||
                state.octave_ != -1 && (abs(octavediff) > 2 || octavediff < -1)) //If we haven't specified the initial octave yet
                InsertSetOctaveEvent(trk, state, targetoctave);

            //We need to insert a dse event, to reserve the play note event's spot!
            //We also assemble the play note event partially (Because we need to hit the note off event for this note to know its duration)
            DSE::TrkEvent noteonev;
            uint8_t octmod = (targetoctave + 2) - state.octave_;
            uint8_t key = (note % NbMidiKeysInOctave);
            noteonev.evcode = vel & 0x7F;
            noteonev.params.push_back(((octmod & 3) << 4) | (key & 0xF)); //Put the param1 without the parameter lenght for now!
            trk.push_back(std::move(noteonev));
        }

        /****************************************************************************************
        ****************************************************************************************/
        void HandleNoteOff(const jdksmidi::MIDITimedBigMessage& mev, TrkState& state, MusicTrack& trk)
        {
            //Ignore orphanned notes off events
            if (!state.noteson_.empty())
            {
                //Update DSE event that was reserved earlier!
                const NoteOnData& noteonev = state.noteson_.front();
                ticks_t              noteduration = abs(static_cast<long>(mev.GetTime() - noteonev.noteonticks));

                assert(noteonev.noteonnum < trk.size());
                assert(!(trk[noteonev.noteonnum].params.empty()));

                uint8_t paramlenby = 0;

                if (state.lasthold_ != noteduration) //If the note duration has changed since the last note, append it!
                {
                    if ((noteduration & 0x00FF0000) > 0)
                        paramlenby = 3;
                    else if ((noteduration & 0x0000FF00) > 0)
                        paramlenby = 2;
                    else if ((noteduration & 0x000000FF) > 0)
                        paramlenby = 1;

                    state.lasthold_ = static_cast<uint32_t>(noteduration); //Update last note duration
                }

                trk[noteonev.noteonnum].params.front() |= (paramlenby & 3) << 6; //Add the nb of param bytes

                //Push the duration in ticks
                for (uint8_t cnt = 0; cnt < paramlenby; ++cnt)
                    trk[noteonev.noteonnum].params.push_back(paramlenby >> ((paramlenby - cnt) * 8));

                state.noteson_.pop_front();
            }
            else
                clog << mev.GetTime() << " - MIDI NoteOff event no preceeded by a NoteOn!";

        }

        void HandleSysExEvents(const jdksmidi::MIDITimedBigMessage& mev, ticks_t& globalticks)
        {
            //using namespace jdksmidi;
            //const MIDISystemExclusive * sysex = mev.GetSysEx();
            //assert(false);
        }

        void HandleMetaEvents(const jdksmidi::MIDITimedBigMessage& mev, ticks_t& globalticks)
        {
            using namespace jdksmidi;
            switch (mev.GetMetaType())
            {
            case META_MARKER_TEXT:
                HandleUnsupportedEvents(mev, globalticks);
                break;

            case META_TRACK_LOOP:
                HandleLoopPoint(mev, globalticks);
                break;

            case META_END_OF_TRACK:
                HandleEoT(mev, globalticks);
                break;

            case META_TEMPO:
                HandleSetTempo(mev, globalticks);
                break;

            case META_CHANNEL_PREFIX:
                assert(false); //#TODO: Implement channel prefix messages. (Makes a bunch of messages have their channel set without specifying on each events)
                break;

            case META_TIMESIG: //#TODO: This changes the midi tick rate
            //The following are meaningless to us
            case META_SEQUENCE_NUMBER:
            case META_GENERIC_TEXT:
            case META_COPYRIGHT:
            case META_TRACK_NAME:
            case META_INSTRUMENT_NAME:
            case META_LYRIC_TEXT:
            case META_CUE_POINT:
            case META_PROGRAM_NAME:
            case META_DEVICE_NAME:
            case META_GENERIC_TEXT_A:
            case META_GENERIC_TEXT_B:
            case META_GENERIC_TEXT_C:
            case META_GENERIC_TEXT_D:
            case META_GENERIC_TEXT_E:
            case META_GENERIC_TEXT_F:
            case META_OUTPUT_CABLE:
            case META_SMPTE:
            case META_KEYSIG:
            case META_SEQUENCER_SPECIFIC:
            default:
                {
                    std::string buf;
                    buf.resize(128);
                    mev.MsgToText(&(buf.front()));
                    clog <<mev.GetTime() <<" unsupported midi event " << buf;
                    break;
                }
            };
        }

        /****************************************************************************************
            HandleUnsupportedEvents
                Handles parsing MIDI text events for obtaining the DSE event stored in them.
        *****************************************************************************************/
        void HandleUnsupportedEvents(const jdksmidi::MIDITimedBigMessage& mev, ticks_t& globalticks)
        {
            using namespace jdksmidi;
            using namespace Poco::JSON;
            using Poco::Dynamic::Var;

            //If its a marker text meta-event, with a sysex, we try parsing what's inside for possible text DSE events
            if (mev.GetMetaType() != META_MARKER_TEXT || mev.GetSysEx() == nullptr || mev.GetSysEx()->GetLength() <= 0)
                return;

            //First, copy the text data from the event's sysex
            const MIDISystemExclusive& txtdat = *(mev.GetSysEx());
            string                      evtxt;
            copy_n(txtdat.GetBuf(), txtdat.GetLength(), back_inserter(evtxt));

            //Try to see if its a DSE event in text form. If it is, parse it!
            size_t found = evtxt.find(TXT_DSE_Event);
            if (found == string::npos)
                return;

            //Parse the event's JSON
            Object::Ptr jsonobj = Parser().parse(evtxt).extract<Object::Ptr>();
            if (!jsonobj || !jsonobj->has(TXT_DSE_Event) || !jsonobj->has("id"))
                return;
            uint8_t    evid = static_cast<uint8_t>(jsonobj->getValue<unsigned int>("id"));
            uint8_t    evchan = jsonobj->has("chan")? static_cast<uint8_t>(jsonobj->getValue<unsigned int>("chan")) : 0xFFui8;
            Array::Ptr args   = jsonobj->getArray("params");
            
            DSE::TrkEvent dsev;
            dsev.evcode = evid;
            if (args)
            {
                for (Var entry : *args)
                {
                    if (!entry.isInteger())
                        throw std::runtime_error("Encountered a bad arg type while parsing a DSE event. Event string: \"" + evtxt + "\"");
                    dsev.params.push_back(entry.convert<uint8_t>());
                }
            }

            //Validate channel id
            if (evchan >= NB_DSETracks)
            {
                clog << "<!>- Ignored text DSE event because channel/track specified was invalid!! (" << static_cast<uint16_t>(evchan) << ") :\n"
                    << "\t" << evtxt << "\n";
                return;
            }

            //Insert parsed event
            m_dsetracks[evchan].push_back(std::move(dsev));
        }

        /****************************************************************************************
            HandleEoT
                Inserts the end of track marker.
        ****************************************************************************************/
        void HandleEoT(const jdksmidi::MIDITimedBigMessage& mev, ticks_t& globalticks)
        {
            for (size_t cnttrk = 0; cnttrk < m_dsetracks.size(); ++cnttrk)
            {
                ticks_t trktickdiff = (globalticks - m_dsetrkstates[cnttrk].ticks_); //The difference in ticks between the track's last tick and the current global tick
                ticks_t evtglobaltick = mev.GetTime(); //The global absolute tick of the event

                //Insert a pause if needed
                ticks_t delta = (evtglobaltick - m_dsetrkstates[cnttrk].ticks_);
                if (delta != 0)
                    HandlePauses(mev, m_dsetrkstates[cnttrk], m_dsetracks[cnttrk], globalticks, delta);

                InsertDSEEvent(m_dsetracks[cnttrk], eTrkEventCodes::EndOfTrack);
            }
        }

        void HandleControlChanges(const jdksmidi::MIDITimedBigMessage& mev, TrkState& state, MusicTrack& trk)
        {
            using namespace jdksmidi;
            switch (mev.GetController())
            {
            case C_LSB:
                InsertDSEEvent(trk, eTrkEventCodes::SetBankHighByte, mev.GetByte2());
                break;
            case C_GM_BANK:
                InsertDSEEvent(trk, eTrkEventCodes::SetBankLowByte, mev.GetByte2());
                break;
            case C_BALANCE:
            case C_PAN:
                HandlePanChanges(mev, state, trk);
                break;
            case C_EXPRESSION:
                InsertDSEEvent(trk, eTrkEventCodes::SetExpress, mev.GetByte2());
                break;
            case C_MAIN_VOLUME:
                InsertDSEEvent(trk, eTrkEventCodes::SetChanVol, mev.GetByte2());
                break;
            case eMidiCC::C_SoundReleaseTime: //0x48, // Sound Controller 3, default: Release Time
                InsertDSEEvent(trk, eTrkEventCodes::SetEnvRelease, mev.GetByte2());
                break;
            case eMidiCC::C_SoundAttackTime: //0x49, // Sound Controller 4, default: Attack Time
                InsertDSEEvent(trk, eTrkEventCodes::SetEnvAtkTime, mev.GetByte2());
                break;

            case C_MODULATION:
            case C_BREATH:
            case C_FOOT:
            case C_PORTA_TIME:
            case C_GENERAL_1:
            case C_GENERAL_2:
            case C_GENERAL_3:
            case C_GENERAL_4:
            case C_DAMPER:
            case C_PORTA:
            case C_SOSTENUTO:
            case C_SOFT_PEDAL:
            case 0x44://              0x44   Legato Footswitch
            case C_HOLD_2:
            case 0x46: //0x46, // Sound Controller 1, default: Sound Variation
            case 0x47: //0x47, // Sound Controller 2, default: Timbre/Harmonic Intens.
            case 0x4A: //0x4A, // Sound Controller 5, default: Brightness
            case 0x4B: // Sound Controller 6, default: Decay Time
            case 0x4C: // Sound Controller 7, default: Vibrato Rate
            case 0x4D: // Sound Controller 8, default: Vibrato Depth
            case 0x4E: // Sound Controller 9, default: Vibrato Delay
            case 0x4F: // Sound Controller 10, default undefined
            case C_GENERAL_5:
            case C_GENERAL_6:
            case C_GENERAL_7:
            case C_GENERAL_8:
            case 0x54://            0x54  Portamento Control
            case C_EFFECT_DEPTH:
            case C_TREMOLO_DEPTH:
            case C_CHORUS_DEPTH:
            case C_CELESTE_DEPTH:
            case C_PHASER_DEPTH:
            case C_DATA_ENTRY:
            case C_DATA_INC:
            case C_DATA_DEC:
            case C_NONRPN_LSB:
            case C_NONRPN_MSB:
            case C_RPN_LSB:
            case C_RPN_MSB:
            case C_ALL_SOUNDS_OFF:
            case C_RESET:
            case C_LOCAL:
            case C_ALL_NOTES_OFF:
            case C_OMNI_OFF:
            case C_OMNI_ON:
            case C_MONO:
            case C_POLY:
            default:
                clog << "Unsupported midi controller CC" << static_cast<unsigned int>(mev.GetController()) << ", Value: " << static_cast<unsigned int>(mev.GetByte1()) << "!";
                break;
            };
        }

        /****************************************************************************************
        ****************************************************************************************/
        void HandleSetTempo(const jdksmidi::MIDITimedBigMessage& mev, ticks_t& globalticks)
        {
            // ticks_t trktickdiff   = (globalticks - states[0].ticks); //The difference in ticks between the track's last tick and the current global tick
            ticks_t evtglobaltick = mev.GetTime(); //The global absolute tick of the event

            //Insert a pause if needed
            ticks_t delta = (evtglobaltick - m_dsetrkstates[0].ticks_);
            if (delta != 0)
                HandlePauses(mev, m_dsetrkstates[0], m_dsetracks[0], globalticks, delta);

            //
            uint8_t tempo = static_cast<uint8_t>(ConvertMicrosecPerQuarterNoteToBPM(mev.GetTempo()));
            InsertDSEEvent(m_dsetracks[0], eTrkEventCodes::SetTempo, { tempo });
        }

        void HandlePanChanges(const jdksmidi::MIDITimedBigMessage& mev, TrkState& state, MusicTrack& trk)
        {
            InsertDSEEvent(trk, eTrkEventCodes::SetChanPan, mev.GetByte2());
            state.trkpan_ = mev.GetByte2();
        }

        void HandlePitchBend(const jdksmidi::MIDITimedBigMessage& mev,
            TrkState& state,
            MusicTrack& trk)
        {
            InsertDSEEvent(trk, eTrkEventCodes::SetPitchBend, mev.GetBenderValue());
        }

        void HandleProgramChange(const jdksmidi::MIDITimedBigMessage& mev,
            TrkState& state,
            MusicTrack& trk)
        {
            InsertDSEEvent(trk, eTrkEventCodes::SetPreset, mev.GetPGValue());
        }

        void HandleLoopPoint(const jdksmidi::MIDITimedBigMessage& mev, ticks_t& globalticks)
        {
            const unsigned char chan = mev.GetChannel();
            DSE::MusicTrack& trk = m_dsetracks[chan];
            TrkState& state = m_dsetrkstates[chan];
            InsertDSEEvent(trk, eTrkEventCodes::LoopPointSet);
            state.looppoint_ = trk.size();
        }

    private:

        static void InsertDSEEvent(MusicTrack& trk, DSE::eTrkEventCodes evcode, std::vector<uint8_t>&& params)
        {
            DSE::TrkEvent dsev;
            dsev.evcode = static_cast<uint8_t>(evcode);
            dsev.params = std::move(params);
            trk.push_back(std::move(dsev));
        }

        static void InsertDSEEvent(MusicTrack& trk, DSE::eTrkEventCodes evcode, uint8_t param)
        {
            DSE::TrkEvent dsev;
            dsev.evcode = static_cast<uint8_t>(evcode);
            dsev.params.push_back(param);
            trk.push_back(std::move(dsev));
        }

        static void InsertDSEEvent(MusicTrack& trk, DSE::eTrkEventCodes evcode)
        {
            DSE::TrkEvent dsev;
            dsev.evcode = static_cast<uint8_t>(evcode);
            trk.push_back(std::move(dsev));
        }

        void InsertSetOctaveEvent(MusicTrack& trk, TrkState& state, uint8_t newoctave)
        {
            InsertDSEEvent(trk, DSE::eTrkEventCodes::SetOctave, { newoctave });
            state.octave_ = newoctave;
        }

    private:

        bool isAccurateTime()const
        {
            return m_bIsAccurateTime;
        }

    private:
        const string m_srcpath;
        bool         m_bIsAccurateTime;
    };

//======================================================================================
//  Functions
//======================================================================================
    /*************************************************************************************************
        MidiToSequence
            Converts a MIDI file into a DSE Sequence.
    *************************************************************************************************/
    MusicSequence MidiToSequence(const std::string& inmidi, std::optional<MusicSequence> seq)
    {
        if (utils::LibWide().isLogOn())
        {
            clog << "================================================================================\n"
                << "Converting MIDI " << inmidi << "to SMDL\n"
                << "================================================================================\n";
        }

        MusicSequence mseq = seq ? std::move(seq.value()) : MusicSequence();
        Poco::Path inxml = Poco::Path(inmidi).setExtension("xml");
        inxml.setBaseName(inxml.getBaseName() + "_seq");
        
        //Grab the xml file if its there, so we can properly fill our meta data
        if (Poco::File(inxml).exists())
        {
            clog << "Found sequence info xml file \"" << Poco::Path::transcode(inxml.toString()) <<"\"\n";
            pugi::xml_document doc;
            doc.load_file(Poco::Path::transcode(inxml.toString()).c_str());
            DSE::ParseDSEXmlNode(doc, &mseq.metadata());
            std::vector<seqinfo_table> seqs;
            DSE::ParseSequenceInfo(doc, seqs);
            mseq.setSeqinfo(seqs.front()); //Music sequences always only have a single seqinfo
        }

        return MIDIToDSE(inmidi)(std::move(mseq));
    }
};