#include <dse/dse_interpreter.hpp>

#include <dse/dse_to_xml.hpp>

#include <utils/pugixml_utils.hpp>

#include <jdksmidi/world.h>
#include <jdksmidi/track.h>
#include <jdksmidi/multitrack.h>
#include <jdksmidi/filereadmultitrack.h>
#include <jdksmidi/fileread.h>
#include <jdksmidi/fileshow.h>
#include <jdksmidi/filewritemultitrack.h>

#include <Poco/Path.h>

#include <pugixml.hpp>

#include <deque>
#include <numeric>

using namespace utils;
using namespace jdksmidi;
using namespace std;
using namespace pugi;
using namespace pugixmlutils;


namespace DSE
{
//======================================================================================
//  Utility
//======================================================================================
    void AddBankChangeMessage(jdksmidi::MIDITrack& outtrack, uint8_t channel, uint32_t time, bankid_t bank)
    {
        using namespace jdksmidi;
        MIDITimedBigMessage bankselMSB;
        MIDITimedBigMessage bankselLSB;

        bankselMSB.SetTime(time);
        bankselLSB.SetTime(time);

        bankselMSB.SetControlChange(channel, C_GM_BANK, static_cast<unsigned char>(bank & 0xFF));
        bankselLSB.SetControlChange(channel, C_LSB, static_cast<unsigned char>((bank >> 8) & 0xFF));
        outtrack.PutEvent(bankselMSB);
        outtrack.PutEvent(bankselLSB);
    }


//======================================================================================
// Sequence XML
//======================================================================================
    namespace SequenceXML
    {
        const std::string NODE_Sequences = "Sequences"s;
        const std::string NODE_Sequence  = "Sequence"s;

        const std::string ATTR_DSEVer = "dse_version"s;

        const std::string PROP_BankLow       = "BankLow"s;
        const std::string PROP_BankHigh      = "BankHigh"s;
        const std::string PROP_OrigFName     = "OriginalFname"s;
        const std::string PROP_OrigDate      = "OriginalDate"s;
        const std::string PROP_OrigLoadOrder = "LoadOrder"s;
        const std::string PROP_NbChannels    = "NbChannels"s;
        const std::string PROP_NbTracks      = "NbTracks"s;
    };
    class DSE_SeqXMLWriter
    {
    public:
        void operator()(const std::string& path, const DSE::MusicSequence& srcseq)
        {
            using namespace SequenceXML;
            xml_document doc;

            xml_node nodeSequences =doc.append_child(NODE_Sequences.c_str());
            xml_node nodeSequence = doc.append_child(NODE_Sequence.c_str());

            doc.save_file(path.c_str());
        }
    };

//======================================================================================
//  DSESequenceToMidi
//======================================================================================
        /*
            DSESequenceToMidi
                Convert a DSE event sequence to MIDI messages, and put them into the target
                file.
        */
    class DSESequenceToMidi
    {
    public:
        /***********************************************************************************
        ***********************************************************************************/
        DSESequenceToMidi(const std::string& outmidiname,
            const MusicSequence& seq,
            const SMDLPresetConversionInfo& remapdata,
            /*eMIDIFormat                        midfmt,*/
            eMIDIMode                          mode,
            uint32_t                           nbloops = 0)
            :m_fnameout(outmidiname), m_seq(seq)/*, m_midifmt(midfmt)*/, m_midimode(mode), m_nbloops(nbloops),
            m_bLoopBegSet(false), m_bTrackLoopable(false), m_songlsttick(0),
            m_convtable(&remapdata),
            m_hasconvtbl(true) //Enable conversion data
        {}

        /***********************************************************************************
        ***********************************************************************************/
        DSESequenceToMidi(const std::string& outmidiname,
            const MusicSequence& seq,
            /*eMIDIFormat                        midfmt,*/
            eMIDIMode                          mode,
            uint32_t                           nbloops = 0)
            :m_fnameout(outmidiname), m_seq(seq)/*, m_midifmt(midfmt)*/, m_midimode(mode), m_nbloops(nbloops),
            m_bLoopBegSet(false), m_bTrackLoopable(false), m_songlsttick(0),
            m_convtable(nullptr),
            m_hasconvtbl(false) //Disable conversion data
        {}

        /***********************************************************************************
            operator()
                Execute the conversion.
        ***********************************************************************************/
        void operator()()
        {
            using namespace jdksmidi;

            ExportAsSingleTrack();

            //Sort the tracks
            m_midiout.SortEventsOrder();

            //Then write the output!
            MIDIFileWriteStreamFileName out_stream(m_fnameout.c_str());

            if (out_stream.IsValid())
            {
                MIDIFileWriteMultiTrack writer(&m_midiout, &out_stream);

                // write the output file
                if (!writer.Write(1))
                    throw std::runtime_error("DSESequenceToMidi::operator(): JDKSMidi failed while writing the MIDI file!");
            }
            else
            {
                stringstream sstr;
                sstr << "DSESequenceToMidi::operator(): Couldn't open file " << m_fnameout << " for writing !";
                throw std::runtime_error(sstr.str());
            }
        }

    private:
        /***********************************************************************************
            HandleFixedPauses
                Handle converting the DSE Delta-time and turning into a midi time stamp.
        ***********************************************************************************/
        inline void HandleFixedPauses(const DSE::TrkEvent& ev,
            TrkState& state)
        {
            state.lastpause_ = static_cast<uint8_t>(TrkDelayCodeVals.at(ev.evcode));
            state.ticks_ += state.lastpause_;
        }

        /***********************************************************************************
            HandleSetPreset
                Converts DSE preset change events into MIDI bank select and MIDI patch
                select.
        ***********************************************************************************/
        void HandleSetPreset(const DSE::TrkEvent& ev,
            uint16_t                        trkno,
            uint8_t                         trkchan,
            TrkState& state,
            jdksmidi::MIDITimedBigMessage& mess,
            jdksmidi::MIDITrack& outtrack)
        {
            using namespace jdksmidi;

            //The program id as read from the event!
            uint8_t originalprgm = ev.params.front();

            //Check if we have to translate preset/bank ids
            if (m_hasconvtbl)
            {
                // -- Select the correct bank --
                auto itfound = m_convtable->FindConversionInfo(originalprgm);

                /*
                    Some presets in the SMD might actually not even exist! Several tracks in PMD2 have this issue
                    So to avoid crashing, verify before looking up a bank.
                */
                if (itfound != m_convtable->end()) //We found conversion info for the DSE preset
                {
                    state.hasinvalidbank = false;
                    state.curbank_ = itfound->second.midibank;
                    state.curprgm_ = itfound->second.midipres;
                    state.origdseprgm_ = originalprgm;
                    state.curmaxpoly_ = itfound->second.maxpoly;
                    state.transpose = itfound->second.transpose;

                    if (itfound->second.idealchan != std::numeric_limits<uint8_t>::max())
                    {
                        state.chanoverriden = true;
                        state.ovrchan_ = itfound->second.idealchan;
                    }
                }
                else
                {
                    //We didn't find any conversion info
                    state.hasinvalidbank = true;
                    state.curbank_ = 0x7F;           //Set to bank 127 to mark the error
                    state.curprgm_ = originalprgm; //Set preset as-is
                    state.origdseprgm_ = originalprgm;
                    state.curmaxpoly_ = -1;
                    state.transpose = 0;
                    state.chanoverriden = false;
                    state.ovrchan_ = std::numeric_limits<uint8_t>::max();

                    clog << "Couldn't find a matching bank for preset #"
                        << static_cast<short>(ev.params.front()) << " ! Setting to bank " << state.curbank_ << " !\n";
                }

            }
            else
            {
                //No need to translate anything
                //state.hasinvalidbank = false;
                //state.curbank_       =  0;
                state.curprgm_ = originalprgm; //Set preset as-is
                state.origdseprgm_ = originalprgm;
                state.curmaxpoly_ = -1;
                state.transpose = 0;
                state.chanoverriden = false;
                state.ovrchan_ = std::numeric_limits<uint8_t>::max();
            }

            //Change only if the preset/bank isn't overriden!
            if (!state.presetoverriden)
            {
                //Add the Bank select message
                AddBankChangeMessage(outtrack, m_seq[trkno].GetMidiChannel(), state.ticks_, state.curbank_);

                //Add the Program change message
                mess.SetProgramChange(static_cast<uint8_t>(trkchan), static_cast<uint8_t>(state.curprgm_));
                outtrack.PutEvent(mess);
            }

            //Clear the notes on buffer
            state.noteson_.clear();

            //Then disable/enable any effect controller 
            //#TODO: 
        }

