#include <dse/dse_conversion.hpp>
#include <dse/dse_to_xml.hpp>
#include <dse/containers/dse_music_sequence.hpp>
#include <dse/dse_sequence.hpp>
#include <dse/dse_common.hpp>

#include <pugixml.hpp>

#include <utils/pugixml_utils.hpp>
#include <utils/parse_utils.hpp>
#include <poco/Path.h>

#include <algorithm>

using namespace std;
using namespace pugi;
using namespace Poco;
using namespace pugixmlutils;

namespace DSE
{
    namespace MusicSeqXML
    {
        const std::string NODE_Sequence = "Sequence";
        const std::string NODE_Track    = "Track";
        
        //Track members
        const std::string NODE_Note  = "Note";
        const std::string NODE_Delay = "Delay";
        const std::string NODE_Event = "Event";
        const std::string NODE_Wait  = "Wait";

        const std::string ATTR_Vol = "vol";
        const std::string ATTR_Pan = "pan";
        const std::string ATTR_On  = "on";
        const std::string ATTR_Channel = "chan";
        const std::string ATTR_Type = "type";

        const std::string ATTR_ID        = "id";
        const std::string ATTR_Duration  = "duration";
        const std::string ATTR_Note      = "note";
        const std::string ATTR_Velocity  = "vel";
        const std::string ATTR_Octave    = "octave";
        const std::string ATTR_ModOctave = "add_octave";
        const std::string ATTR_BPM       = "bpm";
        const std::string ATTR_BankLo    = "bank_low";
        const std::string ATTR_BankHi    = "bank_high";
        const std::string ATTR_Program   = "program";
        const std::string ATTR_Semitones = "semitones";
        const std::string ATTR_Min       = "min";
        const std::string ATTR_Max       = "max";
        const std::string ATTR_Rate      = "rate";
        const std::string ATTR_Depth     = "depth";
        const std::string ATTR_WShape    = "wave_shape";
        const std::string ATTR_Repeats   = "nb_repeats";
        const std::string ATTR_Level     = "level";

        const std::string ATTR_Unk = "unkown";
        const std::string ATTR_Unk2 = "unknown2";
        const std::string ATTR_Unk3 = "unknown3";
        const std::string ATTR_Unk4 = "unknown4";
        const std::string ATTR_Unk5 = "unknown5";
        const std::string ATTR_Decay = "decay";
        const std::string ATTR_Sustain = "sustain";
        const std::string ATTR_Bend = "pitch_bend"; //500 == 1 semitone
        const std::string ATTR_Delay = "delay";
        const std::string ATTR_Fade = "fade";
        const std::string ATTR_Value = "value";
        const std::string ATTR_Target = "target";
        const std::string ATTR_Quarter = "in_quarter_notes";
    };

    //
    // XML Parser
    //
    class MusicSequenceXMLParser
    {
        MusicSequence m_seq;

    public:
        DSE::MusicSequence operator()(const std::string& filepath)
        {
            using namespace MusicSeqXML;
            m_seq = MusicSequence();
            xml_document     doc;
            xml_parse_result parseres;
            DSE_MetaDataSMDL meta;
            try
            {
                if (!(parseres = doc.load_file(filepath.c_str())))
                {
                    stringstream sstr;
                    sstr << "Can't load XML document, Pugixml returned an error : \"" << parseres.description() << "\"";
                    throw std::runtime_error(sstr.str());
                }
                ParseDSEXmlNode(doc, &meta);
                m_seq.setMetadata(meta);

                xml_node seqnode = doc.child(NODE_Sequence.c_str());
                if(!seqnode)
                    throw std::runtime_error("Missing sequence node!");
                
                std::vector<MusicTrack> loadedtrks;
                loadedtrks.reserve(16);
                size_t trkcnt = 0;
                for (xml_node node : seqnode.children(NODE_Track.c_str()))
                {
                    loadedtrks.push_back(ParseTrack(node, trkcnt));
                    ++trkcnt;
                }
                m_seq.setTracks(std::move(loadedtrks));
            }
            catch (...)
            {
                std::throw_with_nested(std::runtime_error("Error parsing music sequence from xml file \""+ filepath + "\""));
            }
            return std::move(m_seq);
        }

