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
    public:

        /*
            - bNoDeltaTimeRounding : If this is true, the program won't try to round delta-time between midi events to the
                                     closest matches in the DSE system.
        */
        MIDIToDSE(const string& srcmidi, bool bNoDeltaTimeRounding = true)
            :m_srcpath(srcmidi), m_bIsAccurateTime(bNoDeltaTimeRounding), m_Tempo(120)
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
            m_Tempo = 120;

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

            //#3 - Do some post-processing stuff

            //#TODO: 
            //#TODO: Make sure that tempo changes are all on track 0
            //#TODO: Add Pause events to track 0 from the last tempo event to the end of track

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
            //ticks_t                ticks = 0;
            const int              nbev = mtrack->GetNumEvents();

            //Iterate through events
            for (int cntev = 0; cntev < nbev; ++cntev)
            {
                const MIDITimedBigMessage* ptrev = mtrack->GetEvent(cntev);
                if (ptrev == nullptr)
                    continue;

                try
                {
                    HandleSingleTrackEvent(mtrack, cntev, *ptrev);
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
        void HandleSingleTrackEvent(const MIDITrack* mtrack, int cntev, const jdksmidi::MIDITimedBigMessage& mev)
        {
            ticks_t       evtime   = mev.GetTime(); //The global absolute tick of the event
            unsigned char channel  = (!mev.IsSystemExclusive() && !mev.IsMetaEvent())? (mev.GetChannel() + 1) : 0; //Add one to channel, so we free Track 0

            //if (mev.IsSystemExclusive())
                //HandleSysExEvents(mtrack, cntev, mev, evtime);
            /*else*/ if (mev.IsMetaEvent())
                HandleMetaEvents(mtrack, cntev, mev, evtime);
            else
                HandleEvent(mtrack, cntev, mev, channel, evtime);
        }

        /****************************************************************************************
        ****************************************************************************************/
        void HandleEvent(const MIDITrack* mtrack, int cntev, const jdksmidi::MIDITimedBigMessage& mev, size_t trkid, ticks_t evtime)
        {
            using namespace jdksmidi;
            TrkState& state = m_dsetrkstates[trkid];
            MusicTrack& trk = m_dsetracks[trkid];

            if (mev.IsControlChange())
                state.ticks_ += HandleControlChanges(mev, trkid, evtime);
            else if (mev.IsProgramChange())
                state.ticks_ += HandleProgramChange(mev, trkid, evtime);
            else if (mev.IsPitchBend())
                state.ticks_ += HandlePitchBend(mev, trkid, evtime);
            else if (mev.IsNoteOn())
                state.ticks_ += HandleNoteOn(mtrack, cntev, mev, trkid);
            else
            {
                std::string txtbuff;
                txtbuff.resize(64); //The MsgToText function requires a buffer of 64 characters. Because its really dumb..
                clog << "<!>- Ignored midi event: \n" << mev.MsgToText(&txtbuff.front()) << "\n";
            }
        }

        /****************************************************************************************
        ****************************************************************************************/
        uint32_t HandleNoteOn(const MIDITrack* mtrack, int cntev, const jdksmidi::MIDITimedBigMessage& mev, size_t trkid)
        {
            TrkState&   state = m_dsetrkstates[trkid];
            MusicTrack& trk = m_dsetracks[trkid];
            const int numevent = mtrack->GetNumEvents();
            const jdksmidi::MIDITimedBigMessage * evnoteoff = nullptr;

            //Find noteoff
            for (int cntsearch = cntev; cntsearch < numevent; ++cntsearch)
            {
                const MIDITimedBigMessage* ptrev = mtrack->GetEvent(cntsearch);
                if (ptrev == nullptr)
                    continue;
                if (ptrev->IsNoteOff() && (ptrev->GetChannel() == mev.GetChannel()) && (ptrev->GetNote() == mev.GetNote()))
                {
                    evnoteoff = ptrev;
                    break;
                }
            }

            if (evnoteoff == nullptr)
                return 0;

            MIDIClockTime holdtime = evnoteoff->GetTime() - mev.GetTime();
            if(holdtime > numeric_limits<unsigned int>::max()) //This should never happen, since it would be completely unsupported
                throw std::overflow_error("The time difference between a note on and off event was longer than the maximum value of a 32 bits integer. (" + to_string(holdtime) + ")!");
            
            uint32_t dttrack = (mev.GetTime() - state.ticks_);
            return InsertNoteEvent(trkid, dttrack, mev.GetNote(), mev.GetVelocity(), (uint32_t)holdtime);
        }

        /****************************************************************************************
        ****************************************************************************************/
        //void HandleNoteOff(const jdksmidi::MIDITimedBigMessage& mev, TrkState& state, MusicTrack& trk)
        //{
        //    //Ignore orphanned notes off events
        //    if (!state.noteson_.empty())
        //    {
        //        //Update DSE event that was reserved earlier!
        //        const NoteOnData& noteonev = state.noteson_.front();
        //        ticks_t              noteduration = abs(static_cast<long>(mev.GetTime() - noteonev.noteonticks));

        //        assert(noteonev.noteonnum < trk.size());
        //        assert(!(trk[noteonev.noteonnum].params.empty()));

        //        uint8_t paramlenby = 0;

        //        if (state.lasthold_ != noteduration) //If the note duration has changed since the last note, append it!
        //        {
        //            if ((noteduration & 0x00FF0000) > 0)
        //                paramlenby = 3;
        //            else if ((noteduration & 0x0000FF00) > 0)
        //                paramlenby = 2;
        //            else if ((noteduration & 0x000000FF) > 0)
        //                paramlenby = 1;

        //            state.lasthold_ = static_cast<uint32_t>(noteduration); //Update last note duration
        //        }

        //        trk[noteonev.noteonnum].params.front() |= (paramlenby & 3) << 6; //Add the nb of param bytes

        //        //Push the duration in ticks
        //        for (uint8_t cnt = 0; cnt < paramlenby; ++cnt)
        //            trk[noteonev.noteonnum].params.push_back(paramlenby >> ((paramlenby - cnt) * 8));

        //        state.noteson_.pop_front();
        //    }
        //    else
        //        clog << mev.GetTime() << " - MIDI NoteOff event no preceeded by a NoteOn!";

        //}

        void HandleSysExEvents(const MIDITrack* mtrack, int cntev, const jdksmidi::MIDITimedBigMessage& mev, ticks_t evtime)
        {
            using namespace jdksmidi;
            const MIDISystemExclusive * sysex = mev.GetSysEx();
            assert(false);
        }

        void HandleMetaEvents(const MIDITrack* mtrack, int cntev, const jdksmidi::MIDITimedBigMessage& mev, ticks_t evtime)
        {
            using namespace jdksmidi;
            switch (mev.GetMetaType())
            {
            case META_MARKER_TEXT:
                HandleUnsupportedEvents(mtrack, cntev, mev, evtime);
                break;

            case META_TRACK_LOOP:
                InsertLoopPoint(evtime);
                break;

            case META_END_OF_TRACK:
                InsertEndOfTrack(evtime);
                break;

            case META_TEMPO:
                InsertSetTempo(evtime, mev.GetTempo());
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
        void HandleUnsupportedEvents(const MIDITrack* mtrack, int cntev, const jdksmidi::MIDITimedBigMessage& mev, ticks_t evtime)
        {
            using namespace jdksmidi;
            using namespace Poco::JSON;
            using Poco::Dynamic::Var;

            //If its a marker text meta-event, with a sysex, we try parsing what's inside for possible text DSE events
            if (mev.GetMetaType() != META_MARKER_TEXT || mev.GetSysEx() == nullptr || mev.GetSysEx()->GetLength() <= 0)
                return;

            //First, copy the text data from the event's sysex
            const MIDISystemExclusive& txtdat = *(mev.GetSysEx());
            string                     evtxt(txtdat.GetBuf(), txtdat.GetBuf() + txtdat.GetLength());
            if (size_t first = string::npos; (first = evtxt.find_first_of('{')) == string::npos || evtxt.find_first_of('}', first) == string::npos)
                return; //Try to filter out non-json stuff before actually having to deal with exceptions from the parser, because exceptions are really slow

            //Parse the event's JSON
            Var parsed;
            try 
            { 
                parsed = Parser().parse(evtxt);
            }
            catch (const Poco::JSON::JSONException&)
            {
                //Literally any text markers that's not parseable json...
                return; //poco doesn't let you check if the text is valid json, it just throws an exception
            }
            Object::Ptr jsonobj = parsed.extract<Object::Ptr>();
            if (!jsonobj || !jsonobj->has("id"))
                return;

            //Parse the event json
            uint8_t       evid   = static_cast<uint8_t>(jsonobj->getValue<unsigned int>("id"));
            uint8_t       evchan = jsonobj->has("chan")? static_cast<uint8_t>(jsonobj->getValue<unsigned int>("chan")) : 0xFFui8;
            Array::Ptr    args   = jsonobj->getArray("params");
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

            HandleParsedDseEvent(mtrack, cntev, evtime, evchan, std::move(dsev));
        }

        void HandleParsedDseEvent(const MIDITrack* mtrack, int cntev, ticks_t evtime, uint8_t evchan, DSE::TrkEvent && ev)
        {
            TrkState& state     = m_dsetrkstates[evchan];
            MusicTrack& trk     = m_dsetracks[evchan];
            uint32_t delta_time = (evtime - state.ticks_);

            switch ((eTrkEventCodes)ev.evcode)
            {
            case eTrkEventCodes::LoopPointSet:
                InsertLoopPoint(evtime);
                break;

            //
            case eTrkEventCodes::SweepTrackVol:
            case eTrkEventCodes::FadeSongVolume:
            case eTrkEventCodes::SweepTrkPan:
            //
            case eTrkEventCodes::SetFTune:
            case eTrkEventCodes::AddFTune:
            case eTrkEventCodes::SetCTune:
            case eTrkEventCodes::AddCTune:
            case eTrkEventCodes::SweepTune:
            case eTrkEventCodes::SetRndNoteRng:
            //
            case eTrkEventCodes::DisableEnvelope:
            case eTrkEventCodes::SetEnvAtkLvl:
            case eTrkEventCodes::SetEnvHold:
            case eTrkEventCodes::SetEnvDecSus:
            case eTrkEventCodes::SetEnvFade:
            // --- LFO Controls ---
            //LFO1
            case eTrkEventCodes::SetLFO1:
            case eTrkEventCodes::SetLFO1DelayFade:
            case eTrkEventCodes::RouteLFO1ToPitch:
            //LFO2
            case eTrkEventCodes::SetLFO2:
            case eTrkEventCodes::SetLFO2DelFade:
            case eTrkEventCodes::RouteLFO2ToVol:
            //LFO3
            case eTrkEventCodes::SetLFO3:
            case eTrkEventCodes::SetLFO3DelFade:
            case eTrkEventCodes::RouteLFO3ToPan:
            //LFO
            case eTrkEventCodes::SetLFO:
            case eTrkEventCodes::SetLFODelFade:
            case eTrkEventCodes::SetLFOParam:
            case eTrkEventCodes::SetLFORoute:
            // --- Skip Bytes ---
            case eTrkEventCodes::SkipNextByte:
            case eTrkEventCodes::SkipNext2Bytes1:
            case eTrkEventCodes::SkipNext2Bytes2:
            //
            case eTrkEventCodes::RepeatFrom:
            case eTrkEventCodes::RepeatSegment:
            case eTrkEventCodes::AfterRepeat:
            {
                state.ticks_ += InsertDSEEvent(evchan, delta_time, (eTrkEventCodes)ev.evcode, move(ev.params));
                return;
            }

            default:
                clog << evtime << "t: unsupported encoded dse event " << ev << "\n";
                return;
            };
        }

        uint32_t HandleControlChanges(const jdksmidi::MIDITimedBigMessage& mev, size_t trkid, ticks_t evtime)
        {
            using namespace jdksmidi;
            TrkState& state = m_dsetrkstates[trkid];
            MIDIClockTime dt = evtime - state.ticks_;

            try
            {
                switch (mev.GetController())
                {
                case C_LSB:
                    return InsertSetBankHigh(trkid, (uint32_t)dt, mev.GetByte2());

                case C_GM_BANK:
                    return InsertSetBankLow(trkid, (uint32_t)dt, mev.GetByte2());

                case C_BALANCE:
                    return InsertSetChanPan(trkid, (uint32_t)dt, mev.GetByte2());

                case C_PAN:
                    return InsertSetTrkPan(trkid, (uint32_t)dt, mev.GetByte2());

                case C_EXPRESSION:
                    return InsertSetExpression(trkid, (uint32_t)dt, mev.GetByte2());

                case C_MAIN_VOLUME:
                    return InsertSetTrkVol(trkid, (uint32_t)dt, mev.GetByte2());

                case eMidiCC::C_SoundReleaseTime: //0x48, // Sound Controller 3, default: Release Time
                    return InsertDSEEvent(trkid, (uint32_t)dt, eTrkEventCodes::SetEnvRelease, { mev.GetByte2() });

                case eMidiCC::C_SoundAttackTime: //0x49, // Sound Controller 4, default: Attack Time
                    return InsertDSEEvent(trkid, (uint32_t)dt, eTrkEventCodes::SetEnvAtkTime, { mev.GetByte2() });

                case C_CELESTE_DEPTH:
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
            catch (...)
            {
                throw_with_nested(std::runtime_error("Error processing HandleControlChanges()."));
            }
            return 0;
        }

        uint32_t HandlePitchBend(const jdksmidi::MIDITimedBigMessage& mev, size_t trkid, ticks_t evtime)
        {
            TrkState& state = m_dsetrkstates[trkid];
            MIDIClockTime tm = (evtime - state.ticks_);
            return InsertSetPitchBend(trkid, (uint32_t)tm, mev.GetBenderValue());
        }

        uint32_t HandleProgramChange(const jdksmidi::MIDITimedBigMessage& mev, size_t trkid, ticks_t evtime)
        {
            TrkState& state = m_dsetrkstates[trkid];
            MIDIClockTime tm = (evtime - state.ticks_);
            return InsertSetProgram(trkid, (uint32_t)tm, mev.GetPGValue());
        }

    private:


        inline uint32_t InsertAddTrkPan(size_t trackid, uint32_t delta_time, uint8_t addpan)
        {
            m_dsetrkstates[trackid].trkpan_ += addpan;
            return InsertDSEEvent(trackid, delta_time, DSE::eTrkEventCodes::AddTrkPan, { addpan });
        }

        inline uint32_t InsertSetTrkPan(size_t trackid, uint32_t delta_time, uint8_t newpan)
        {
            if (m_dsetrkstates[trackid].trkpan_ == newpan)
                return 0;
            m_dsetrkstates[trackid].trkpan_ = newpan;
            return InsertDSEEvent(trackid, delta_time, DSE::eTrkEventCodes::SetTrkPan, { newpan });
        }

        inline uint32_t InsertSetChanPan(size_t trackid, uint32_t delta_time, uint8_t newpan)
        {
            if (m_dsetrkstates[trackid].chpan_ == newpan)
                return 0;
            m_dsetrkstates[trackid].trkpan_ = newpan;
            return InsertDSEEvent(trackid, delta_time, DSE::eTrkEventCodes::SetChanPan, { newpan });
        }

        // -- Vol --
        inline uint32_t InsertAddTrkVol(size_t trackid, uint32_t delta_time, uint8_t addvol)
        {
            m_dsetrkstates[trackid].trkvol_ += addvol;
            return InsertDSEEvent(trackid, delta_time, DSE::eTrkEventCodes::AddTrkVol, { addvol });
        }

        inline uint32_t InsertSetTrkVol(size_t trackid, uint32_t delta_time, uint8_t newvol)
        {
            if (m_dsetrkstates[trackid].trkvol_ == newvol)
                return 0;
            m_dsetrkstates[trackid].trkvol_ = newvol;
            return InsertDSEEvent(trackid, delta_time, DSE::eTrkEventCodes::SetTrkVol, { newvol });
        }

        inline uint32_t InsertSetChanVol(size_t trackid, uint32_t delta_time, uint8_t newvol)
        {
            if (m_dsetrkstates[trackid].chvol_ == newvol)
                return 0;
            m_dsetrkstates[trackid].chvol_ = newvol;
            return InsertDSEEvent(trackid, delta_time, DSE::eTrkEventCodes::SetChanVol, { newvol });
        }

        inline uint32_t InsertSetExpression(size_t trackid, uint32_t delta_time, uint8_t newvol)
        {
            if (m_dsetrkstates[trackid].expr_ == newvol)
                return 0;
            m_dsetrkstates[trackid].expr_ = newvol;
            return InsertDSEEvent(trackid, delta_time, DSE::eTrkEventCodes::SetExpress, { newvol });
        }

        // -- Prgm --
        inline uint32_t InsertSetProgram(size_t trackid, uint32_t delta_time, uint8_t prgm)
        {
            if (m_dsetrkstates[trackid].curprgm_ == prgm)
                return 0;
            m_dsetrkstates[trackid].curprgm_ = prgm;
            return InsertDSEEvent(trackid, delta_time, DSE::eTrkEventCodes::SetProgram, { prgm });
        }

        inline uint32_t InsertSetBank(size_t trackid, uint32_t delta_time, uint16_t bank)
        {
            if (m_dsetrkstates[trackid].curbank_ == bank)
                return 0;
            m_dsetrkstates[trackid].curbank_ = bank;
            return InsertDSEEvent(trackid, delta_time, DSE::eTrkEventCodes::SetBank, { (uint8_t)bank, (uint8_t)(bank >> 8) });
        }

        inline uint32_t InsertSetBankLow(size_t trackid, uint32_t delta_time, uint8_t banklow)
        {
            if ((m_dsetrkstates[trackid].curbank_ & 0x00FF) == banklow)
                return 0;
            m_dsetrkstates[trackid].curbank_ = (m_dsetrkstates[trackid].curbank_ & 0xFF00) | banklow;
            return InsertDSEEvent(trackid, delta_time, DSE::eTrkEventCodes::SetBankLowByte, { banklow });
        }
        
        inline uint32_t InsertSetBankHigh(size_t trackid, uint32_t delta_time, uint8_t bankhigh)
        {
            if (((m_dsetrkstates[trackid].curbank_ & 0xFF00) >> 8) == bankhigh)
                return 0;
            m_dsetrkstates[trackid].curbank_ = (m_dsetrkstates[trackid].curbank_ & 0x00FF) | ((uint16_t)bankhigh << 8);
            return InsertDSEEvent(trackid, delta_time, DSE::eTrkEventCodes::SetBankHighByte, { bankhigh });
        }

        inline uint32_t InsertSetOctave(size_t trackid, uint32_t delta_time, uint8_t newoctave)
        { 
            if (m_dsetrkstates[trackid].octave_ == newoctave)
                return 0;
            m_dsetrkstates[trackid].octave_ = newoctave;
            return InsertDSEEvent(trackid, delta_time, DSE::eTrkEventCodes::SetOctave, { newoctave });
        }

        inline uint32_t InsertSetTempo(MIDIClockTime absolute_time, uint8_t tempo)
        {
            TrkState&     state = m_dsetrkstates[0]; //Tempo is only on track 0
            MIDIClockTime mt    = absolute_time - state.ticks_;
            if (m_Tempo == tempo)
                return 0; //If we already set the tempo previously don't bother putting another event
            m_Tempo = tempo;

            if (mt > numeric_limits<uint32_t>::max())
                throw std::overflow_error("A set tempo event was set to be inserted at a time value larger than an unsigned 32 bits integer.");
            return state .ticks_ += InsertDSEEvent(0, (uint32_t)mt, DSE::eTrkEventCodes::SetTempo, {tempo});
        }

        inline uint32_t InsertSetPitchBend(size_t trackid, uint32_t delta_time, int16_t bend)
        {
            if (m_dsetrkstates[trackid].pitchbend_ == bend)
                return 0;
            m_dsetrkstates[trackid].pitchbend_ = bend;
            return InsertDSEEvent(0, delta_time, DSE::eTrkEventCodes::SetPitchBend, { (uint8_t)bend, (uint8_t)(bend >> 8) });
        }

        inline uint32_t InsertSetPitchBendRange(size_t trackid, uint32_t delta_time, uint8_t bendrng)
        {
            if (m_dsetrkstates[trackid].bendrng_ == bendrng)
                return 0;
            m_dsetrkstates[trackid].bendrng_ = bendrng;
            return InsertDSEEvent(0, delta_time, DSE::eTrkEventCodes::SetPitchBendRng, { bendrng });
        }

        inline void InsertLoopPoint(MIDIClockTime absolute_time)
        {
            for (size_t cnttrk = 0; cnttrk < m_dsetracks.size(); ++cnttrk)
            {
                TrkState& state = m_dsetrkstates[cnttrk];
                MIDIClockTime mt = absolute_time - state.ticks_;
                if (mt > numeric_limits<uint32_t>::max())
                    throw std::overflow_error("A loop point event was set to be inserted at a time value larger than an unsigned 32 bits integer.");
                state.ticks_ += InsertDSEEvent(cnttrk, (uint32_t)mt, DSE::eTrkEventCodes::LoopPointSet);
                state.looppoint_ = state.ticks_;
            }
        }

        inline void InsertEndOfTrack(MIDIClockTime absolute_time)
        {
            for (size_t cnttrk = 0; cnttrk < m_dsetracks.size(); ++cnttrk)
            {
                TrkState& state = m_dsetrkstates[cnttrk];
                MIDIClockTime mt = absolute_time - state.ticks_;
                if (mt > numeric_limits<uint32_t>::max())
                    throw std::overflow_error("A end of track event was set to be inserted at a time value larger than an unsigned 32 bits integer.");
                state.ticks_ += InsertDSEEvent(cnttrk, (uint32_t)mt, DSE::eTrkEventCodes::EndOfTrack);
            }
        }

    private:

        //Returns the smallest amount of bytes needed to hole the given note hold duration
        inline unsigned int GetSmallestHoldBytes(uint32_t value)
        {
            return (value <= numeric_limits<uint8_t>::max())? 1 : ((value <= numeric_limits<uint16_t>::max())? 2 : ((value <= int24max)? 3 : 4));
        }

        //Insert a play note event. Automatically handle octave and pauses
        uint32_t InsertNoteEvent(size_t trackid, uint32_t delta_time, midinote_t note, int8_t velocity = 127, uint32_t hold_time = 0)
        {
            try
            {
                MusicTrack& trk = m_dsetracks[trackid];
                TrkState& state = m_dsetrkstates[trackid];
                std::vector<uint8_t> params(1);
                uint32_t evduration = 0; //Keep track of any pause events we may insert

                //Get Octave Diff
                uint8_t newoctave = (note / 12);
                uint8_t noteid = (note % 12);
                int     octavediff = (int)newoctave - (int)state.octave_;

                //For large differences insert a separate event
                if ((octavediff >= 2) || (octavediff < -2))
                {
                    evduration += InsertSetOctave(trackid, delta_time, newoctave);
                    octavediff = NoteEvOctaveShiftRange; //0 octave change is 2
                }
                else if ((octavediff < 2) && (octavediff >= -2))
                    octavediff = (octavediff + NoteEvOctaveShiftRange) & 3; //0 octave change is 2

                //Clamp and determine hold time
                hold_time = std::clamp(hold_time, (unsigned int)0, (unsigned int)int24max);
                if (hold_time == state.lasthold_)
                    hold_time = 0;               //Don't put a hold time if it's the same as the last time
                else if (hold_time != 0)
                    state.lasthold_ = hold_time; //In this case we're putting a hold time. Just ignore if it's 0.

                //Calc hold bytes
                uint8_t nbholdbytes = (hold_time > 0) ? GetSmallestHoldBytes(hold_time) : 0;

                //Calculate the first parameter byte
                params[0] = ((nbholdbytes & 3) << 6) | ((uint8_t)(octavediff & 3) << 4) | (noteid & NoteEvParam1NoteMask); //hold byte length is set up later

                //Insert the hold duration bytes if applicable
                for (size_t cnt = 0; cnt < nbholdbytes; ++cnt)
                    params.push_back((uint8_t)(hold_time >> ((nbholdbytes - 1) - cnt))); //Slap in all the hold bytes in big endian order

                //Add the event
                return evduration += InsertDSEEvent(trackid, (delta_time - evduration), (eTrkEventCodes)(velocity & 0x7F), std::move(params));
            }
            catch (...)
            {
                throw_with_nested(std::runtime_error("Error while inserting note event"));
            }
        }

        //Insert a track event at the time position specified and inserts pauses as needed
        uint32_t InsertDSEEvent(size_t trackid, uint32_t delta_time, DSE::eTrkEventCodes evcode, std::vector<uint8_t>&& params = {})
        {
            MusicTrack& trk = m_dsetracks[trackid];
            TrkState& state = m_dsetrkstates[trackid];
            uint32_t    evduration = GetEventDuration(state.lastpause_, evcode, params); //Most events don't have a duration

            assert(state.ticks_ <= (state.ticks_ + delta_time));
            assert((delta_time - evduration) >= 0);
            if (delta_time > 0)
                evduration += InsertPause(trackid, delta_time - evduration);

            trk.push_back(DSE::TrkEvent{ static_cast<uint8_t>(evcode), std::move(params) });
            return evduration;
        }

        //For a given time interval, insert the appropriate pause event
        uint32_t InsertPause(size_t trackid, uint32_t duration)
        {
            MusicTrack& trk   = m_dsetracks[trackid];
            TrkState&   state = m_dsetrkstates[trackid];

            //First try somthing simple
            if (state.lastpause_ > 0 && duration == state.lastpause_)
            {
                trk.push_back(DSE::TrkEvent{ (uint8_t)eTrkEventCodes::RepeatLastPause });
                return state.lastpause_;
            }
            else if ((duration <= (uint32_t)eTrkDelays::_half))
            {
                auto itfound = TicksToTrkDelayID.find(duration);
                if (itfound != TicksToTrkDelayID.end())
                {
                    trk.push_back(DSE::TrkEvent{ (uint8_t)itfound->second });
                    return state.lastpause_ = duration; //Delays also count as pauses
                }
            }
            
            //Check if we're in range for pausing 
            uint32_t diff_last = (uint32_t)std::abs((long long)duration - state.lastpause_);
            if (diff_last < std::numeric_limits<uint8_t>::max())
            {
                trk.push_back(DSE::TrkEvent{ (uint8_t)eTrkEventCodes::AddToLastPause, { (uint8_t)diff_last } });
                return state.lastpause_ = duration;
            }

            //Otherwise we have to go with a full blown pause event
            if (duration <= std::numeric_limits<uint8_t>::max())
            {
                trk.push_back(DSE::TrkEvent{ (uint8_t)eTrkEventCodes::Pause8Bits, { (uint8_t)duration } });
                return state.lastpause_ = duration;
            }
            else if (duration <= std::numeric_limits<uint16_t>::max())
            {
                trk.push_back(DSE::TrkEvent{ (uint8_t)eTrkEventCodes::Pause16Bits, { (uint8_t)duration, (uint8_t)(duration >> 8) } });
                return state.lastpause_ = duration;
            }
            else if (duration <= std::numeric_limits<uint16_t>::max() + std::numeric_limits<uint8_t>::max()) //24bits
            {
                trk.push_back(DSE::TrkEvent{ (uint8_t)eTrkEventCodes::Pause16Bits, { (uint8_t)duration, (uint8_t)(duration >> 8), (uint8_t)(duration >> 16) } });
                return state.lastpause_ = duration;
            }
            
            //If we still can't fit the pause duration into a single pause event, use several.
            uint32_t durleft = duration;
            while (durleft > 0)
                durleft -= InsertPause(trackid, durleft);
            return duration; //Return the initial duration so it all gets added to the track's tick counter
        }

        //Tell the ticks an event will increase the tick counter by
        uint32_t GetEventDuration(uint32_t lastpause, eTrkEventCodes ev, const std::vector<uint8_t>& args)
        {
            if (ev <= eTrkEventCodes::NoteOnEnd || ev > eTrkEventCodes::PauseUntilRel)
                return 0; //Anything but delay or pause events have a duration of 0

            if (ev <= eTrkEventCodes::Delay_64N && ev > eTrkEventCodes::NoteOnEnd)
                return (uint32_t)TrkDelayCodeVals.at((uint8_t)ev); //Fixed delays

            if (ev == eTrkEventCodes::RepeatLastPause)
                return lastpause;

            assert(args.size() >= 1);
            //Next for a given kind of pause return the duration
            switch (ev)
            {
            case eTrkEventCodes::AddToLastPause:
                {
                    return lastpause + args.front();
                }
            case eTrkEventCodes::Pause16Bits:
                {
                    assert(args.size() == 2);
                    return (uint16_t)(args[1] << 8 | args[0]);
                }
            case eTrkEventCodes::Pause24Bits:
                {
                    assert(args.size() == 3);
                    return (uint32_t)(args[2] << 16 | args[1] << 8 | args[0]);
                }
            }
            //Pause until release, and 8bits pause
            return args.front();
        }

    private:
        static const size_t int24max = numeric_limits<uint8_t>::max() + numeric_limits<uint16_t>::max();

        bool isAccurateTime()const
        {
            return m_bIsAccurateTime;
        }

    private:
        const string                    m_srcpath;
        bool                            m_bIsAccurateTime;
        MusicSequence                   m_seqOut;
        std::unique_ptr<MIDIMultiTrack> m_miditrks;
        std::map<uint8_t, MusicTrack>   m_dsetracks;
        std::map<uint8_t, TrkState>     m_dsetrkstates;
        uint8_t                         m_Tempo;
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