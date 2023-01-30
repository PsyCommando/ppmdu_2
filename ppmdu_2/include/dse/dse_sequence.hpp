#ifndef DSE_SEQUENCE_HPP
#define DSE_SEQUENCE_HPP
/*
dse_sequence.hpp
2015/06/26
psycommando@gmail.com
Description: Contains utilities to deal with DSE event tracks. Or anything within a "trk\20" chunk.
*/
#include <dse/dse_common.hpp>
#include <utils/library_wide.hpp>
#include <map>
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>
#include <set>

namespace DSE
{
    //Forward Decl
    struct TrkEventInfo;

//====================================================================================================
//  Typedefs / Enums / Constants
//====================================================================================================

    const uint16_t     DefaultTickRte           = 48;                //Default tick rate per quarter note of the Digital Sound Element Sound Driver for sequence playback.
    const unsigned int NbTrkDelayValues         = 16;                //The nb of track delay prefixes in the DSE format
    const uint32_t     TrkParam1Default         = 0x01000000;        //The default value for the parameter 1 value in the trk chunk header!
    const uint32_t     TrkParam2Default         = 0x0000FF04;        //The default value for the parameter 2 value in the trk chunk header!
    const uint8_t      NoteEvParam1NoteMask     = 0b0000'1111;       //The id of the note "eDSENote" is stored in the lower nybble
    const uint8_t      NoteEvParam1PitchMask    = 0b0011'0000;       //The value of those 2 bits in the "param1" of a NoteOn event indicate if/how to modify the track's current pitch.
    const uint8_t      NoteEvParam1NbParamsMask = 0b1100'0000;       //The value of those two bits indicates the amount of bytes to be parsed as parameters for the note on event
    const int8_t       NoteEvOctaveShiftRange   = 2;                 //The Nb of octave the note event can modify the track's current octave

    /// <summary>
    /// An enum of all the 16 possible delay events supported in DSE.
    /// The underlying value is the amount of ticks the associated event lasts.
    /// </summary>
    enum struct eTrkDelays : uint8_t
    {
        _half           = 96, // 1/2 note
        _dotqtr         = 72, // dotted 1/4 note
        _two3rdsofahalf = 64, // (1/2 note)/3 * 2
        _qtr            = 48, // 1/4 note
        _dot8th         = 36, // dotted 1/8 note
        _two3rdsofqtr   = 32, // (1/4 note)/3 * 2
        _8th            = 24, // 1/8 note
        _dot16th        = 18, // dotted 1/16 note
        _two3rdsof8th   = 16, // (1/8 note)/3 * 2
        _16th           = 12, // 1/16 note
        _dot32nd        =  9, // dotted 1/32
        _two3rdsof16th  =  8, // (1/16 note)/3 * 2
        _32nd           =  6, // 1/32 note
        _dot64th        =  4, // dotted 1/64 note
        _two3rdsof32th  =  3, // (1/32 note)/3 * 2  #NOTE: Its not truly 2/3 of a 32th note, because the game seems to round the duration, so its not identical to the last one.
        _64th           =  2, // 1/64 note          #NOTE: Its not truly a 64th note, because the game seems to round the duration, so its not identical to the last one.
    };

    /// <summary>
    /// Enum for all the possible DSE sequence events or ranges of events.
    /// The underlying value is the event code corresponding to the event in a DSE track.
    /// All event codes missing from the ranges or values covered in the enum should be considered a bad event that will stop playback.
    /// </summary>
    enum struct eTrkEventCodes : uint8_t
    {
        Invalid         = 0x00, //#TODO: check if this is true at all times, because I remember seeing some tracks that used these and not as an end of track marker of sort

        //Reserved range for NoteOn + Velocity
        NoteOnBeg       = 0x01, //The first event code reserved for play note events.
        NoteOnEnd       = 0x7F, //The last event code that is reserved for playing a note.