    private:

        MusicTrack ParseTrack(xml_node trk, size_t trkcnt)
        {
            using namespace MusicSeqXML;
            MusicTrack mtrk;
            try
            {
                xml_attribute atrkchan = trk.attribute(ATTR_Channel.c_str());
                if (!atrkchan)
                    throw std::runtime_error("Track is missing its channel attribute!");
                
                mtrk.SetMidiChannel((uint8_t)atrkchan.as_uint());

                for (xml_node evnode : trk.children())
                {
                    const std::string evname = evnode.name();

                    if (evname == NODE_Delay)
                        mtrk.push_back(ParseDelay(evnode));
                    else if (evname == NODE_Note)
                        mtrk.push_back(ParseNote(evnode));
                    else if (evname == NODE_Wait)
                        mtrk.push_back(ParseWait(evnode));
                    else if (evname == NODE_Event)
                        mtrk.push_back(ParseEvent(evnode));
                    else
                        throw std::runtime_error("Unknown event tag \"" + evname + "\"");
                }
            }
            catch (...)
            {
                std::throw_with_nested(std::runtime_error("Error while parsing track #" + std::to_string(trkcnt)));
            }
            return mtrk;
        }

        TrkEvent ParseDelay(xml_node ev)
        {
            using namespace MusicSeqXML;
            TrkEvent      outev;
            xml_attribute adur = ev.attribute(ATTR_Duration.c_str());
            if (!adur)
                throw std::runtime_error("Delay event is missing its duration attribute!");
            uint8_t ticks = (uint8_t)adur.as_uint();
            auto    found = FindClosestTrkDelayID(ticks);
            if(!found)
                throw std::runtime_error("Delay event has an invalid duration attribute!");
            outev.evcode = TrkDelayToEvID.at(*found);
            return outev;
        }

        TrkEvent ParseNote(xml_node ev)
        {
            using namespace MusicSeqXML;
            TrkEvent outev;
            xml_attribute avelocity = ev.attribute(ATTR_Velocity.c_str());
            xml_attribute anote     = ev.attribute(ATTR_Note.c_str());
            xml_attribute aoctave   = ev.attribute(ATTR_ModOctave.c_str());
            xml_attribute aduration = ev.attribute(ATTR_Duration.c_str());
            if (!anote)
                throw std::runtime_error("Note event tag is missing its note attribute.");

            //Velocity is the event code
            outev.evcode = std::clamp( avelocity.as_uint(std::numeric_limits<int8_t>::max()), (unsigned int)0, (unsigned int)std::numeric_limits<int8_t>::max());

            //Assemble the note parameter
            uint8_t noteparam0 = 0;

            //Duration
            const uint32_t duration = aduration.as_uint();
            uint8_t  nbdurationbytes = 0;
            if (duration > 0)
            {
                if (duration <= 0xFFui8)
                    nbdurationbytes = 1;
                else if (duration <= 0xFFFFui16)
                    nbdurationbytes = 2;
                else
                    nbdurationbytes = 3;
                noteparam0 |= (nbdurationbytes & 0x3) << 6;
            }

            //Get the octave shift
            const uint8_t octaveshift = std::clamp((int8_t)aoctave.as_int(0), (int8_t)(- 2), (int8_t)1) + NoteEvOctaveShiftRange;
            noteparam0 |= (octaveshift & 3ui8) << 4;

            //Get the note id
            eNote noteid = StringToNote(anote.as_string());
            if (noteid >= eNote::nbNotes) //If our id is invalid
                noteparam0 |= (uint8_t)anote.as_uint() & 0x0F; //Confine to lower nybble. sometimes the note is a special number used in some sequences
            else
                noteparam0 |= (uint8_t)noteid & 0x0F;

            //Handle param 0
            outev.params.push_back(noteparam0);

            //Handle optional duration bytes (Big endian)
            if (nbdurationbytes == 3)
            {
                outev.params.push_back((duration >> 16) & 0xFFui8);
                outev.params.push_back((duration >> 8) & 0xFFui8);
                outev.params.push_back(duration & 0xFFui8);
            }
            else if (nbdurationbytes == 2)
            {
                outev.params.push_back((duration >> 8) & 0xFFui8);
                outev.params.push_back(duration & 0xFFui8);
            }
            else if(nbdurationbytes == 1)
                outev.params.push_back(duration & 0xFFui8);

            return outev;
        }