        /***********************************************************************************
            HandleSoundBank
                Handle all events for setting the sound bank, and swdl to use.
        ***********************************************************************************/
        void HandleSoundBank(const DSE::TrkEvent& ev,
            uint16_t                        trkno,
            uint8_t                         trkchan,
            TrkState& state,
            jdksmidi::MIDITimedBigMessage& mess,
            jdksmidi::MIDITrack& outtrack)
        {
            using namespace jdksmidi;
            const DSE::eTrkEventCodes code = static_cast<DSE::eTrkEventCodes>(ev.evcode);

            if (code == eTrkEventCodes::SetBank)
            {
                state.curbank_ = (static_cast<bankid_t>(ev.params[0]) | static_cast<bankid_t>(ev.params[1]) << 8);
            }
            else if (code == eTrkEventCodes::SetBankHighByte)
            {
                state.curbank_ = (state.curbank_ & 0x00FFui16) | (static_cast<bankid_t>(ev.params[0]) << 8);
            }
            else if (code == eTrkEventCodes::SetBankLowByte)
            {
                state.curbank_ = (state.curbank_ & 0xFF00ui16) | (static_cast<bankid_t>(ev.params[0]));
            }
            AddBankChangeMessage(outtrack, m_seq[trkno].GetMidiChannel(), state.ticks_, state.curbank_);
        }

        /***********************************************************************************
            HandlePauses
                Handle all pause events.
        ***********************************************************************************/
        inline void HandlePauses(eTrkEventCodes        code,
            const DSE::TrkEvent& ev,
            TrkState& state)
        {
            if (code == eTrkEventCodes::Pause24Bits)
            {
                state.lastpause_ = (static_cast<uint32_t>(ev.params[2]) << 16) | (static_cast<uint32_t>(ev.params[1]) << 8) | ev.params[0];
                state.ticks_ += state.lastpause_;
            }
            else if (code == eTrkEventCodes::Pause16Bits)
            {
                state.lastpause_ = (static_cast<uint16_t>(ev.params.back()) << 8) | ev.params.front();
                state.ticks_ += state.lastpause_;
            }
            else if (code == eTrkEventCodes::Pause8Bits)
            {
                state.lastpause_ = ev.params.front();
                state.ticks_ += state.lastpause_;
            }
            else if (code == eTrkEventCodes::AddToLastPause)
            {
                int8_t value = static_cast<int8_t>(ev.params.front()); //The value is signed

                if (ev.params.front() >= state.lastpause_)
                {
                    state.lastpause_ = 0;

                    if (utils::LibWide().isLogOn())
                        clog << "Warning: AddToLastPause event addition resulted in a negative value! Clamping to 0!\n";
                }
                else
                    state.lastpause_ = state.lastpause_ + value;

                state.ticks_ += state.lastpause_;
            }
            else if (code == eTrkEventCodes::RepeatLastPause)
            {
                state.ticks_ += state.lastpause_;
            }
            else if (code == eTrkEventCodes::PauseUntilRel)
            {
#ifdef _DEBUG
                clog << "<!>- Error: Event 0x95 not yet implemented!\n";
                assert(false);
#else
                throw runtime_error("DSESequenceToMidi::HandlePauses() : Event 0x95 not yet implemented!\n");
#endif
            }
        }