        //Delays
        Delay_HN        = 0x80, // 1/2 note
        Delay_DQN       = 0x81, // dotted 1/4 note
        Delay_HN3       = 0x82, // (1/2 note)/3 * 2
        Delay_QN        = 0x83, // 1/4 note
        Delay_D8N       = 0x84, // dotted 1/8 note
        Delay_QN3       = 0x85, // (1/4 note)/3 * 2
        Delay_8N        = 0x86, // 1/8 note
        Delay_D16N      = 0x87, // dotted 1/16 note
        Delay_8N3       = 0x88, // (1/8 note)/3 * 2
        Delay_16N       = 0x89, // 1/16 note
        Delay_D32N      = 0x8A, // dotted 1/32
        Delay_16N3      = 0x8B, // (1/16 note)/3 * 2
        Delay_32N       = 0x8C, // 1/32 note
        Delay_D64N      = 0x8D, // dotted 1/64 note
        Delay_32N3      = 0x8E, // (1/32 note)/3 * 2
        Delay_64N       = 0x8F, // 1/64 note

        //Non-play note events
        RepeatLastPause = 0x90, //Repeat the last silence
        AddToLastPause  = 0x91, //Pause the track for the duration of the last pause + the duration specified
        Pause8Bits      = 0x92, //Pause the track for specified duration (uses a uint8)
        Pause16Bits     = 0x93, //Pause the track for specified duration (uses a uint16)
        Pause24Bits     = 0x94, //Pause the track for specified duration (uses a uint24)
        PauseUntilRel   = 0x95, //Pause until the noteOff event for the last playing note is received. Will always wait at least as long as its check interval parameter.

        _BAD_0x96       = 0x96,
        _BAD_0x97       = 0x97,

        EndOfTrack      = 0x98, //Marks the end of the track. Also serve as padding.
        LoopPointSet    = 0x99, //Marks the location where the track should loop from.

        _BAD_0x9A       = 0x9A,
        _BAD_0x9B       = 0x9B,

        RepeatFrom      = 0x9C, //Marks the location any subsequent "RepeatSegment" events should repeat from, and indicates the amount of times to repeat.
        RepeatSegment   = 0x9D, //Repeat the segment starting at the last "RepeatFrom" event. 
        AfterRepeat     = 0x9E, //After the last "RepeatSegment" event has finished its repeats, playback will jump here.

        _BAD_0x9F       = 0x9F,

        SetOctave       = 0xA0, //Sets the octave notes are currently played at.
        AddOctave       = 0xA1, //Adds the given value to the current octave.

        _BAD_0xA2       = 0xA2,
        _BAD_0xA3       = 0xA3,

        SetTempo        = 0xA4, //Sets the tempo of the track in BPM.
        SetTempo2       = 0xA5, //Also sets the tempo of the track in BPM.

        _BAD_0xA6       = 0xA6,
        _BAD_0xA7       = 0xA7,

        SetBank         = 0xA8, //Set both bytes of the bank id.
        SetBankHighByte = 0xA9, //Sets bank id's high byte.
        SetBankLowByte  = 0xAA, //Set bank id's low byte.
        SkipNextByte    = 0xAB, //Skip processing the next byte!
        SetProgram      = 0xAC, //Sets the instrument preset to use

        _BAD_0xAD       = 0xAD,
        _BAD_0xAE       = 0xAE,

        FadeSongVolume  = 0xAF, //Sweep the song's volume. First arg is the rate, second is the target volume.
        DisableEnvelope = 0xB0, //Disable envelope
        SetEnvAtkLvl    = 0xB1, //Sets the enveloppe's attack parameter on the current program
        SetEnvAtkTime   = 0xB2, //Set the envelope attack time parameter on the current program
        SetEnvHold      = 0xB3, //Set envelope hold parameter on the current program
        SetEnvDecSus    = 0xB4, //Set envelope decay and sustain on the current program
        SetEnvFade      = 0xB5, //Set envelope fade parameter on current program
        SetEnvRelease   = 0xB6, //Set envelope release parameter on current program