        TrkEvent ParseWait(xml_node ev)
        {
            using namespace MusicSeqXML;
            xml_attribute evty = ev.attribute(ATTR_Type.c_str());
            if (!evty)
                throw std::runtime_error("Wait event with missing event type!");
            const string         evname = evty.as_string();
            const eTrkEventCodes code   = StringToEventCode(evname);
            if (code == eTrkEventCodes::Invalid)
                throw runtime_error("Got bad event type \"" + evname + "\"!");

            TrkEvent outev{ (uint8_t)code };
            if (code == eTrkEventCodes::RepeatLastPause)
                return outev; //No parameters

            switch (code)
            {
            
            case eTrkEventCodes::AddToLastPause:
            case eTrkEventCodes::Pause8Bits:
            case eTrkEventCodes::PauseUntilRel:
            {
                outev.params.push_back(ClampUInt8(ev.attribute(ATTR_Duration.c_str()).as_uint()));
                break;
            }
            case eTrkEventCodes::Pause16Bits:
            {
                const uint16_t duration = ClampUInt16(ev.attribute(ATTR_Duration.c_str()).as_uint());
                outev.params.push_back((uint8_t)duration);
                outev.params.push_back((uint8_t)((duration >> 8) & 0xFF));
                break;
            }
            case eTrkEventCodes::Pause24Bits:
            {
                const uint32_t duration = ev.attribute(ATTR_Duration.c_str()).as_uint();
                outev.params.push_back((uint8_t)duration);
                outev.params.push_back((uint8_t)((duration >> 8) & 0xFF));
                outev.params.push_back((uint8_t)((duration >> 16) & 0xFF));
                break;
            }
            };
            return outev;
        }