        /***********************************************************************************
            HandleEvent
                Main conditional structure for converting events from the DSE format into
                MIDI messages.
        ***********************************************************************************/
        void HandleEvent(uint16_t              trkno,
            uint8_t               trkchan,
            TrkState& state,
            const DSE::TrkEvent& ev,
            jdksmidi::MIDITrack& outtrack)
        {
            using namespace jdksmidi;
            MIDITimedBigMessage       mess;
            const DSE::eTrkEventCodes code = static_cast<DSE::eTrkEventCodes>(ev.evcode);

            //Log events if neccessary
            if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
                clog << setfill(' ') << setw(8) << right << state.ticks_ << "t : " << ev << setfill(' ') << setw(16) << right;

            //Handle Pauses then play notes, then anything else!
            if (code >= eTrkEventCodes::RepeatLastPause && code <= eTrkEventCodes::PauseUntilRel)
                HandlePauses(code, ev, state);
            else if (code >= eTrkEventCodes::Delay_HN && code <= eTrkEventCodes::Delay_64N)
                HandleFixedPauses(ev, state);
            else if (code >= eTrkEventCodes::NoteOnBeg && code <= eTrkEventCodes::NoteOnEnd)
                HandlePlayNote(trkno, trkchan, state, ev, outtrack);
            else
            {
                //Now that we've handled the pauses
                switch (code)
                {
                    //
                case eTrkEventCodes::SetTempo:
                case eTrkEventCodes::SetTempo2:
                {
                    mess.SetTempo(ConvertTempoToMicrosecPerQuarterNote(ev.params.front()));
                    mess.SetTime(state.ticks_);
                    outtrack.PutEvent(mess);
                    break;
                }

                // --- Octave ---
                case eTrkEventCodes::SetOctave:
                {
                    int8_t newoctave = ev.params.front();
                    if (newoctave > DSE_MaxOctave)
                        clog << "New octave value set is too high !" << static_cast<unsigned short>(newoctave) << "\n";
                    state.octave_ = newoctave;
                    break;
                }
                case eTrkEventCodes::AddOctave:
                {
                    int8_t newoctave = ev.params.front();
                    newoctave += state.octave_;
                    if (newoctave > DSE_MaxOctave)
                        clog << "New octave value set is too high !" << static_cast<unsigned short>(newoctave) << "\n";
                    state.octave_ = newoctave;
                    break;
                }

                // --- Volume ---
                case eTrkEventCodes::SetExpress:
                {
                    mess.SetControlChange(trkchan, jdksmidi::C_EXPRESSION, ev.params.front());
                    mess.SetTime(state.ticks_);
                    outtrack.PutEvent(mess);
                    break;
                }
                case eTrkEventCodes::SetTrkVol:
                {
                    state.trkvol_ = std::clamp(static_cast<int8_t>(ev.params.front()), static_cast<int8_t>(0), std::numeric_limits<int8_t>::max()); //Always a positive value
                    mess.SetControlChange(trkchan, jdksmidi::C_MAIN_VOLUME, state.trkvol_);
                    mess.SetTime(state.ticks_);
                    outtrack.PutEvent(mess);
                    break;
                }
                case eTrkEventCodes::AddTrkVol:
                {
                    state.trkvol_ += static_cast<int8_t>(ev.params.front());
                    state.trkvol_ = std::clamp(state.trkvol_, static_cast<int8_t>(0), std::numeric_limits<int8_t>::max()); //Always a positive value
                    mess.SetControlChange(trkchan, jdksmidi::C_MAIN_VOLUME, state.trkvol_);
                    mess.SetTime(state.ticks_);
                    outtrack.PutEvent(mess);
                    break;
                }
                case eTrkEventCodes::SweepTrackVol:
                {
                    //TODO: Figure something out for this one. Sweeping the track's volume is kinda tricky to handle if we wanna convert the midis back after. 
                    //Note: Maybe check if this is actually something related to legato?
                    if (ShouldMarkUnsupported())
                        HandleUnsupported(ev, trkno, state, mess, outtrack);
                    break;
                }

                case eTrkEventCodes::SetChanVol: //Channel Pressure?
                {
                    //Attempting channel pressure
                    mess.SetChannelPressure(trkchan, ev.params.front());
                    mess.SetTime(state.ticks_);
                    outtrack.PutEvent(mess);
                    break;
                }
                case eTrkEventCodes::FadeSongVolume:
                {
                    if (ShouldMarkUnsupported())
                        HandleUnsupported(ev, trkno, state, mess, outtrack);
                    break;
                }
                case eTrkEventCodes::SetNoteVol: //Aftertouch?
                {
                    //Attempting aftertouch
                    uint8_t lastnote = !(state.noteson_.empty()) ? state.noteson_.front().noteid : 0;
                    mess.SetPolyPressure(trkchan, lastnote, ev.params.front());
                    outtrack.PutEvent(mess);
                    break;
                }

                // --- Pan/Balance ---
                case eTrkEventCodes::SetTrkPan:
                {
                    state.trkpan_ = static_cast<int8_t>(ev.params.front());
                    mess.SetControlChange(trkchan, jdksmidi::C_PAN, state.trkpan_);
                    mess.SetTime(state.ticks_);
                    outtrack.PutEvent(mess);
                    break;
                }
                case eTrkEventCodes::AddTrkPan:
                {
                    state.trkpan_ += static_cast<int8_t>(ev.params.front()); //They probably don't gate against overflows...
                    mess.SetControlChange(trkchan, jdksmidi::C_PAN, state.trkpan_);
                    mess.SetTime(state.ticks_);
                    outtrack.PutEvent(mess);
                    break;
                }
                case eTrkEventCodes::SweepTrkPan:
                {
                    if (ShouldMarkUnsupported())
                        HandleUnsupported(ev, trkno, state, mess, outtrack);
                    break;
                }
                case eTrkEventCodes::SetChanPan:
                {
                    //assert(false); //#TODO: Need to track pan for the whole channel + track

                    //TEST with balance!
                    mess.SetControlChange(trkchan, jdksmidi::C_BALANCE, ev.params.front());
                    mess.SetTime(state.ticks_);
                    outtrack.PutEvent(mess);
                    break;
                }

                // --- Pitch ---
                case eTrkEventCodes::SetPitchBend: //################### FIXME LATER ######################
                {
                    //NOTE: Pitch bend's range is implementation specific in MIDI. Though PMD2's pitch bend range may vary per program split
                    mess.SetPitchBend(trkchan, (static_cast<int16_t>(ev.params.front() << 8) | static_cast<int16_t>(ev.params.back())));
                    mess.SetTime(state.ticks_);
                    outtrack.PutEvent(mess);
                    break;
                }
                case eTrkEventCodes::SetPitchBendRng:
                {
                    MIDITimedBigMessage rpn1msg;
                    MIDITimedBigMessage rpn2msg;
                    //0x0 0x0 is pitch bend range
                    rpn1msg.SetControlChange(trkchan, jdksmidi::C_RPN_LSB, 0);
                    rpn1msg.SetTime(state.ticks_);
                    outtrack.PutEvent(rpn1msg);
                    rpn2msg.SetControlChange(trkchan, jdksmidi::C_RPN_MSB, 0);
                    rpn2msg.SetTime(state.ticks_);
                    outtrack.PutEvent(rpn2msg);

                    //Possibly pitch bend range?
                    mess.SetControlChange(trkchan, jdksmidi::C_DATA_ENTRY, ev.params.front());
                    mess.SetTime(state.ticks_);
                    outtrack.PutEvent(mess);
                    break;
                }
                case eTrkEventCodes::SetFTune:
                case eTrkEventCodes::AddFTune:
                case eTrkEventCodes::SetCTune:
                case eTrkEventCodes::AddCTune:
                case eTrkEventCodes::SweepTune:
                case eTrkEventCodes::SetRndNoteRng:
                {
                    if (ShouldMarkUnsupported())
                        HandleUnsupported(ev, trkno, state, mess, outtrack);
                    break;
                }

                case eTrkEventCodes::SetDetuneRng:
                {
                    //#TODO: This seems to be wrong. detune only takes a byte of value, but the event's params are 2 bytes
                    mess.SetControlChange(trkchan, jdksmidi::C_CELESTE_DEPTH, ev.params.front());
                    mess.SetTime(state.ticks_);
                    outtrack.PutEvent(mess);
                    break;
                }

                // --- Volume Envelope Controls ---
                case eTrkEventCodes::DisableEnvelope:
                {
                    if (ShouldMarkUnsupported())
                        HandleUnsupported(ev, trkno, state, mess, outtrack);
                    break;
                }
                case eTrkEventCodes::SetEnvAtkLvl:
                {
                    if (ShouldMarkUnsupported())
                        HandleUnsupported(ev, trkno, state, mess, outtrack);
                    break;
                }
                case eTrkEventCodes::SetEnvAtkTime:
                {
                    mess.SetControlChange(trkchan, static_cast<unsigned char>(eMidiCC::C_SoundAttackTime), ev.params.front());
                    mess.SetTime(state.ticks_);
                    outtrack.PutEvent(mess);
                    break;
                }
                case eTrkEventCodes::SetEnvHold:
                {
                    if (ShouldMarkUnsupported())
                        HandleUnsupported(ev, trkno, state, mess, outtrack);
                    break;
                }
                case eTrkEventCodes::SetEnvDecSus:
                {
                    if (ShouldMarkUnsupported())
                        HandleUnsupported(ev, trkno, state, mess, outtrack);
                    break;
                }
                case eTrkEventCodes::SetEnvFade:
                {
                    if (ShouldMarkUnsupported())
                        HandleUnsupported(ev, trkno, state, mess, outtrack);
                    break;
                }
                case eTrkEventCodes::SetEnvRelease:
                {
                    mess.SetControlChange(trkchan, static_cast<unsigned char>(eMidiCC::C_SoundReleaseTime), ev.params.front());
                    mess.SetTime(state.ticks_);
                    outtrack.PutEvent(mess);
                    break;
                }

                // --- LFO Controls ---
                case eTrkEventCodes::SetLFO1:
                case eTrkEventCodes::SetLFO1DelayFade:
                case eTrkEventCodes::RouteLFO1ToPitch:
                {
                    if (ShouldMarkUnsupported())
                        HandleUnsupported(ev, trkno, state, mess, outtrack);
                    break;
                }
                //LFO2
                case eTrkEventCodes::SetLFO2:
                case eTrkEventCodes::SetLFO2DelFade:
                case eTrkEventCodes::RouteLFO2ToVol:
                {
                    if (ShouldMarkUnsupported())
                        HandleUnsupported(ev, trkno, state, mess, outtrack);
                    break;
                }
                //LFO3
                case eTrkEventCodes::SetLFO3:
                case eTrkEventCodes::SetLFO3DelFade:
                case eTrkEventCodes::RouteLFO3ToPan:
                {
                    if (ShouldMarkUnsupported())
                        HandleUnsupported(ev, trkno, state, mess, outtrack);
                    break;
                }

                case eTrkEventCodes::SetLFO:
                case eTrkEventCodes::SetLFODelFade:
                case eTrkEventCodes::SetLFOParam:
                case eTrkEventCodes::SetLFORoute:
                {
                    if (ShouldMarkUnsupported())
                        HandleUnsupported(ev, trkno, state, mess, outtrack);
                    break;
                }


                //------------------ Program bank related events ------------------
                case eTrkEventCodes::SetProgram:
                {
                    mess.SetTime(state.ticks_);
                    HandleSetPreset(ev, trkno, trkchan, state, mess, outtrack);
                    break;
                }
                case eTrkEventCodes::SetBank:
                case eTrkEventCodes::SetBankHighByte:
                case eTrkEventCodes::SetBankLowByte:
                {
                    mess.SetTime(state.ticks_);
                    HandleSoundBank(ev, trkno, trkchan, state, mess, outtrack);
                    break;
                }

                //------------------ Repeat segment/loop events ------------------
                case eTrkEventCodes::LoopPointSet:
                {
                    if (m_nbloops == 0)
                    {
                        if (!m_bLoopBegSet)
                        {
                            //Only place an event if we don't loop the track via code, and haven't placed it already to avoid playbak issues
                            mess.SetMetaType(META_TRACK_LOOP);
                            mess.SetTime(state.ticks_);
                            outtrack.PutTextEvent(state.ticks_, META_MARKER_TEXT, TXT_LoopStart.c_str(), TXT_LoopStart.size());
                            outtrack.PutEvent(mess);
                        }
                    }

                    m_bTrackLoopable = true; //If we got a loop pos, then the track is loopable
                    m_bLoopBegSet = true;

                    //Mark the loop position
                    state.looppoint_ = ((size_t)state.eventno_ + 1);  //Add one to avoid re-processing the loop marker
                    m_beflooptrkstates[trkno] = state;                 //Save the track state
                    break;
                }

                // Since midi has no support whatsoever for these below, we're going to mark the points as they are
                case eTrkEventCodes::RepeatFrom: //"Dal Segno" symbol
                {
                    //#TODO: Will require some special implementation!
                    //#      Additionally, if ripping the midi not for playback it might be desirable to process the midis as close to original as possible!

                    if (ShouldMarkUnsupported())
                        HandleUnsupported(ev, trkno, state, mess, outtrack);
                    state.repeatmark_ = ((size_t)state.eventno_ + 1); //Make sure we don't start repeating on the same event
                    state.repeattimes_ = ev.params.front();
                    break;
                }
                case eTrkEventCodes::RepeatSegment: //"D.S." Becomes "D.S. to coda" if the after repeat is set?
                {
                    //#TODO: Will require some special implementation
                    if (ShouldMarkUnsupported())
                        HandleUnsupported(ev, trkno, state, mess, outtrack);
                    state.repeat_ = state.eventno_; //Make sure we don't end repeating on the same event
                    break;
                }
                case eTrkEventCodes::AfterRepeat: //"Coda"
                {
                    //#TODO: Will require some special implementation
                    if (ShouldMarkUnsupported())
                        HandleUnsupported(ev, trkno, state, mess, outtrack);
                    state.afterrepmark_ = ((size_t)state.eventno_ + 1);
                    break;
                }

                //------------------ Byte Skipping events! ------------------ 
                case eTrkEventCodes::SkipNextByte:
                case eTrkEventCodes::SkipNext2Bytes1:
                case eTrkEventCodes::SkipNext2Bytes2:
                {
                    //Mark them for science!
                    if (ShouldMarkUnsupported())
                        HandleUnsupported(ev, trkno, state, mess, outtrack);
                    break;
                }
                //------------------ Unsupported Events ------------------ 
                default:
                {
                    //Put a cue point to mark unsupported events and their parameters
                    if (ShouldMarkUnsupported())
                        HandleUnsupported(ev, trkno, state, mess, outtrack);
                }
                };
            }

            if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
                clog << "\n";

            //Event handling done, increment event counter
            state.eventno_ += 1;
        }

