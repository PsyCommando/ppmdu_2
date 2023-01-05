#ifndef MIDI_HELPERS_HPP
#define MIDI_HELPERS_HPP
#include <utils/common_suffixes.hpp>

namespace utils
{

//
// Constants
//
    const uint8_t NbMidiKeysInOctave = 12;
    const uint8_t MIDIDrumChannel    = 9;

//
// Enums
// 
    // Midi controllers not defined in jdksmidi
    enum eMidiCC : uint8_t
    {
        C_SoundReleaseTime = 72ui8,
        C_SoundAttackTime  = 73ui8,
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
        SingleTrack,
        MultiTrack,
    };

    //
    // Convertion Functions
    //
    const uint32_t NbMicrosecPerMinute = 60000000;

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