        _BAD_0xB7       = 0xB7,
        _BAD_0xB8       = 0xB8,
        _BAD_0xB9       = 0xB9,
        _BAD_0xBA       = 0xBA,
        _BAD_0xBB       = 0xBB,

        SetNoteVol      = 0xBC, //SetNoteVolume (?) 

        _BAD_0xBD       = 0xBD,
        
        SetChanPan      = 0xBE, //Sets current channel panning
        Unk_0xBF        = 0xBF, //Unknown //#TODO
        Unk_0xC0        = 0xC0, //Unknown //#TODO

        _BAD_0xC1       = 0xC1,
        _BAD_0xC2       = 0xC2,

        SetChanVol      = 0xC3, //Sets current channel volume

        _BAD_0xC4       = 0xC4,
        _BAD_0xC5       = 0xC5,
        _BAD_0xC6       = 0xC6,
        _BAD_0xC7       = 0xC7,
        _BAD_0xC8       = 0xC8,
        _BAD_0xC9       = 0xC9,
        _BAD_0xCA       = 0xCA,

        SkipNext2Bytes1 = 0xCB,

        _BAD_0xCC       = 0xCC,
        _BAD_0xCD       = 0xCD,
        _BAD_0xCE       = 0xCE,
        _BAD_0xCF       = 0xCF,

        SetFTune        = 0xD0, //Sets fine tune
        AddFTune        = 0xD1, //Adds value to current fine tune
        SetCTune        = 0xD2, //Sets coarse tune
        AddCTune        = 0xD3, //Adds value to current coarse tune
        SweepTune       = 0xD4, //Interpolate between the given tune values
        SetRndNoteRng   = 0xD5, //Sets random notes range
        SetDetuneRng    = 0xD6, //Sets detune range
        SetPitchBend    = 0xD7, //Sets the pitch bend
        Unk_0xD8        = 0xD8, //Unknown, possibly changes unused paramter? //#TODO

        _BAD_0xD9       = 0xD9,
        _BAD_0xDA       = 0xDA,

        SetPitchBendRng = 0xDB, //Set the bend range for pitch bending

        //LFO control
        SetLFO1         = 0xDC, //Sets LFO rate, depth, and waveform
        SetLFO1DelayFade= 0xDD, //Sets the LFO effect delay, and fade out

        _BAD_0xDE       = 0xDE,

        RouteLFO1ToPitch= 0xDF, //Route the LFO1 output to note pitch if set to > 0

        SetTrkVol       = 0xE0, //Sets primary track volume.
        AddTrkVol       = 0xE1, //Adds value to track volume
        SweepTrackVol   = 0xE2, //Interpolate track volume to to the specified value at the specified rate
        SetExpress      = 0xE3, //Sets secondary volume control. AKA expression or GM CC#11.
        SetLFO2         = 0xE4, //Sets LFO rate, depth, and waveform
        SetLFO2DelFade  = 0xE5, //Sets the LFO effect delay, and fade out

        _BAD_0xE6       = 0xE6,

        RouteLFO2ToVol  = 0xE7, //Route the LFO2 output to volume if set to > 0
        SetTrkPan       = 0xE8, //Sets the panning of the track.
        AddTrkPan       = 0xE9, //Adds value to track panning.
        SweepTrkPan     = 0xEA, //Interpolate the track's panning value to the specified value at the specified rate

        _BAD_0xEB       = 0xEB,

        SetLFO3         = 0xEC, //Sets LFO rate, depth, and waveform.
        SetLFO3DelFade  = 0xED, //Sets the LFO effect delay, and fade out

        _BAD_0xEE       = 0xEE,

        RouteLFO3ToPan  = 0xEF, //Routes the LFO3 output to the track panning value if > 0