        /***********************************************************************************
            HandleUnsupported
                Handle the default handling for unsupported messages. Just put them in a
                wrapper event and output that.
        ***********************************************************************************/
        void HandleUnsupported(const DSE::TrkEvent& ev,
            uint16_t                        trkno,
            TrkState& state,
            jdksmidi::MIDITimedBigMessage& mess,
            jdksmidi::MIDITrack& outtrack)
        {
            using namespace jdksmidi;
            //Write it as json
            stringstream evmark;
            evmark << "{\"" << TXT_DSE_Event <<"\": 1, \"chan\": " <<dec << static_cast<unsigned short>(trkno)
                << ", \"id\": " << static_cast<unsigned short>(ev.evcode) <<", \"params\": [ ";

            //Then Write any parameters
            for (size_t i = 0; i < ev.params.size(); ++i)
            {
                evmark << static_cast<unsigned short>(ev.params[i]);
                if (i < (ev.params.size() - 1) )
                    evmark << ", ";
            }
            evmark << " ]}";

            const string mark = evmark.str();
            outtrack.PutTextEvent(state.ticks_, META_MARKER_TEXT, mark.c_str(), mark.size());

            //Count unknown events
            auto itfound = m_unhandledEvList.find(ev.evcode);

            if (itfound != m_unhandledEvList.end())
                itfound->second += 1;
            else
                m_unhandledEvList.insert(std::make_pair(ev.evcode, 1));
            clog << "\tEvent ID: 0x" << hex << uppercase << static_cast<unsigned short>(ev.evcode) << nouppercase << dec << ", is unsupported ! Ignoring..\n";
        }