        TrkEvent ParseEvent(xml_node ev)
        {
            using namespace MusicSeqXML;
            xml_attribute evty = ev.attribute(ATTR_Type.c_str());
            if (!evty)
                throw std::runtime_error("Event with missing event type!");
            const string         evname = evty.as_string();
            const eTrkEventCodes code  = StringToEventCode(evname);
            if (code == eTrkEventCodes::Invalid)
                throw runtime_error("Got bad event type \"" + evname + "\"!");

            TrkEvent outev{ (uint8_t)code };

            switch (code)
            {
            case eTrkEventCodes::SetTempo:
            case eTrkEventCodes::SetTempo2:
            {
                outev.params.push_back(ClampUInt8(ev.attribute(ATTR_BPM.c_str()).as_uint()));
                break;
            }

            case eTrkEventCodes::RepeatFrom:
            {
                outev.params.push_back(ClampUInt8(ev.attribute(ATTR_Repeats.c_str()).as_uint()));
                break;
            }
            case eTrkEventCodes::SetOctave:
            case eTrkEventCodes::AddOctave:
            {
                outev.params.push_back(ClampInt8(ev.attribute(ATTR_Octave.c_str()).as_int()));
                break;
            }

            case eTrkEventCodes::SetBank:
            {
                outev.params.push_back(ClampInt8From0(ev.attribute(ATTR_BankHi.c_str()).as_int()));
                outev.params.push_back(ClampInt8From0(ev.attribute(ATTR_BankLo.c_str()).as_uint()));
                break;
            }

            case eTrkEventCodes::SetBankHighByte:
            {
                outev.params.push_back(ClampInt8From0(ev.attribute(ATTR_BankHi.c_str()).as_int()));
                break;
            }
            case eTrkEventCodes::SetBankLowByte:
            {
                outev.params.push_back(ClampInt8From0(ev.attribute(ATTR_BankLo.c_str()).as_uint()));
                break;
            }

            case eTrkEventCodes::SetProgram:
            {
                outev.params.push_back(ClampInt8From0(ev.attribute(ATTR_Program.c_str()).as_uint()));
                break;
            }

            case eTrkEventCodes::SweepTrackVol:
            case eTrkEventCodes::FadeSongVolume:
            {
                const uint16_t rate = ClampUInt16(ev.attribute(ATTR_Rate.c_str()).as_uint());
                outev.params.push_back((uint8_t)rate);
                outev.params.push_back((uint8_t)((rate >> 8) & 0xFF));
                outev.params.push_back(ClampDSEVolume(ev.attribute(ATTR_Vol.c_str()).as_uint()));
                break;
            }

            case eTrkEventCodes::SetEnvAtkTime:
            case eTrkEventCodes::SetEnvHold:
            case eTrkEventCodes::SetEnvFade:
            case eTrkEventCodes::SetEnvRelease:
            {
                outev.params.push_back(ClampInt8From0(ev.attribute(ATTR_Level.c_str()).as_uint()));
                break;
            }

            case eTrkEventCodes::SetTrkPan:
            case eTrkEventCodes::AddTrkPan:
            case eTrkEventCodes::SetChanPan:
            {
                outev.params.push_back(ClampDSEPan(ev.attribute(ATTR_Pan.c_str()).as_uint()));
                break;
            }

            case eTrkEventCodes::SweepTrkPan:
            {
                const uint16_t rate = ClampUInt16(ev.attribute(ATTR_Rate.c_str()).as_uint());
                outev.params.push_back((uint8_t)rate);
                outev.params.push_back((uint8_t)((rate >> 8) & 0xFF));
                outev.params.push_back(ClampDSEPan(ev.attribute(ATTR_Pan.c_str()).as_uint()));
                break;
            }

            case eTrkEventCodes::SetExpress:
            case eTrkEventCodes::SetTrkVol:
            case eTrkEventCodes::AddTrkVol:
            case eTrkEventCodes::SetNoteVol:
            case eTrkEventCodes::SetChanVol:
            case eTrkEventCodes::SetEnvAtkLvl:
            {
                outev.params.push_back(ClampDSEVolume(ev.attribute(ATTR_Vol.c_str()).as_uint()));
                break;
            }

            case eTrkEventCodes::SetEnvDecSus:
            {
                outev.params.push_back(ClampInt8From0(ev.attribute(ATTR_Decay.c_str()).as_uint()));
                outev.params.push_back(ClampDSEVolume(ev.attribute(ATTR_Sustain.c_str()).as_uint()));
                break;
            }

            case eTrkEventCodes::SetFTune:
            case eTrkEventCodes::AddFTune:
            case eTrkEventCodes::SetCTune:
            case eTrkEventCodes::AddCTune:
            {
                outev.params.push_back(ClampInt8(ev.attribute(ATTR_Semitones.c_str()).as_int()));
                break;
            }

            case eTrkEventCodes::SweepTune:
            {
                const uint16_t rate = ClampUInt16(ev.attribute(ATTR_Rate.c_str()).as_uint());
                outev.params.push_back((uint8_t)rate);
                outev.params.push_back((uint8_t)((rate >> 8) & 0xFF));
                outev.params.push_back(ClampInt8(ev.attribute(ATTR_Semitones.c_str()).as_int()));
                break;
            }

            case eTrkEventCodes::SetRndNoteRng:
            {
                outev.params.push_back(ClampInt8From0(ev.attribute(ATTR_Min.c_str()).as_int()));
                outev.params.push_back(ClampInt8From0(ev.attribute(ATTR_Max.c_str()).as_int()));
                break;
            }

            case eTrkEventCodes::SetDetuneRng:
            {
                const uint16_t detune = ClampUInt16(ev.attribute(ATTR_Semitones.c_str()).as_uint());
                outev.params.push_back((uint8_t)detune);
                outev.params.push_back((uint8_t)((detune >> 8) & 0xFF));
                break;
            }

            case eTrkEventCodes::SetPitchBend:
            {
                const uint16_t bend = ClampInt16(ev.attribute(ATTR_Bend.c_str()).as_int());
                outev.params.push_back((uint8_t)((bend >> 8) & 0xFF)); //Big endian aparently
                outev.params.push_back((uint8_t)bend);
                break;
            }

            case eTrkEventCodes::SetPitchBendRng:
            {
                outev.params.push_back(ClampUInt8(ev.attribute(ATTR_Semitones.c_str()).as_uint()));
                break;
            }

            case eTrkEventCodes::RouteLFO1ToPitch:
            case eTrkEventCodes::RouteLFO2ToVol:
            case eTrkEventCodes::RouteLFO3ToPan:
            {
                outev.params.push_back((uint8_t)ev.attribute(ATTR_On.c_str()).as_bool());
                break;
            }

            case eTrkEventCodes::SetLFO:
            case eTrkEventCodes::SetLFO1:
            case eTrkEventCodes::SetLFO2:
            case eTrkEventCodes::SetLFO3:
            {
                const uint16_t rate = ClampUInt16(ev.attribute(ATTR_Rate.c_str()).as_uint());
                const uint16_t depth = ClampUInt16(ev.attribute(ATTR_Depth.c_str()).as_uint());
                outev.params.push_back((uint8_t)rate);
                outev.params.push_back((uint8_t)((rate >> 8) & 0xFF));
                outev.params.push_back((uint8_t)depth);
                outev.params.push_back((uint8_t)((depth >> 8) & 0xFF));
                outev.params.push_back(ClampUInt8(ev.attribute(ATTR_WShape.c_str()).as_uint()));
                break;
            }

            case eTrkEventCodes::SetLFODelFade:
            case eTrkEventCodes::SetLFO1DelayFade:
            case eTrkEventCodes::SetLFO2DelFade:
            case eTrkEventCodes::SetLFO3DelFade:
            {
                const uint16_t delay = ClampUInt16(ev.attribute(ATTR_Delay.c_str()).as_uint());
                const uint16_t fade = ClampUInt16(ev.attribute(ATTR_Fade.c_str()).as_uint());
                outev.params.push_back((uint8_t)delay);
                outev.params.push_back((uint8_t)((delay >> 8) & 0xFF));
                outev.params.push_back((uint8_t)fade);
                outev.params.push_back((uint8_t)((fade >> 8) & 0xFF));
                break;
            }

            case eTrkEventCodes::SetLFOParam:
            {
                outev.params.push_back(ClampUInt8(ev.attribute(ATTR_ID.c_str()).as_uint()));
                outev.params.push_back(ClampUInt8(ev.attribute(ATTR_Value.c_str()).as_uint()));
                break;
            }

            case eTrkEventCodes::SetLFORoute:
            {
                outev.params.push_back(ClampUInt8(ev.attribute(ATTR_ID.c_str()).as_uint()));
                outev.params.push_back((uint8_t)(ev.attribute(ATTR_On.c_str()).as_bool()));
                outev.params.push_back(ClampUInt8(ev.attribute(ATTR_Target.c_str()).as_uint()));
                break;
            }

            case eTrkEventCodes::EndOfTrack:
            case eTrkEventCodes::DisableEnvelope:
            case eTrkEventCodes::RepeatSegment:
            case eTrkEventCodes::AfterRepeat:
            case eTrkEventCodes::LoopPointSet:
            case eTrkEventCodes::SkipNextByte:
            case eTrkEventCodes::SkipNext2Bytes1:
            case eTrkEventCodes::SkipNext2Bytes2:
            case eTrkEventCodes::Unk_0xC0:
            {
                //No params
                break;
            }

            case eTrkEventCodes::Unk_0xF6:
            case eTrkEventCodes::Unk_0xBF:
            {
                outev.params.push_back(ClampUInt8(ev.attribute(ATTR_Unk.c_str()).as_uint()));
                break;
            }

            case eTrkEventCodes::Unk_0xD8:
            {
                outev.params.push_back(ClampUInt8(ev.attribute(ATTR_Unk.c_str()).as_uint()));
                outev.params.push_back(ClampUInt8(ev.attribute(ATTR_Unk2.c_str()).as_uint()));
                break;
            }

            default:
            {
                //Ignored
                break;
            }
            };
            return outev;
        }
    };