        SetLFO          = 0xF0, //Sets LFO rate, depth, and waveform
        SetLFODelFade   = 0xF1, //Sets the LFO effect delay, and fade out
        SetLFOParam     = 0xF2, //Sets the LFO's parameter and its value
        SetLFORoute     = 0xF3, //Set what LFO is routed to what, and whether its enabled

        _BAD_0xF4       = 0xF4,
        _BAD_0xF5       = 0xF5,

        Unk_0xF6        = 0xF6, //Unknown //#TODO // Probably used to sync music to the script engine

        _BAD_0xF7       = 0xF7,

        SkipNext2Bytes2 = 0xF8, //Skip processing the next 2 bytes

        _BAD_0xF9       = 0xF9,
        _BAD_0xFA       = 0xFA,
        _BAD_0xFB       = 0xFB,
        _BAD_0xFC       = 0xFC,
        _BAD_0xFD       = 0xFD,
        _BAD_0xFE       = 0xFE,
        _BAD_0xFF       = 0xFF,
    };

    /// <summary>
    /// Enum list of the 12 notes from a single octave matching their value from the note event.
    /// *NOTE : 0xF is valid and used in some circumstances, but isn't in here, because more data is needed.
    /// </summary>
    enum struct eNote : uint8_t
    {
        C  = 0x0,
        Cs = 0x1,
        D  = 0x2,
        Ds = 0x3,
        E  = 0x4,
        F  = 0x5,
        Fs = 0x6,
        G  = 0x7,
        Gs = 0x8,
        A  = 0x9,
        As = 0xA,
        B  = 0xB,
        nbNotes, //Must be last of valid notes
        Invalid,
    };
    
    /// <summary>
    /// A map of the delay event codes (0x80-0x8F) matching a particular track delay enum value.
    /// </summary>
    extern const std::map<uint8_t, eTrkDelays> TrkDelayCodeVals;

    /// <summary>
    /// A map of each delay enum values, to their event code equivalents in a DSE sequence (0x80-0x8F).
    /// </summary>
    extern const std::map<eTrkDelays, uint8_t> TrkDelayToEvID;

    /// <summary>
    /// For a given amount of ticks, returns the matching track delay ID.
    /// Prefer using the function FindClosestTrkDelayID instead.
    /// </summary>
    extern const std::map<uint8_t, eTrkDelays> TicksToTrkDelayID;

    /// <summary>
    /// Array of all the track delay enum values from largest to lowest delay.
    /// </summary>
    extern const std::array<eTrkDelays, NbTrkDelayValues> TrkDelayCodesTbl;

    /// <summary>
    /// An array of strings representation of a note in order matching the values in the eNote enum.
    /// Only represents the 12 valid notes. Anything above 0xB isn't in this array.
    /// </summary>
    extern const std::array<std::string, static_cast<uint8_t>(eNote::nbNotes)> NoteNames;

    /// <summary>
    /// Contains details specifics on how to parse all event codes.
    /// </summary>
    extern const std::vector<TrkEventInfo> TrkEventsTable;

    /// <summary>
    /// List of bad DSE event codes that stop the tracker.
    /// </summary>
    extern const std::set<eTrkEventCodes> BadEvents;

//====================================================================================================
// Structs
//====================================================================================================
    class DSESequenceToMidi;

    /// <summary>
    /// Contains details specifics on how to parse an event codes.
    ///  **Used only for the event lookup table!**
    /// </summary>
    struct TrkEventInfo
    {
        //Event code range
        eTrkEventCodes evcodebeg = eTrkEventCodes::Invalid;   //Beginning of the range of event codes that can be used to represent this event.
        eTrkEventCodes evcodeend = eTrkEventCodes::Invalid;   //Leave to invalid when is event with single code
        //nb params
        uint32_t       nbreqparams = 0; //if has any required parameters
        std::string    evlbl;       //text label for the event, mainly for logging/debugging
    };