        /***********************************************************************************
            HandleBankAndPresetOverrides
                Handle placing bank+preset change messages around a few specific notes.
        ***********************************************************************************/
        void HandleBankAndPresetOverrides(uint16_t              trkno,
            uint8_t               trkchan,
            TrkState& state,
            jdksmidi::MIDITrack& outtrack,
            midinote_t& mnoteid,
            const SMDLPresetConversionInfo::NoteRemapData& remapdata)
        {
            using namespace jdksmidi;

            //Remap the note
            //auto remapdata = m_convtable->RemapNote( state.origdseprgm_, (mnoteid & 0x7F) ); //Use the original program value for this
            //    
            //*** Remap the note ***
            mnoteid = remapdata.destnote;


            //First, check if we're playing a note past when the channel's preset + bank was changed by a previously processed track 
            if (m_midimode == eMIDIMode::GM &&
                !m_chanreqpresresets[trkchan].empty() &&
                m_chanreqpresresets[trkchan].front() <= state.ticks_)
            {
                bankid_t      bnktoset = 0;
                dsepresetid_t prgtoset = 0;

                //Figure out what to reset the state to
                if (state.presetoverriden)
                {
                    bnktoset = state.ovrbank_;
                    prgtoset = state.ovrprgm_;
                }
                else
                {
                    bnktoset = state.curbank_;
                    prgtoset = state.curprgm_;
                }

                //Make the messages
                AddBankChangeMessage(outtrack, trkchan, state.ticks_, bnktoset);

                MIDITimedBigMessage msgpreset;
                msgpreset.SetTime(state.ticks_);
                msgpreset.SetProgramChange(static_cast<uint8_t>(trkchan), static_cast<uint8_t>(prgtoset));
                outtrack.PutEvent(msgpreset);

                //Remove from the list!
                m_chanreqpresresets[trkchan].pop_front();
            }


            //Check if we should do an override, or restore the bank + preset
            if (state.presetoverriden &&
                remapdata.destpreset != state.ovrprgm_ &&
                remapdata.destbank != state.ovrbank_) //Restore if the preset+bank was overriden, but the current preset doesn't define a bank+preset override!
            {
                //*** Restore override ***
                state.presetoverriden = false;
                state.ovrprgm_ = InvalidPresetID;
                state.ovrbank_ = InvalidBankID;

                //Put some bank+preset change messages
                AddBankChangeMessage(outtrack, trkchan, state.ticks_, state.curbank_);

                //Restore original program
                MIDITimedBigMessage msgpreset;
                msgpreset.SetTime(state.ticks_);
                msgpreset.SetProgramChange(static_cast<uint8_t>(trkchan), static_cast<uint8_t>(state.curprgm_));
                outtrack.PutEvent(msgpreset);
            }
            else if (remapdata.destpreset != InvalidPresetID ||
                remapdata.destbank != InvalidBankID)  //If the preset is not being overriden, but the current preset defines valid overrides of the bank+preset!
            {
                //*** Override Preset and Bank ***
                if (remapdata.destbank != InvalidBankID)
                {
                    //Check if its necessary to put new events
                    if (!state.presetoverriden || state.ovrbank_ != remapdata.destbank)
                    {
                        state.ovrbank_ = remapdata.destbank;
                        AddBankChangeMessage(outtrack, trkchan, state.ticks_, state.ovrbank_);
                    }
                    //Do nothing otherwise
                }
                else if (state.presetoverriden)
                {
                    //Restore bank only
                    AddBankChangeMessage(outtrack, trkchan, state.ticks_, state.curbank_);
                }

                if (remapdata.destpreset != InvalidPresetID)
                {
                    //Check if its necessary to put new events
                    if (!state.presetoverriden || state.ovrprgm_ != remapdata.destpreset)
                    {
                        state.ovrprgm_ = remapdata.destpreset;
                        MIDITimedBigMessage msgpreset;

                        msgpreset.SetTime(state.ticks_);

                        msgpreset.SetProgramChange(static_cast<uint8_t>(trkchan), static_cast<uint8_t>(state.ovrprgm_));
                        outtrack.PutEvent(msgpreset);
                    }
                    //Do nothing otherwise
                }
                else if (state.presetoverriden)
                {
                    //Restore preset only
                    MIDITimedBigMessage msgpreset;
                    msgpreset.SetTime(state.ticks_);

                    //Restore original program
                    msgpreset.SetProgramChange(static_cast<uint8_t>(trkchan), static_cast<uint8_t>(state.curprgm_));
                    outtrack.PutEvent(msgpreset);
                }

                //Mark the track state as overriden!
                state.presetoverriden = true;
            }
        }

        /***********************************************************************************
            HandleChannelOverride
                Overrides the channel properly as needed, depending on the note remap data,
                or whether the preset has channel override data.
        ***********************************************************************************/
        inline uint8_t HandleChannelOverride(uint8_t                                         trkchan,
            TrkState& state,
            const SMDLPresetConversionInfo::NoteRemapData& remapdata)
        {
            //Only in GM
            if (m_midimode == eMIDIMode::GM)
            {
                //Handle per note remap
                if (remapdata.idealchan != std::numeric_limits<uint8_t>::max())
                {
                    //Handle per note remap
                    if (trkchan != MIDIDrumChannel) //We don't care about the drum channel, since it ignores program changes!
                        m_chanreqpresresets[remapdata.idealchan].push_back(state.ticks_); //Mark the channel for being re-inited
                    //state.chantoreinit = remapdata.idealchan; 
                    return remapdata.idealchan;
                }
                else if (state.chanoverriden)
                {
                    //Handle per preset remap
                    return state.ovrchan_;
                }
            }
            return trkchan;
        }

        /***********************************************************************************
            HandlePlayNote
                Handle converting a Playnote event into a MIDI key on and key off message !
        ************************************************************************************/
        void HandlePlayNote(uint16_t              trkno,
            uint8_t               trkchan,
            TrkState& state,
            const DSE::TrkEvent& ev,
            jdksmidi::MIDITrack& outtrack)
        {
            using namespace jdksmidi;
            MIDITimedBigMessage mess;

            //Check polyphony
            //if( state.curmaxpoly_ != -1 && state.curmaxpoly_ != 0 && state.noteson_.size() > static_cast<uint8_t>(state.curmaxpoly_) )
            //{
            //    //Kill another note and take its place
            //    uint8_t  notetokill   = state.noteson_.back().noteid;
            //    uint32_t noteoffnum   = state.noteson_.back().noteoffnum;
            //    uint32_t noteoffticks = state.noteson_.back().noteoffticks;
            //    state.noteson_.pop_back();

            //    //Check for the note off
            //    int evnum = 0;
            //    if( outtrack.FindEventNumber( state.noteson_.back().noteoffticks, &evnum ) )
            //    {
            //        MIDITimedBigMessage * ptrmess = outtrack.GetEvent( evnum );
            //        if( ptrmess != nullptr )
            //        {
            //            ptrmess->SetTime(state.ticks_); //Make it happen sooner
            //        }
            //        else
            //            assert(false);
            //    }
            //    else
            //        assert(false); 
            //}

            //Turn off sustain if neccessary
            //if( state.sustainon )
            //{
            //    MIDITimedBigMessage susoff;
            //    susoff.SetTime(state.ticks_);
            //    susoff.SetControlChange( trkchan, 66, 0 ); //sustainato
            //    outtrack.PutEvent( susoff );
            //    state.sustainon = false;
            //}

            //Interpret the first parameter byte of the play note event
            midinote_t mnoteid = 0;
            uint8_t    param2len = 0; //length in bytes of param2
            int8_t     octmod = 0;
            uint8_t    parsedkey = 0;
            //ParsePlayNoteParam1( ev.params.front(), state.octave_, param2len, mnoteid );

            ParsePlayNoteParam1(ev.params.front(), octmod, param2len, parsedkey);

            //Special case for when the play note even is 0xF
            if (parsedkey > static_cast<uint8_t>(eNote::nbNotes))
            {
                clog << "<!>- Event on track#" << trkno << ", has key ID 0x" << hex << static_cast<short>(parsedkey) << dec << "! Unsupported!\n";
                return;
            }
            else
            {
                state.octave_ = static_cast<int8_t>(state.octave_) + octmod;                          //Apply octave modification
                mnoteid = (state.octave_ * static_cast<uint8_t>(eNote::nbNotes)) + parsedkey; //Calculate MIDI key!
            }


            //Parse the note hold duration bytes
            uint32_t holdtime = 0;
            for (size_t cntby = 0; cntby < param2len; ++cntby)
                holdtime = (holdtime << 8) | ev.params[cntby + 1];

            if (param2len != 0)
                state.lasthold_ = holdtime;

            mess.SetTime(state.ticks_);

            //Check if we should change the note to another.
            if (m_hasconvtbl)
            {
                //Remap the note
                auto remapdata = m_convtable->RemapNote(state.origdseprgm_, (mnoteid & 0x7F)); //Use the original program value for this

                //Handle channel overrides for the preset, and for each notes
                trkchan = HandleChannelOverride(trkchan, state, remapdata);


                //Handle preset overrides for each notes
                HandleBankAndPresetOverrides(trkno, trkchan, state, outtrack, mnoteid, remapdata);

                //Apply transposition
                if (state.transpose != 0)
                {
                    int transposed = mnoteid + (state.transpose * NbMidiKeysInOctave);

                    if (transposed >= 0 && transposed < 127)
                        mnoteid = transposed;
                    else
                        clog << "<!>- Invalid transposition value was ignored! The transposed note " << transposed << " was out of the MIDI range!\n";
                }
            }

            //If we got an invalid bank, we just silence every notes, while leaving theme there!
            if (state.hasinvalidbank && !ShouldLeaveNoteWithInvalidPreset())
                mess.SetNoteOn(trkchan, mnoteid, 0); //leave the note, but play no sound!
            else
                mess.SetNoteOn(trkchan, mnoteid, static_cast<uint8_t>(ev.evcode & 0x7F));

            outtrack.PutEvent(mess);

            if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
                clog << "=> " << MidiNoteIdToText(mnoteid) << ", ";

            //Make the noteoff message
            MIDITimedBigMessage noteoff;
            uint32_t            noteofftime = state.ticks_ + state.lasthold_;


            //Check if we should cut the duration the key is held down.
            if (m_hasconvtbl)
            {
                auto itfound = m_convtable->FindConversionInfo(state.origdseprgm_);

                if (itfound != m_convtable->end() && itfound->second.maxkeydowndur != 0)
                    noteofftime = state.ticks_ + utils::Clamp(state.lasthold_, 0ui32, itfound->second.maxkeydowndur);
            }

            if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
                clog << "hold " << state.lasthold_ << " tick(s)";


            noteoff.SetTime(noteofftime);
            noteoff.SetNoteOff(trkchan, (mnoteid & 0x7F), static_cast<uint8_t>(ev.evcode & 0x7F)); //Set proper channel from original track eventually !!!!
            outtrack.PutEvent(noteoff);

            //if( state.curmaxpoly_ != -1 && state.curmaxpoly_ != 0 )
            //{
            //    //Add the note to the noteon list !
            //    NoteOnData mynoteon;
            //    mynoteon.noteid = (mnoteid & 0x7F);
            //    mynoteon.noteonnum = outtrack.GetNumEvents()-2;
            //    mynoteon.noteonticks = state.ticks_;
            //    mynoteon.noteoffnum = outtrack.GetNumEvents()-1;
            //    mynoteon.noteoffticks = state.ticks_ + state.lasthold_;
            //    state.noteson_.push_front( mynoteon );
            //}
        }