    //
    // XML Writer
    //
    class MusicSequenceXMLWriter
    {
        const MusicSequence& m_seq;

    public:
        MusicSequenceXMLWriter(const DSE::MusicSequence& mseq):m_seq(mseq) {}

        void operator()(const std::string& filepath)
        {
            using namespace MusicSeqXML;
            xml_document doc;
            WriteDSEXmlNode(doc, &m_seq.metadata());
            xml_node seqnode = AppendChildNode(doc, NODE_Sequence);
            WriteSequenceInfo(seqnode, m_seq.seqinfo());

            for (size_t cnttrk = 0; cnttrk < m_seq.getNbTracks(); ++cnttrk)
            {
                WriteTrack(seqnode, cnttrk);
            }

            if (!doc.save_file(filepath.c_str()))
                throw std::runtime_error("Can't write xml file \"" + filepath + "\"");
        }

    private:
        void WriteTrack(xml_node parent, size_t trkid)
        {
            using namespace MusicSeqXML;
            const MusicTrack&    trk = m_seq[trkid];
            WriteCommentNode(parent, "Track #"s + std::to_string(trkid));

            xml_node trknode = AppendChildNode(parent, NODE_Track);
            AppendAttribute(trknode, ATTR_Channel, trk.GetMidiChannel());

            for (const TrkEvent& ev : trk)
            {
                WriteMessage(trknode, ev);
            }
        }

