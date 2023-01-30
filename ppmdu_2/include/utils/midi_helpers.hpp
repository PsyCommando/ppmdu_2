#ifndef MIDI_HELPERS_HPP
#define MIDI_HELPERS_HPP
#include <utils/common_suffixes.hpp>
#include <vector>

namespace jdksmidi {class MIDITrack;}

namespace utils
{
//==============================================================================
// Constants
//==============================================================================
    const uint8_t  NbMidiKeysInOctave  = 12;
    const uint8_t  MIDIDrumChannel     = 9;
    const uint32_t NbMicrosecPerMinute = 60000000;
    const uint16_t MIDICCHighByteMask  = 0b0011'1111'0000'0000;

    const uint8_t  NonCommercialSysexID = 0x7D;

//==============================================================================
// Enums
//==============================================================================

    /// <summary>
    /// MIOI controller change messages IDs.
    /// </summary>
    enum eMidiCC : uint8_t
    {
        C_DataEntryLSB     = 0x26,
        C_SoundReleaseTime = 72,
        C_SoundAttackTime  = 73,

        C_SoundDecayTime   = 75,

        //General Purpose CC we're using for non-standard things
        C_SoundAttackLevel = 80,
        C_SoundSusLevel    = 81,
        C_SoundHoldTime    = 82,
        C_SoundFadeTime    = 83,
    };

    /*************************************************************************************************
        eMIDIMode
            The MIDI file's "sub-standard".
            - GS inserts a GS Mode reset SysEx event, and then turns the drum channel off.
            - XG insets a XG reset Sysex event.
            - GM doesn't insert any special SysEx events.
    *************************************************************************************************/
    enum struct eMIDIMode
    {
        GM,
        GS,
        XG,
    };

    /*************************************************************************************************
        eMIDIFormat
            The standard MIDI file format to use to export the MIDI data.
            - SingleTrack : Is format 0, a single track for all events.
            - MultiTrack  : Is format 1, one dedicated tempo track, and all the other tracks for events.
    *************************************************************************************************/
    enum struct eMIDIFormat
    {
        SingleTrack, //Type 0
        MultiTrack,  //Type 1
        Type2,       //Type 2
    };

//==============================================================================
// Roland GS System Exclusive Messages
//==============================================================================

    /// <summary>
    /// Writes the system exclusive message to reset the device mode to Roland GS.
    /// </summary>
    /// <param name="mastertrk">Track to write the system exclusive message to.</param>
    void WriteGSSysex(jdksmidi::MIDITrack* mastertrk);

    /// <summary>
    /// Writes to the specified track the sysex used by Roland GS devices to turn off forced drum channels.
    /// </summary>
    /// <param name="mastertrk">Track to write the system exclusive message to.</param>
    void WriteGSDrumsOffSysex(jdksmidi::MIDITrack* mastertrk);

//==============================================================================
// Yamaha XG System Exclusive Messages
//==============================================================================

    /// <summary>
    /// Writes the system exclusive message to reset the device mode to Yamaha XG.
    /// </summary>
    /// <param name="mastertrk">Track to write the system exclusive message to.</param>
    void WriteXGSysex(jdksmidi::MIDITrack* mastertrk);


//
// 
// 
    void PutRPNValue(unsigned long time, uint8_t chanid, jdksmidi::MIDITrack& trkout, uint16_t rpn_id, uint16_t rpn_value);


    //Put a sysex with manufacturer id 0x7D which is the educational/non-commercial id slot.
    void PutGenericSysExData(unsigned long time, uint8_t chanid, jdksmidi::MIDITrack& trkout, const std::vector<uint8_t>& msgdata);

//==============================================================================
// Convertion Functions
//==============================================================================

    constexpr uint32_t ConvertTempoToMicrosecPerQuarterNote(uint32_t bpm)
    {
        return NbMicrosecPerMinute / bpm;
    }

    constexpr uint32_t ConvertMicrosecPerQuarterNoteToBPM(uint32_t mpqn)
    {
        return mpqn / NbMicrosecPerMinute;
    }
};
#endif // !MIDI_HELPERS_HPP