        /****************************************************************************
            PrepareMidiFile
                Place common messages into the MIDI file.
        ****************************************************************************/
        void PrepareMidiFile()
        {
            using namespace jdksmidi;
            //Setup Common Data
            //m_midiout = MIDIMultiTrack( m_trkstates.size() );
            //m_midiout.SetClksPerBeat( m_seq.metadata().tpqn );

            //Put a XG or GS sysex message if specified
            if (m_midimode == eMIDIMode::GS)
                WriteGSSysex();
            else if (m_midimode == eMIDIMode::XG)
                WriteXGSysex();

            //Init track 0 with time signature
            MIDITimedBigMessage timesig;
            timesig.SetTime(0);
            timesig.SetTimeSig();
            m_midiout.GetTrack(0)->PutEvent(timesig);
            m_midiout.GetTrack(0)->PutTextEvent(0, META_TRACK_NAME, m_seq.metadata().fname.c_str(), m_seq.metadata().fname.size());
            m_midiout.GetTrack(0)->PutTextEvent(0, META_GENERIC_TEXT, UtilityID.c_str(), UtilityID.size());
        }

        /****************************************************************************
        ****************************************************************************/
        void WriteGSSysex()
        {
            using namespace jdksmidi;
            {
                MIDITimedBigMessage gsreset;

                gsreset.SetTime(0);
                gsreset.SetSysEx(jdksmidi::SYSEX_START_N);

                MIDISystemExclusive mygssysex;
                mygssysex.PutEXC();
                mygssysex.PutByte(0x41); //Roland's ID
                mygssysex.PutByte(0x10); //Device ID, 0x10 is default 
                mygssysex.PutByte(0x42); //Model ID, 0x42 is universal for Roland
                mygssysex.PutByte(0x12); //0x12 means we're sending data 

                mygssysex.PutByte(0x40); //highest byte of address
                mygssysex.PutByte(0x00); //mid byte of address
                mygssysex.PutByte(0x7F); //lowest byte of address

                mygssysex.PutByte(0x00); //data

                mygssysex.PutByte(0x41); //checksum
                mygssysex.PutEOX();

                gsreset.CopySysEx(&mygssysex);
                m_midiout.GetTrack(0)->PutEvent(gsreset);
            }
            {
                //Now send the message to turn off the drum channel!
                MIDITimedBigMessage gsoffdrums;
                gsoffdrums.SetSysEx(jdksmidi::SYSEX_START_N);

                MIDISystemExclusive drumsysex;
                drumsysex.PutByte(0x41); //Roland's ID
                drumsysex.PutByte(0x10); //Device ID, 0x10 is default 
                drumsysex.PutByte(0x42); //Model ID, 0x42 is universal for Roland
                drumsysex.PutByte(0x12); //0x12 means we're sending data 

                drumsysex.PutByte(0x40); //highest byte of address
                drumsysex.PutByte(0x10); //mid byte of address
                drumsysex.PutByte(0x15); //lowest byte of address

                drumsysex.PutByte(0x00); //data

                drumsysex.PutByte(0x1B); //checksum
                drumsysex.PutEOX();
                gsoffdrums.CopySysEx(&drumsysex);
                m_midiout.GetTrack(0)->PutEvent(gsoffdrums);
            }
        }

        /****************************************************************************
        ****************************************************************************/
        void WriteXGSysex()
        {
            using namespace jdksmidi;
            //Ugh.. I have no clue if that's how I should do this.. 
            // JDKSmidi has 0 documentation and some of the most 
            // incoherent and unintuitive layout I've seen.. 
            // Though I've actually seen worse..
            MIDITimedBigMessage xgreset;
            xgreset.SetTime(0);
            //xgreset.SetSysEx(jdksmidi::SYSEX_START_N);

            std::array<uint8_t, 9> XG_SysEx{ {0x43,0x10,0x4C,0x00,0x00,0x7E,0x00} };
            MIDISystemExclusive mysysex(XG_SysEx.data(), XG_SysEx.size(), XG_SysEx.size(), false);
            xgreset.SetDataLength(9);
            //mysysex.PutEXC();
            //mysysex.PutByte(0x43); //Yamaha's ID
            //mysysex.PutByte(0x10); //Device ID, 0x10 is default 
            //mysysex.PutByte(0x4c);
            //mysysex.PutByte(0x00);
            //mysysex.PutByte(0x00);
            //mysysex.PutByte(0x7E);
            //mysysex.PutByte(0x00);
            //mysysex.PutEOX();

            xgreset.CopySysEx(&mysysex);
            xgreset.SetSysEx(jdksmidi::SYSEX_START_N);

            //Not sure if this all that I should do.. Documentation is sparse for XG

            m_midiout.GetTrack(0)->PutEvent(xgreset);
        }

