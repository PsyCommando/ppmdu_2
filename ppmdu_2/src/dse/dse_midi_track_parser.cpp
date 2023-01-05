#include <dse/dse_interpreter.hpp>
#include <dse/dse_to_xml.hpp>

#include <jdksmidi/world.h>
#include <jdksmidi/track.h>
#include <jdksmidi/multitrack.h>
#include <jdksmidi/filereadmultitrack.h>
#include <jdksmidi/fileread.h>
#include <jdksmidi/fileshow.h>
#include <jdksmidi/filewritemultitrack.h>

#include <Poco/Path.h>
#include <Poco/File.h>

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

            if (utils::LibWide().isLogOn())
            {
                clog << "------------------------\n"
                    << "Converting MIDI to DSE\n"
                    << "------------------------\n"
                    ;
            }

            //#1 - Load the MIDI
            MIDIFileReadStreamFile rs(m_srcpath.c_str());
            MIDIMultiTrack         tracks;
            MIDIFileReadMultiTrack track_loader(&tracks);
            MIDIFileRead           reader(&rs, &track_loader);
            MusicSequence          seq(startseq);

            if (!reader.Parse()) //Apparently handling errors with exceptions is too much to ask to jdksmidi :P
                throw runtime_error("JDKSMIDI: File parsing failed. Reason not specified..");

            if (utils::LibWide().isLogOn())
            {
                clog << "MIDI loaded! :\n"
                    << "\t" << m_srcpath << "\n"
                    << "\t\t" << "NbTracks : " << tracks.GetNumTracksWithEvents() << "\n"
                    << "\t\t" << "NbEvents : " << tracks.GetNumEvents() << "\n"
                    << "\n";
                ;
            }

            //#2 - Convert the MIDI to a sequence
            if(seq.metadata().fname.empty())
                seq.metadata().fname = utils::GetBaseNameOnly(m_srcpath);
            if(seq.metadata().origfname.empty())
                seq.metadata().origfname = Poco::Path::transcode(Poco::Path(seq.metadata().fname).setExtension("smd").toString()); //#TODO: Maybe try grabbing the original fname from the midi's metadata or something?
            seq.metadata().createtime.SetTimeToNow();

            ConvertMIDI(seq, tracks);
            return seq;
        }

    private:
        /****************************************************************************************
        ****************************************************************************************/
        void ConvertMIDI(MusicSequence & seq, const jdksmidi::MIDIMultiTrack& midi)
        {
            using namespace jdksmidi;
            map<uint8_t,MusicTrack> tracks;

            //Determine if multi-tracks
            const bool ismultitrk = midi.GetNumTracksWithEvents() > 1;

            if (ismultitrk)
                throw std::runtime_error("Input midi is multi-track format, which is unsupported!");
           

            //If single track, use the MIDI channel for each events to place them onto a specific track
            ConvertFromSingleTrackMidi(tracks, midi, seq.metadata());

            //Convert our track map into a track vector
            std::vector<MusicTrack> passedtrk;
            passedtrk.reserve(NB_DSETracks);
            uint8_t cntchan = 0;
            for (auto& entry : tracks)
            {
                entry.second.SetMidiChannel(cntchan++);
                if (((size_t)entry.first + 1) > passedtrk.size())
                    passedtrk.resize((size_t)entry.first + 1);
                passedtrk[entry.first] = std::move(entry.second);
            }
            seq.setTracks(std::move(passedtrk));
        }

        /****************************************************************************************
            ConvertFromMultiTracksMidi
        ****************************************************************************************/
        void ConvertFromMultiTracksMidi(vector<MusicTrack>& tracks,
            const jdksmidi::MIDIMultiTrack& midi,
            DSE::DSE_MetaDataSMDL& dseMeta)
        {
            using namespace jdksmidi;
            cerr << "Not implemented!\n";
            assert(false);
        }

        /****************************************************************************************
            ConvertFromSingleTrackMidi
        ****************************************************************************************/
        void ConvertFromSingleTrackMidi(map<uint8_t,MusicTrack>& tracks, const jdksmidi::MIDIMultiTrack& midi, DSE::DSE_MetaDataSMDL& dseMeta)
        {
            using namespace jdksmidi;
            if (midi.GetTrack(0) == nullptr)
                throw std::runtime_error("ConvertFromSingleTrackMidi(): JDKSMIDI: jdksmidi returned a null track ! wtf..");

            //Maintain a global tick count
            ticks_t              ticks = 0;
            const MIDITrack& mtrack = *(midi.GetTrack(0));
            const int            nbev = mtrack.GetNumEvents();
            map<uint8_t, TrkState> trkstates;
            //vector<TrkState>     trkstates;
            //trkstates.reserve(NB_DSETracks);//Pre-emptively pre-alloc all midi channels

            //Iterate through events
            for (int cntev = 0; cntev < nbev; ++cntev)
            {
                const MIDITimedBigMessage* ptrev = mtrack.GetEvent(cntev);
                if (ptrev == nullptr)
                    continue;

                if (ptrev->IsEndOfTrack())
                    HandleEoT(*ptrev, trkstates, tracks, ticks);
                else if (ptrev->IsMetaEvent())
                    HandleUnsupportedEvents(*ptrev, trkstates, tracks, dseMeta, ticks);
                else if (ptrev->IsTempo())
                    HandleSetTempo(*ptrev, trkstates, tracks, ticks); //Tempo can only go on track 0
                else
                    HandleSingleTrackEvent(*ptrev, tracks[ptrev->GetChannel()], trkstates[ptrev->GetChannel()], ticks);
            }

        }

        /****************************************************************************************
            The handling of single track events differs slightly
            Global ticks is the nb of ticks since the beginning of the single track.
            Its used to properly pad events with silences if required, and properly
            calculate the delta time of each events on each separate tracks.
        ****************************************************************************************/
        void HandleSingleTrackEvent(const jdksmidi::MIDITimedBigMessage& mev,
            MusicTrack& trk,
            TrkState& state,
            ticks_t& globalticks)
        {
            ticks_t trktickdiff = (globalticks - state.ticks_); //The difference in ticks between the track's last tick and the current global tick
            ticks_t evtglobaltick = mev.GetTime(); //The global absolute tick of the event

            //Insert a pause if needed
            ticks_t delta = (evtglobaltick - state.ticks_);
            if (delta != 0)
                HandlePauses(mev, state, trk, globalticks, delta);

            //After the delay was handled, deal with the event
            HandleEvent(mev, state, trk, globalticks);
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
                trk.push_back(pauseev);
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

                trk.push_back(pauseev);
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
                trk.push_back(addpauseev);
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
                trk.push_back(shortpauseev);
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
                trk.push_back(longpauseev);
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
                        trk.push_back( shortpauseev );

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
                        trk.push_back( longpauseev );

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
            else if (!mev.IsEndOfTrack())
            {
                std::string txtbuff;
                txtbuff.resize(64); //The MsgToText function requires a buffer of 64 characters. Because its really dumb..
                clog << "<!>- Ignored midi event: \n" << mev.MsgToText(&txtbuff.front()) << "\n";
            }
        }

        /****************************************************************************************
        ****************************************************************************************/
        void HandleNoteOn(const jdksmidi::MIDITimedBigMessage& mev,
            TrkState& state,
            MusicTrack& trk)
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
            trk.push_back(noteonev);
        }

        /****************************************************************************************
        ****************************************************************************************/
        void HandleNoteOff(const jdksmidi::MIDITimedBigMessage& mev,
            TrkState& state,
            MusicTrack& trk)
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
                    else if ((noteduration & 0x0000FF00) > 0)
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


        /****************************************************************************************
            HandleUnsupportedEvents
                Handles parsing MIDI text events for obtaining the DSE event stored in them.
        *****************************************************************************************/
        void HandleUnsupportedEvents(const jdksmidi::MIDITimedBigMessage& mev,
            map<uint8_t, TrkState>& states,
            map<uint8_t, MusicTrack>& trks,
            DSE_MetaDataSMDL& dsemeta,
            ticks_t& globalticks)
        {
            using namespace jdksmidi;

            //If its a marker text meta-event, with a sysex, we try parsing what's inside for possible text DSE events
            if (mev.GetMetaType() == META_MARKER_TEXT && mev.GetSysEx() != nullptr && mev.GetSysEx()->GetLength() > 0)
            {
                //First, copy the text data from the event's sysex
                const MIDISystemExclusive& txtdat = *(mev.GetSysEx());
                string                      evtxt;
                auto                        itins = back_inserter(evtxt);
                copy_n(txtdat.GetBuf(), txtdat.GetLength(), itins);

                //Try to see if its a DSE event in text form. If it is, parse it!
                auto found = evtxt.find(TXT_DSE_Event);
                if (found != string::npos)
                {
                    //Find all values in the text string
                    vector<uint8_t> values;
                    auto            itstrend = end(evtxt);
                    for (auto itstr = begin(evtxt); itstr != itstrend; ++itstr)
                    {
                        auto itnext = (itstr + 1);
                        if ((*itstr) == '0' && itnext != itstrend && (*itnext) == 'x')
                        {
                            uint32_t     value = 0;
                            stringstream sstrparse(string(itstr, itstrend));
                            sstrparse >> hex >> value;
                            values.push_back(value);
                        }
                    }

                    //If we found at least 2 values, interpret them
                    if (values.size() >= 2)
                    {
                        static const unsigned int ParamBegPos = 2; //The index at which params begin
                        DSE::TrkEvent dsev;
                        uint8_t       evchan = values.at(0); //Event channel
                        dsev.evcode = values.at(1); //Event id

                        //Validate channel id
                        if (evchan >= NB_DSETracks)
                        {
                            clog << "<!>- Ignored text DSE event because channel/track specified was invalid!! (" << static_cast<uint16_t>(evchan) << ") :\n"
                                << "\t" << evtxt << "\n";
                            return;
                        }

                        //Copy parameters
                        for (size_t i = ParamBegPos; i < values.size(); ++i)
                            dsev.params.push_back(values[i]);

                        //Insert parsed event
                        trks.at(evchan).push_back(dsev);
                    }
                    else
                    {
                        clog << "<!>- Ignored text DSE event because it was lacking a channel ID or/and even ID!! :\n"
                            << "\t" << evtxt << "\n";
                    }
                }
            }
        }

        /****************************************************************************************
            HandleEoT
                Inserts the end of track marker.
        ****************************************************************************************/
        void HandleEoT(const jdksmidi::MIDITimedBigMessage& mev,
            map<uint8_t, TrkState>& states,
            map<uint8_t, MusicTrack>& trks,
            ticks_t& globalticks)
        {

            for (size_t cnttrk = 0; cnttrk < trks.size(); ++cnttrk)
            {
                ticks_t trktickdiff = (globalticks - states[cnttrk].ticks_); //The difference in ticks between the track's last tick and the current global tick
                ticks_t evtglobaltick = mev.GetTime(); //The global absolute tick of the event

                //Insert a pause if needed
                ticks_t delta = (evtglobaltick - states[cnttrk].ticks_);
                if (delta != 0)
                    HandlePauses(mev, states[cnttrk], trks[cnttrk], globalticks, delta);

                InsertDSEEvent(trks[cnttrk], eTrkEventCodes::EndOfTrack);
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
                state.trkpan_ = mev.GetByte2();
                InsertDSEEvent(trk, eTrkEventCodes::SetChanPan, state.trkpan_);
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
        void HandleSetTempo(const jdksmidi::MIDITimedBigMessage& mev,
            map<uint8_t, TrkState>& states,
            map<uint8_t, MusicTrack>& trks,
            ticks_t& globalticks)
        {
            // ticks_t trktickdiff   = (globalticks - states[0].ticks); //The difference in ticks between the track's last tick and the current global tick
            ticks_t evtglobaltick = mev.GetTime(); //The global absolute tick of the event

            //Insert a pause if needed
            ticks_t delta = (evtglobaltick - states[0].ticks_);
            if (delta != 0)
                HandlePauses(mev, states[0], trks[0], globalticks, delta);

            //
            uint8_t tempo = static_cast<uint8_t>(ConvertMicrosecPerQuarterNoteToBPM(mev.GetTempo()));
            InsertDSEEvent(trks[0], eTrkEventCodes::SetTempo, { tempo });
        }

        void HandlePanChanges(const jdksmidi::MIDITimedBigMessage& mev,
            TrkState& state,
            MusicTrack& trk)
        {
            InsertDSEEvent(trk, eTrkEventCodes::SetChanPan, mev.GetByte1());
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


    private:

        static void InsertDSEEvent(MusicTrack& trk, DSE::eTrkEventCodes evcode, std::vector<uint8_t>&& params)
        {
            DSE::TrkEvent dsev;
            dsev.evcode = static_cast<uint8_t>(evcode);
            dsev.params = std::move(params);
            trk.push_back(dsev);
        }

        static void InsertDSEEvent(MusicTrack& trk, DSE::eTrkEventCodes evcode, uint8_t param)
        {
            DSE::TrkEvent dsev;
            dsev.evcode = static_cast<uint8_t>(evcode);
            dsev.params.push_back(param);
            trk.push_back(dsev);
        }

        static void InsertDSEEvent(MusicTrack& trk, DSE::eTrkEventCodes evcode)
        {
            DSE::TrkEvent dsev;
            dsev.evcode = static_cast<uint8_t>(evcode);
            trk.push_back(dsev);
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