    /// <summary>
    /// Helper struct for containing the data parsed from a raw playnote event.
    /// </summary>
    struct ev_play_note
    {
        midinote_t mnoteid   = 0; //If applicable, the midi note id obtained when combining the note parsed, and the current track octave + octmod.
        uint8_t    param2len = 0; //length in bytes of the optional duration parameter following the playnote event.
        int8_t     octmod    = 0; //The value to be added to the current track octave.
        uint8_t    parsedkey = 0; //The note from 0x0 to 0xF from the playnote event.
        uint32_t   holdtime  = 0; //The optional duration to hold the note for.
    };

//====================================================================================================
// Track Data
//====================================================================================================

    /// <summary>
    /// Represent a raw track event used in the SEDL and SMDL format!
    /// </summary>
    struct TrkEvent
    {
        uint8_t              evcode = 0;
        std::vector<uint8_t> params;
    };

    /// <summary>
    /// First 4 bytes of data of a trk chunk. Contains track-specific info such as its trackID and its MIDI channel number.
    /// </summary>
    struct TrkPreamble
    {
        static constexpr uint32_t size() {return 4;}
        uint8_t trkid  = 0;
        uint8_t chanid = 0;
        uint8_t unk1   = 0;
        uint8_t unk2   = 0;

        /// <summary>
        /// 
        /// </summary>
        template<class _outit> _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( trkid,  itwriteto );
            itwriteto = utils::WriteIntToBytes( chanid, itwriteto );
            itwriteto = utils::WriteIntToBytes( unk1,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk2,   itwriteto );
            return itwriteto;
        }

        /// <summary>
        /// 
        /// </summary>
        template<class _init> _init ReadFromContainer( _init itReadfrom, _init itPastEnd )
        {
            trkid  = utils::ReadIntFromBytes<decltype(trkid)> ( itReadfrom, itPastEnd );
            chanid = utils::ReadIntFromBytes<decltype(chanid)>( itReadfrom, itPastEnd );
            unk1   = utils::ReadIntFromBytes<decltype(unk1)>  ( itReadfrom, itPastEnd );
            unk2   = utils::ReadIntFromBytes<decltype(unk2)>  ( itReadfrom, itPastEnd );
            return itReadfrom;
        }
    };