        /****************************************************************************
            ExportAsSingleTrack
                Method handling export specifically for single track MIDI format 0
        ****************************************************************************/
        void ExportAsSingleTrack()
        {
            using namespace jdksmidi;

            //Setup our track states
            m_trkstates.resize(m_seq.getNbTracks());
            m_beflooptrkstates.resize(m_seq.getNbTracks());
            m_songlsttick = 0;

            //Setup the time signature and etc..
            PrepareMidiFile();

            //Re-Organize channels if in GM mode, and build priority queue
            if (m_midimode == eMIDIMode::GM)
                RearrangeChannels();
            else
            {
                m_trackpriorityq.resize(m_seq.getNbTracks());
                std::iota(m_trackpriorityq.begin(), m_trackpriorityq.end(), 0); //Fill up the queue sequentially otherwise 0..15
            }

            //Play all tracks at least once
            for (unsigned int trkno = 0; trkno < m_trackpriorityq.size(); ++trkno)
            {
                unsigned int curtrk = m_trackpriorityq[trkno];
                ExportATrack(curtrk, 0);

                //Keep track of the very last tick of the song as a whole.
                if (m_songlsttick < m_trkstates[curtrk].ticks_)
                    m_songlsttick = m_trkstates[curtrk].ticks_;
            }

            //Just mark the end of the loop when we're not looping ourselves via code. This prevents there being a delay when looping in external players!
            if (m_bTrackLoopable && m_nbloops == 0)
                m_midiout.GetTrack(0)->PutTextEvent(m_songlsttick, META_MARKER_TEXT, TXT_LoopEnd.c_str(), TXT_LoopEnd.size());

            WriteUnhandledEventsReport();

            //Loop the track again if loopable and requested !
            if (m_bTrackLoopable)
            {
                //Then, if we're set to loop, then loop
                for (unsigned int nbloops = 0; nbloops < m_nbloops; ++nbloops)
                {
                    for (unsigned int trkno = 0; trkno < m_trackpriorityq.size(); ++trkno)
                    {
                        unsigned int curtrk = m_trackpriorityq[trkno];

                        //Restore track state
                        uint32_t backticks = m_trkstates[curtrk].ticks_; //Save ticks
                        m_trkstates[curtrk] = m_beflooptrkstates[curtrk]; //Overwrite state
                        m_trkstates[curtrk].ticks_ = backticks;                 //Restore ticks

                        ExportATrack(curtrk, 0, m_trkstates[curtrk].looppoint_);
                    }
                }
            }


        }

        /*
            RearrangeChannels
                Try to free channel 10 if possible, and if not, set a track that has a 0x7F program change on chan 10.

                Only for GM conversion. Should probably get phased out eventually, because its pretty bad at its job.
                Not that swapping stuff from channel to channel is easy..
        */
        void RearrangeChannels()
        {
            //Init 
            m_chanreqpresresets.resize(NbMidiChannels);
            m_trkchanmap.resize(m_seq.getNbTracks(), 0); //Init the channel map!

            //Start by filling up the priority queue
            //#NOTE: This is really atrocious, 
            vector<size_t> presetswithchanremaps;       //The presets that have key remaps that play on another channel!

            //Make a list of the presets that remap notes to different channels
            for (const auto& remapentry : (*m_convtable))
            {
                for (const auto& notermap : remapentry.second.remapnotes)
                {
                    //If we have a channel remap for a specific key, and we haven't added this preset to the list yet, add it!
                    if ((notermap.second.idealchan != std::numeric_limits<uint8_t>::max()) &&
                        std::find(presetswithchanremaps.begin(), presetswithchanremaps.end(), remapentry.first) != presetswithchanremaps.end())
                    {
                        presetswithchanremaps.push_back(remapentry.first);
                    }
                }
            }

            //Find which tracks use those presets, and put them first in the priority queue!
            for (size_t cnttrk = 1; cnttrk < m_seq.getNbTracks(); ++cnttrk) //Skip track 1 again
            {
                bool prioritize = false;
                for (size_t cntev = 0; cntev < m_seq[cnttrk].size(); ++cntev)
                {
                    if (m_seq[cnttrk][cntev].evcode == static_cast<uint8_t>(eTrkEventCodes::SetProgram))
                    {
                        //When we get a SetProgram event, check if it matches one of the presets we're looking for.
                        for (const auto& apreset : presetswithchanremaps)
                        {
                            if (m_seq[cnttrk][cntev].params.front() == apreset)
                                prioritize = true;
                        }
                    }
                }

                //If we need to prioritize, push it in the front
                if (prioritize)
                    m_trackpriorityq.push_front(cnttrk);
                else
                    m_trackpriorityq.push_back(cnttrk);
            }

            //Push track 0
            m_trackpriorityq.push_back(0);

            //*** Next re-arrange the tracks ! ***

            array<bool, NbMidiChannels> usedchan{false};         //Keep tracks of channels in use

            //Check if something is on channel 10, and populate our channel map
            deque<size_t> chan10trks; //A list of tracks using chan 10

            //Always skip the first DSE track, as it can't have instruments on it
            for (size_t cnttrk = 1; cnttrk < m_seq.getNbTracks(); ++cnttrk)
            {
                if (m_seq[cnttrk].GetMidiChannel() < NbMidiChannels)
                    usedchan[m_seq[cnttrk].GetMidiChannel()] = true;
                else
                    clog << "<!>- Warning: Encountered a track with an invalid MIDI channel in the current sequence!\n";

                if (m_seq[cnttrk].GetMidiChannel() == 9)
                    chan10trks.push_back(cnttrk);

                m_trkchanmap[cnttrk] = m_seq[cnttrk].GetMidiChannel();
            }

            //If channel 10 not in use, nothing else to do here!
            if (chan10trks.empty())
                return;

            // -------------------------------------
            // -- Mitigate the drum channel issue --
            // -------------------------------------

            // --- Step 1! ---
            size_t nbchaninuse = std::count_if(usedchan.begin(), usedchan.end(), [](bool entry) { return entry; });

            //First, try to reassign to an empty channel!
            if (nbchaninuse != NbMidiChannels)
            {
                const size_t  NbToReloc = chan10trks.size();
                deque<size_t> modch10trks;
                for (size_t cnttrks = 0; cnttrks < NbToReloc; ++cnttrks)
                {
                    size_t trktorelocate = chan10trks[cnttrks];

                    for (size_t cntchan = 0; cntchan < usedchan.size(); ++cntchan)
                    {
                        if (!usedchan[cntchan] && cntchan != 9)
                        {
                            //Re-assign the unused channel
                            usedchan[m_trkchanmap[trktorelocate]] = false;   //Vaccate the old channel
                            m_trkchanmap[trktorelocate] = static_cast<uint8_t>(cntchan);
                            usedchan[cntchan] = true;
                            //Remove a track from the list to relocate!
                        }
                        else
                            modch10trks.push_back(trktorelocate);
                    }
                }
                chan10trks = move(modch10trks);
                //If we re-assigned all tracks, nothing more to do here !
                if (chan10trks.empty())
                    return;
                //If we still haven't reassigned all tracks, we go to step 2
            }


            // --- Step 2! ---
            //Then, we'll try to swap our place with a track that makes use of preset 0x7F, which is used for drums usually.
            deque<std::pair<size_t, size_t>> drumusingchans; //First is track index, second is channel it uses

            for (size_t cnttrk = 1; cnttrk < m_seq.getNbTracks(); ++cnttrk) //Skip track 1 again
            {
                for (size_t cntev = 0; cntev < m_seq[cnttrk].size(); ++cntev)
                {
                    if (m_seq[cnttrk][cntev].evcode == static_cast<uint8_t>(eTrkEventCodes::SetProgram) &&
                        m_seq[cnttrk][cntev].params.front() == 0x7F)
                    {
                        drumusingchans.push_back(make_pair(cnttrk, m_seq[cnttrk].GetMidiChannel()));

                        //If one of the tracks we have is using channel 10 AND is using preset 0x7F, remove it from the list
                        // as it doesn't need to be relocated!
                        auto itfound = std::find(chan10trks.begin(), chan10trks.end(), cnttrk);
                        if (itfound != chan10trks.end())
                            chan10trks.erase(itfound);
                    }
                }
            }

            //Now, do the actual swapping!
            if (!drumusingchans.empty())
            {
                for (; !chan10trks.empty() && !drumusingchans.empty(); )
                {
                    auto & drumuserchan = drumusingchans.back();

                    uint8_t prevchan = m_trkchanmap[chan10trks.back()];
                    m_trkchanmap[chan10trks.back()] = m_trkchanmap[drumuserchan.first];
                    m_trkchanmap[drumuserchan.first] = prevchan;

                    drumusingchans.pop_back();
                    chan10trks.pop_back();
                }

                //If we handled all the tracks on channel 10, we're done!
                if (chan10trks.empty())
                    return;
            }


            // --- If we get here, we're screwed! ---
            clog << "<!>- Warning: Unable to reassign the channels for track(s) ";

            for (const auto& entry : chan10trks)
                clog << entry << " ";

            clog << " !\n";
        }