        void WriteMessage(xml_node parent, const TrkEvent& ev)
        {
            using namespace MusicSeqXML;
            const eTrkEventCodes code = static_cast<eTrkEventCodes>(ev.evcode);

            if (code >= eTrkEventCodes::Delay_HN && code <= eTrkEventCodes::Delay_64N)
                WriteDelay(parent, ev);
            else if (code >= eTrkEventCodes::NoteOnBeg && code <= eTrkEventCodes::NoteOnEnd)
                WriteNote(parent, ev);
            else if (code >= eTrkEventCodes::RepeatLastPause && code <= eTrkEventCodes::PauseUntilRel)
                WriteWaitEvent(parent, ev);
            else
                WriteEvent(parent, ev);
        }

        void WriteDelay(xml_node parent, const TrkEvent& ev)
        {
            using namespace MusicSeqXML;
            const uint8_t code   = ev.evcode;
            xml_node      evnode = AppendChildNode(parent, NODE_Delay);
            AppendAttribute(evnode, ATTR_Duration, (unsigned int)TrkDelayCodeVals.at(code) );
        }

        void WriteNote(xml_node parent, const TrkEvent& ev)
        {
            using namespace MusicSeqXML;
            const uint8_t      code      = ev.evcode;
            const ev_play_note plev      = ParsePlayNote(ev);
            xml_node           evnode    = AppendChildNode(parent, NODE_Note);

            AppendAttribute(evnode, ATTR_Velocity, code);
            AppendAttribute(evnode, ATTR_Note,     (plev.parsedkey < NoteNames.size()) ? NoteNames[plev.parsedkey] : std::to_string(plev.parsedkey));
            if(plev.octmod != 0)
                AppendAttribute(evnode, ATTR_ModOctave,   plev.octmod);
            if(plev.holdtime > 0)
                AppendAttribute(evnode, ATTR_Duration, plev.holdtime);
        }

        void WriteWaitEvent(xml_node parent, const TrkEvent& ev)
        {
            using namespace MusicSeqXML;
            const eTrkEventCodes code     = static_cast<eTrkEventCodes>(ev.evcode);
            xml_node             waitnode = AppendChildNode(parent, NODE_Wait);
            auto                 evinfo   = GetEventInfo(code);
            if (!evinfo)
                throw std::runtime_error("Couldn't get matching event info for event number #\"" + std::to_string((uint8_t)code) + "\"");
            const TrkEventInfo   info     = *evinfo;
            AppendAttribute(waitnode, ATTR_Type, info.evlbl);

            if (code == eTrkEventCodes::RepeatLastPause)
                return; //No parameters

            uint32_t durationparam = 0;
            switch (code)
            {
            case eTrkEventCodes::AddToLastPause:
            case eTrkEventCodes::Pause8Bits:
            case eTrkEventCodes::PauseUntilRel:
                durationparam = ev.params.front();
                break;
            case eTrkEventCodes::Pause16Bits:
                durationparam = (static_cast<uint32_t>(ev.params[1]) << 8) | ev.params[0];
                break;
            case eTrkEventCodes::Pause24Bits:
                durationparam = (static_cast<uint32_t>(ev.params[2]) << 16) | (static_cast<uint32_t>(ev.params[1]) << 8) | ev.params[0];
                break;
            };

            AppendAttribute(waitnode, ATTR_Duration, durationparam);
        }