//====================================================================================================
// Functions
//====================================================================================================

    /// <summary>
    /// This interpret and returns the 3 values that are stored in the playnote event's first parameter.
    /// </summary>
    /// <param name="noteparam1">The first parameter byte of a raw playnote event to parse.</param>
    /// <param name="out_octdiff">The parsed octave modifier from the parameter.</param>
    /// <param name="out_param2len">The parsed duration/param2 bytes length from the parameter.</param>
    /// <param name="out_key">The parsed note (0x0-0xF) id from the parameter.</param>
    void ParsePlayNoteParam1(uint8_t  noteparam1, int8_t& out_octdiff, uint8_t& out_param2len, uint8_t& out_key);

    /// <summary>
    /// Parses a raw playnote event and puts the result in a struct.
    /// </summary>
    /// <param name="ev">The raw play note event.</param>
    /// <param name="curoctave">The octave the track is at currently, or 0. The octave modifier of the note will be added to this value in the resulting struct.</param>
    ev_play_note ParsePlayNote(const TrkEvent& ev, uint8_t curoctave = 0);

    /// <summary>
    /// Find the closest delay event code for a given number of ticks.
    /// </summary>
    /// <param name="delayticks">A delay duration in ticks to find an existing delay event for.</param>
    /// <returns>Returns a pair with the closest pause event found, and a boolean indicating if it could find a delay below eTrkDelays::_half(96) ticks.</returns>
    std::optional<eTrkDelays>  FindClosestTrkDelayID(uint8_t delayticks);

    /// <summary>
    /// Returns a TrkEventInfo struct for the given event code providing information on a given event/envent range.
    /// </summary>
    /// <param name="ev">The event code of the event to get the TrkEventInfo for.</param>
    /// <returns>Optionally returns a TrkEventInfo struct with the info on the event matching the event code passed as argument. Or nullopt.</returns>
    std::optional<TrkEventInfo> GetEventInfo(eTrkEventCodes ev);

    /// <summary>
    /// Convert an event name to an event code.
    /// </summary>
    /// <param name="str">The label for an event to be converted back into an event code.</param>
    /// <returns>The event code matching the string passed as arg, or eTrkEventCodes::Invalid if no matches could be found.</returns>
    eTrkEventCodes StringToEventCode(const std::string& str);

    /// <summary>
    /// Convert a string with the symbol of one of the 12 notes into a eNote enum value.
    /// </summary>
    /// <param name="str">The symbol of the note to convert into a eNote enum value.</param>
    /// <returns>The eNote value matching the string symbol. Or a value bigger than eNote::NbNote when no match could be found.</returns>
    eNote StringToNote(const std::string& str);

    /// <summary>
    /// Return a textual representation of a midi note id!
    /// </summary>
    /// <param name="midinote">A midi note to get a string for..</param>
    /// <returns>The string representation of the midi note in the format "[NoteName][OctaveNumber]".</returns>
    std::string MidiNoteIdToText(midinote_t midinote);

    /// <summary>
    /// This function can be used to parse a track of DSE events 
    /// into a vector of track events, and a track preamble.
    /// </summary>
    template<class _itin> std::pair<std::vector<TrkEvent>,TrkPreamble> ParseTrkChunk( _itin beg, _itin end )
    {
        using namespace std;
        ChunkHeader hdr;
        beg = hdr.ReadFromContainer(beg, end);

        if( hdr.label != static_cast<uint32_t>(eDSEChunks::trk) )
            throw runtime_error("ParseTrkChunk(): Unexpected chunk label !");

        if( static_cast<uint32_t>(abs(std::distance(beg,end))) < hdr.datlen )
            throw runtime_error("ParseTrkChunk(): Track chunk continues beyond the expected end !");

        //Set the actual end of the events track
        _itin itendevents = beg;
        std::advance( itendevents, hdr.datlen );

        vector<TrkEvent> events;
        TrkPreamble      preamb;
        beg = preamb.ReadFromContainer(beg, end);

        events.reserve( hdr.datlen ); //Reserve worst case scenario
        for_each( beg, 
                  itendevents, 
                  EventParser<back_insert_iterator<vector<TrkEvent>>>(back_inserter(events)) );
        events.shrink_to_fit();       //Dealloc unused space

        return move( make_pair( std::move(events), std::move(preamb) ) );
    }

    /*****************************************************************
        WriteTrkChunk
            This function can be used to write a track of DSE events 
            into a container using an insertion iterator. 

            - writeit   : Iterator to insert into the destination container.
            - preamble  : track preamble info.
            - evbeg     : iterator to the beginning of the events track.
            - evend     : iterator to the end of the events track.
            - nbenvents : nb of events in the range. Saves a call to std::distance!

    *****************************************************************/
    template<class _backinsit, class _inevit> size_t WriteTrkChunk( _backinsit       & writeit, 
                                 const TrkPreamble & preamble, 
                                 _inevit             evbeg, 
                                 _inevit             evend,
                                 size_t              nbenvents )
    {
        using namespace std;

        //Count track size
        size_t tracklen = 0;
        for( auto itr = evbeg; itr != evend; ++itr ) tracklen += (itr->params.size() + 1);

        //Write header
        ChunkHeader hdr;
        hdr.label  = static_cast<uint32_t>(eDSEChunks::trk);
        hdr.datlen = TrkPreamble::size() + tracklen;   //We don't need to count padding here
        hdr.param1 = TrkParam1Default;
        hdr.param2 = TrkParam2Default;

        writeit = hdr.WriteToContainer( writeit );

        //Write preamble
        preamble.WriteToContainer( writeit );

        //Write events
        for( ; evbeg != evend; ++evbeg )
        {
            (*writeit) = evbeg->evcode;
            ++writeit;
            for( const auto & aparam : (evbeg->params) )
            {
                (*writeit) = aparam;
                ++writeit;
            }
        }

        return ((size_t)hdr.Size + hdr.datlen);
    }


    template<class _backinsit, class _inevit> _backinsit WriteTrkChunk( _backinsit         writeit, 
                                 const TrkPreamble & preamble, 
                                 _inevit             evbeg, 
                                 _inevit             evend )
    {
        return WriteTrkChunk( writeit, preamble, evbeg, evend, static_cast<size_t>(std::distance( evbeg, evend )) );
    }