        /*
            ExportATrack
                Exports a single track, intrk, to the midi output, in the track slot specified by outtrk.

                - intrk  : The DSE track we're processing.
                - outtrk : The MIDI track in the output to place the events processed.
                - evno   : The event to begin parsing the track at.
        */
        void ExportATrack(unsigned int intrk, unsigned int outtrk, size_t evno = 0)
        {
            if (utils::LibWide().isLogOn())
            {
                clog << "---- Exporting Track#" << intrk << " ----\n";
            }

            for (; evno < m_seq.track(intrk).size(); ++evno)
            {
                if (m_seq[intrk][evno].evcode == static_cast<uint8_t>(DSE::eTrkEventCodes::EndOfTrack) &&
                    !ShouldIgnorePrematureEoT())
                    break; //Break on 0x98 as requested 

                //Obtain the channel the content of this track is played on
                uint8_t prefchan = m_seq[intrk].GetMidiChannel();

                if (m_midimode == eMIDIMode::GM)   //In GM mode, use our remapped channel table
                    prefchan = m_trkchanmap[intrk];

                HandleEvent(intrk,
                    prefchan,
                    m_trkstates[intrk],
                    m_seq[intrk][evno],
                    *(m_midiout.GetTrack(outtrk)));
            }

            if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
                clog << "---- End of Track ----\n\n";
        }

        /*
            ShouldMarkUnsupported
                Return whether unsupported events should be marked in the MIDI!
        */
        bool ShouldMarkUnsupported()const
        {
            return true; //#TODO: Make this configurable
        }

        /*
            ShouldIgnorePrematureEoT
                Returns whether we should keep converting events past any premature end of the track marker, if there are any.
        */
        bool ShouldIgnorePrematureEoT()const
        {
            return false; //#TODO: Make this configurable
        }

        /*
            ShouldLeaveNoteWithInvalidPreset
                Some tracks sometimes refers to a program in the SWD that is not loaded. It results in no sound being
                produced when the notes associated with that program number are played. However that's not the case with
                a MIDI.

                Setting this property to false will leave the silent note events in the resulting MIDI, but will set their
                velocity to 0, essentially making them silent.

                Setting it to true will leave the note events as-is.
        */
        bool ShouldLeaveNoteWithInvalidPreset()const
        {
            return false;
        }


        void WriteUnhandledEventsReport()
        {
            if (m_unhandledEvList.empty())
                return;

            clog << "<!>- Ingored the following unsupported events: \n";

            for (const auto& ev : m_unhandledEvList)
            {
                clog << "\tEventID: 0x" << hex << uppercase << static_cast<unsigned short>(ev.first) << dec << nouppercase
                    << ", ignored " << ev.second << " times.\n";
            }

        }

    private:
        const std::string& m_fnameout;
        const MusicSequence& m_seq;
        const SMDLPresetConversionInfo* m_convtable;
        bool                               m_hasconvtbl;    //Whether we should use the conv table at all
        uint32_t                           m_nbloops;
        /*eMIDIFormat                        m_midifmt;*/
        eMIDIMode                          m_midimode;

        //State variables
        //#TODO: This is horrible! I need to build something better!
        std::vector<TrkState>              m_trkstates;
        std::vector<TrkState>              m_beflooptrkstates; //Saved states of each tracks just before the loop point event! So we can restore each tracks to their intended initial states.
        std::vector<uint8_t>               m_trkchanmap;       //Contains the channel to use for each tracks. This overrides what's in the sequence!
        std::deque<size_t>                 m_trackpriorityq;   //tracks are ordered in this vector to be parsed in that order. Mainly to ensure tracks that have key remaps in which the channel is forced to something are handled first!
        std::vector<deque<uint32_t>>       m_chanreqpresresets; //The times for each tracks when we need to re-establish the current program + bank, after having other tracks play events on our MIDI channel!.  

        std::map<uint8_t, int>             m_unhandledEvList;   //A table to store all the unsupported events encountered and the number of times it was encountered!

        uint32_t                           m_songlsttick;      //The very last tick of the song

        bool                               m_bTrackLoopable;
        //Those two only apply to single track mode !
        bool                               m_bLoopBegSet;

        jdksmidi::MIDIMultiTrack           m_midiout;
    };

    //======================================================================================
    //  Functions
    //======================================================================================<
    void WriteMusicSequenceXml(const std::string& outmidi, const MusicSequence& seq)
    {
        //Save the extra xml with the sequence meta-data
        pugi::xml_document doc;
        DSE::WriteDSEXmlNode(doc, &seq.metadata());
        DSE::WriteSequenceInfo(doc, seq.seqinfo());
        Poco::Path outxml = Poco::Path(outmidi).setExtension("xml");
        outxml.setBaseName(outxml.getBaseName() + "_seq");
        doc.save_file(Poco::Path::transcode(outxml.toString()).c_str());
    }

    void SequenceToMidi(const std::string& outmidi,
        const MusicSequence& seq,
        const SMDLPresetConversionInfo& remapdata,
        int                              nbloop,
        eMIDIMode                        midmode)
    {
        if (utils::LibWide().isLogOn())
        {
            clog << "================================================================================\n"
                << "Converting SMDL to MIDI " << outmidi << "\n"
                << "================================================================================\n";
        }
        DSESequenceToMidi(outmidi, seq, remapdata, midmode, nbloop)();
        WriteMusicSequenceXml(outmidi, seq);
    }

    void SequenceToMidi(const std::string& outmidi,
        const MusicSequence& seq,
        int                              nbloop,
        eMIDIMode                        midmode)
    {
        if (utils::LibWide().isLogOn())
        {
            clog << "================================================================================\n"
                << "Converting SMDL to MIDI " << outmidi << "\n"
                << "================================================================================\n";
        }
        DSESequenceToMidi(outmidi, seq, midmode, nbloop)();
        WriteMusicSequenceXml(outmidi, seq);
    }
}