        void WriteEvent(xml_node parent, const TrkEvent& ev)
        {
            using namespace MusicSeqXML;
            const eTrkEventCodes code = static_cast<eTrkEventCodes>(ev.evcode);
            xml_node             evnode = AppendChildNode(parent, NODE_Event);
            auto                 evinfo = GetEventInfo(code);
            if (!evinfo)
                throw std::runtime_error("Couldn't get matching event info for event number #\"" + std::to_string((uint8_t)code) + "\"");
            const TrkEventInfo   info = *evinfo;
            AppendAttribute(evnode, ATTR_Type, info.evlbl);

            switch (code)
            {
            case eTrkEventCodes::SetTempo:
            case eTrkEventCodes::SetTempo2:
                AppendAttribute(evnode, ATTR_BPM, ev.params.front());
                break;

            case eTrkEventCodes::RepeatFrom:
                AppendAttribute(evnode, ATTR_Repeats, ev.params.front());
                break;

            case eTrkEventCodes::SetOctave:
            case eTrkEventCodes::AddOctave:
                AppendAttribute(evnode, ATTR_Octave, ev.params.front());
                break;

            case eTrkEventCodes::SetBank:
                AppendAttribute(evnode, ATTR_BankHi, ev.params[0]);
                AppendAttribute(evnode, ATTR_BankLo, ev.params[1]);
                break;
            case eTrkEventCodes::SetBankHighByte:
                AppendAttribute(evnode, ATTR_BankHi, ev.params[0]);
                break;
            case eTrkEventCodes::SetBankLowByte:
                AppendAttribute(evnode, ATTR_BankLo, ev.params[0]);
                break;

            case eTrkEventCodes::SetProgram:
                AppendAttribute(evnode, ATTR_Program, ev.params[0]);
                break;

            case eTrkEventCodes::SweepTrackVol:
            case eTrkEventCodes::FadeSongVolume:
                AppendAttribute(evnode, ATTR_Rate, (uint16_t)((ev.params[1] << 8) | ev.params[0]));
                AppendAttribute(evnode, ATTR_Vol, ev.params[2]);
                break;

            case eTrkEventCodes::SetEnvAtkTime:
            case eTrkEventCodes::SetEnvHold:
            case eTrkEventCodes::SetEnvFade:
            case eTrkEventCodes::SetEnvRelease:
                AppendAttribute(evnode, ATTR_Level, ev.params[0]);
                break;

            case eTrkEventCodes::SetTrkPan:
            case eTrkEventCodes::AddTrkPan:
            case eTrkEventCodes::SetChanPan:
                AppendAttribute(evnode, ATTR_Pan, ev.params[0]);
                break;

            case eTrkEventCodes::SweepTrkPan:
                AppendAttribute(evnode, ATTR_Rate, (uint16_t)((ev.params[1] << 8) | ev.params[0]));
                AppendAttribute(evnode, ATTR_Pan, ev.params[2]);
                break;

            case eTrkEventCodes::SetExpress:
            case eTrkEventCodes::SetTrkVol:
            case eTrkEventCodes::AddTrkVol:
            case eTrkEventCodes::SetNoteVol:
            case eTrkEventCodes::SetChanVol:
            case eTrkEventCodes::SetEnvAtkLvl:
                AppendAttribute(evnode, ATTR_Vol, ev.params[0]);
                break;

            case eTrkEventCodes::SetEnvDecSus:
                AppendAttribute(evnode, ATTR_Decay,   ev.params[0]);
                AppendAttribute(evnode, ATTR_Sustain, ev.params[1]);
                break;

            case eTrkEventCodes::SetFTune:
            case eTrkEventCodes::AddFTune:
            case eTrkEventCodes::SetCTune:
            case eTrkEventCodes::AddCTune:
                AppendAttribute(evnode, ATTR_Semitones, ev.params[0]);
                break;

            case eTrkEventCodes::SweepTune:
                AppendAttribute(evnode, ATTR_Rate, (uint16_t)((ev.params[1] << 8) | ev.params[0]));
                AppendAttribute(evnode, ATTR_Semitones, ev.params[2]);
                break;

            case eTrkEventCodes::SetRndNoteRng:
                AppendAttribute(evnode, ATTR_Min, ev.params[0]);
                AppendAttribute(evnode, ATTR_Max, ev.params[1]);
                break;

            case eTrkEventCodes::SetDetuneRng:
                AppendAttribute(evnode, ATTR_Semitones, (uint16_t)((ev.params[1] << 8) | ev.params[0]));
                break;

            case eTrkEventCodes::SetPitchBend:
                AppendAttribute(evnode, ATTR_Bend, (int16_t)((ev.params[0] << 8) | ev.params[1])); //Big endian aparently
                break;

            case eTrkEventCodes::SetPitchBendRng:
                AppendAttribute(evnode, ATTR_Semitones, ev.params[0]);
                break;

            case eTrkEventCodes::RouteLFO1ToPitch:
            case eTrkEventCodes::RouteLFO2ToVol:
            case eTrkEventCodes::RouteLFO3ToPan:
                AppendAttribute(evnode, ATTR_On, (bool)ev.params[0]);
                break;

            case eTrkEventCodes::SetLFO:
            case eTrkEventCodes::SetLFO1:
            case eTrkEventCodes::SetLFO2:
            case eTrkEventCodes::SetLFO3:
                AppendAttribute(evnode, ATTR_Rate,   (uint16_t)((ev.params[1] << 8) | ev.params[0]));
                AppendAttribute(evnode, ATTR_Depth,  (uint16_t)((ev.params[3] << 8) | ev.params[2]));
                AppendAttribute(evnode, ATTR_WShape, ev.params[4]);
                break;

            case eTrkEventCodes::SetLFODelFade:
            case eTrkEventCodes::SetLFO1DelayFade:
            case eTrkEventCodes::SetLFO2DelFade:
            case eTrkEventCodes::SetLFO3DelFade:
                AppendAttribute(evnode, ATTR_Delay, (uint16_t)((ev.params[1] << 8) | ev.params[0]));
                AppendAttribute(evnode, ATTR_Fade,  (uint16_t)((ev.params[3] << 8) | ev.params[2]));
                break;

            case eTrkEventCodes::SetLFOParam:
                AppendAttribute(evnode, ATTR_ID,    ev.params[0]);
                AppendAttribute(evnode, ATTR_Value, ev.params[1]);
                break;
                    
            case eTrkEventCodes::SetLFORoute:
                AppendAttribute(evnode, ATTR_ID,     ev.params[0]);
                AppendAttribute(evnode, ATTR_On,     (bool)ev.params[1]);
                AppendAttribute(evnode, ATTR_Target, ev.params[2]);
                break;

            case eTrkEventCodes::EndOfTrack:
            case eTrkEventCodes::DisableEnvelope:
            case eTrkEventCodes::RepeatSegment:
            case eTrkEventCodes::AfterRepeat:
            case eTrkEventCodes::LoopPointSet:
            case eTrkEventCodes::SkipNextByte:
            case eTrkEventCodes::SkipNext2Bytes1:
            case eTrkEventCodes::SkipNext2Bytes2:
            case eTrkEventCodes::Unk_0xC0:
                //No params
                break;

            case eTrkEventCodes::Unk_0xF6:
            case eTrkEventCodes::Unk_0xBF:
                AppendAttribute(evnode, ATTR_Unk, ev.params[0]);
                break;

            case eTrkEventCodes::Unk_0xD8:
                AppendAttribute(evnode, ATTR_Unk, ev.params[0]);
                AppendAttribute(evnode, ATTR_Unk2, ev.params[1]);
                break;

            default:
                //Ignored
                break;
            };
        }
    };

    //
    // Functions
    //
    void MusicSequenceToXML(const DSE::MusicSequence& seq, const std::string& filepath)
    {
        MusicSequenceXMLWriter(seq).operator()(filepath);
    }

    DSE::MusicSequence XMLToMusicSequence(const std::string& filepath)
    {
        return MusicSequenceXMLParser()(filepath);
    }

    bool IsXMLMusicSequence(const std::string& xmlfilepath)
    {
        using namespace MusicSeqXML;
        xml_document doc;
        doc.load_file(xmlfilepath.c_str());
        return (doc.child(NODE_Sequence.c_str()));
    }
};