//====================================================================================================
// EventParser
//====================================================================================================

    /// <summary>
    /// Pass the bytes of an event track, after the preamble,
    ///  and it will eat bytes and turn them into events within 
    /// the container refered to by the iterator !
    /// </summary>
    template<class _outit> class EventParser
    {
    public:
        typedef _outit outit_t;

        EventParser(_outit itout) :m_itDest(itout), m_hasBegun(false), m_bytesToRead(0) {}

        //Feed bytes to this 
        void operator()(uint8_t by)
        {
            if (!m_hasBegun) //If we haven't begun parsing an event
            {
                //if( isDelayCode(by) )   //If we read a delay code, just put it in our upcoming event
                //    m_curEvent.dt = by;
                //else
                beginNewEvent(by);
            }
            else
                fillEvent(by);
        }

    private:
        void beginNewEvent(uint8_t by)
        {
            auto einfo = GetEventInfo(static_cast<eTrkEventCodes>(by));

            if (!einfo) //If the event was not found
            {
                std::stringstream sstr;
                sstr << "EventParser::beginNewEvent(): Unknown event type 0x" << std::hex << static_cast<uint16_t>(by) << std::dec
                    << " encountered! Cannot continue due to unknown parameter length and possible resulting mis-alignment..";
                throw std::runtime_error(sstr.str());
            }

            m_curEventInf = *einfo;
            m_curEvent.evcode = by;

            if (m_curEventInf.nbreqparams == 0)
                endEvent(); //If its an event with 0 parameters end it now
            else
            {
                m_bytesToRead = m_curEventInf.nbreqparams; //Parse the required params first
                m_hasBegun = true;
            }
        }

        //This reads the parameter bytes for a given event. And reduce the nb of bytes to read counter
        void fillEvent(uint8_t by)
        {
            m_curEvent.params.push_back(by);

            if (m_curEvent.params.size() == 1 && (m_curEventInf.evcodebeg == eTrkEventCodes::NoteOnBeg))
                m_bytesToRead += (m_curEvent.params.front() & NoteEvParam1NbParamsMask) >> 6; //For play notes events, the nb of extra bytes of data to read is contained in bits 7 and 8

            --m_bytesToRead;

            if (m_bytesToRead == 0)
                endEvent();
        }

        void endEvent()
        {
            (*m_itDest) = std::move(m_curEvent);
            ++m_itDest;
            m_curEvent = TrkEvent(); //Re-init object state after move
            m_bytesToRead = 0;
            m_hasBegun = false;
        }

    private:
        outit_t      m_itDest;      // Output for assembled events
        bool         m_hasBegun;    // Whether we're working on an event right now
        uint32_t     m_bytesToRead; // this contains the amount of bytes to read before the event is fully parsed
        TrkEvent     m_curEvent;    // The event being assembled currently.
        TrkEventInfo m_curEventInf; // Info on the current event type
    };

};

//
//Stream Operators
//
std::ostream& operator<<(std::ostream& strm, const DSE::TrkEvent& ev);

#endif