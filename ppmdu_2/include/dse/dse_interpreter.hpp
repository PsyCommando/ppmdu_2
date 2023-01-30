#ifndef DSE_INTERPRETER_HPP
#define DSE_INTERPRETER_HPP
/*
dse_interpreter.hpp
2015/07/01
psycommando@gmail.com
Description: This class is meant to interpret a sequence of DSE audio events into standard MIDI events or text.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <dse/dse_conversion.hpp>
#include <dse/dse_common.hpp>
#include <dse/dse_sequence.hpp>
#include <dse/dse_conversion_info.hpp>

#include <utils/midi_helpers.hpp>

#include <string>
#include <deque>
#include <optional>

namespace DSE
{
//===============================================================================
// Constants
//===============================================================================

    /// <summary>
    /// String used to identify that a midi was exported using audioutil and what version of it was used.
    /// </summary>
    extern const std::string UtilityID;

    //Special text for midi text events used to preserve some dse specific events during conversion and import
    
    /// <summary>
    /// Used in imported/exported midis to mark the point where the song will loop from after reaching the end.
    /// This text event is supported by a few midi software synths like foobar2000.
    /// </summary>
    extern const std::string TXT_LoopStart;

    /// <summary>
    /// Used in imported/exported midis to mark the point where the song should loop from TXT_LoopStart.
    /// This text event is supported by a few midi software synths like foobar2000.
    /// </summary>
    extern const std::string TXT_LoopEnd;

    extern const std::string TXT_HoldNote;

    /// <summary>
    /// Text for midi text events that marks DSE events that have no MIDI equivalents.
    /// </summary>
    extern const std::string TXT_DSE_Event;

    extern const std::string TXT_RepeatMark;
    extern const std::string TXT_RepeatFromMark;
    extern const std::string TXT_AfterRepeatMark;

//===============================================================================
// Structs
//===============================================================================
    /*
    * Used to track notes currently being on for dealing with voice stealing and notes off events
    */
    struct NoteOnData
    {
        midinote_t noteid = 0;
        uint32_t   noteonticks = 0;
        uint32_t   noteoffticks = 0;
        size_t     noteonnum = 0; //Event num of the note on event
        size_t     noteoffnum = 0; //Event num of the eventual note off event
    };

    /***********************************************************************************
        TrkState
            Structure used for tracking the state of a track, to simulate events
            having only an effect at runtime.
    ***********************************************************************************/
    struct TrkState
    {
        uint8_t                trkid_  = 0; //Id of the track in the storage class this struct is in
        uint8_t                chanid_ = 0; //Id of the midi channel to use for events on this track 

        unsigned long          ticks_ = 0; //The current tick count for the track
        uint32_t               eventno_ = 0; //Event index counter to identify a single problematic event
        uint32_t               lastpause_ = 0; //Duration of the last pause event, including fixed duration pauses.
        uint32_t               lasthold_ = 0; //Last duration a note was held
        uint8_t                octave_ = 0; //The track's current octave
        dsepresetid_t          curprgm_ = 0; //Keep track of the current program to apply pitch correction on specific instruments
        bankid_t               curbank_ = 0;
        int8_t                 curmaxpoly_ = -1; //Maximum polyphony for current preset!

        int8_t                 trkvol_ = 0; //The volume the track is currently set to
        int8_t                 trkpan_ = 0; //The pan the track is currently set to
        int8_t                 chvol_  = 0; //Channel Volume (???)
        int8_t                 chpan_  = 0; //Channel pan (balance?)
        int8_t                 expr_   = 0; //Track expression volume

        int16_t                pitchbend_ = 0;
        uint8_t                bendrng_   = 0;
        int16_t                ftune_     = 0x2000; //A440 tuning
        int8_t                 ctune_     = 0x40;   //A440 tuning

        size_t                 looppoint_ = 0; //The index of the envent after the loop pos

        size_t                 repeatmark_ = 0; //Index of the last event indicating the start of a repeated segment.
        uint8_t                repeattimes_ = 1; //The amount of times to repeat the segment between the repeat mark and repeat message.
        size_t                 repeat_ = 0; //Index of the last event indicating we should repeat from the last mark.
        size_t                 afterrepmark_ = 0; //Index of the event indicating where to jump to after repeating the repeated segment.

        std::deque<NoteOnData> noteson_; //The notes currently on

        bool                   hasinvalidbank = false; //This is toggled when a bank couldn't be found. It stops all playnote events from playing. 

        int8_t                 transpose = 0; //The nb of octaves to transpose the notes played by this channel!

        //Those allows to keep track of when to revert to and from the overriden presets
        bool                   presetoverriden = false;
        bankid_t               ovrbank_ = InvalidBankID;
        dsepresetid_t          ovrprgm_ = InvalidDSEPresetID;
        dsepresetid_t          origdseprgm_ = 0; //The original program ID, not the one that has been remaped

        //This is for presets that overrides the channel they're played on
        bool                   chanoverriden = false;     //Whether the current preset overrides the channel
        uint8_t                ovrchan_ = std::numeric_limits<uint8_t>::max(); //The channel we override to
        uint8_t                chantoreinit = std::numeric_limits<uint8_t>::max(); //If we did override a note from this track, and played it on another channel, this indicates the channel we need to refresh the current preset + bank on!
    };

//===============================================================================
//  Export Utilities
//===============================================================================

    /*************************************************************************************************
        SequenceToMidi
            This function convert a MusicSequence to a midi according to the parameters specified!
                -outmidi    : The path and name of the MIDI file that will be exported.
                - seq       : The MusicSequence to export.
                - remapdata : Information on how each DSE track presets, translate to MIDI presets.
                - midmode   : The MIDI sub-standard to use for bypassing GM's limitations. 
                              GS is preferred as its the most supported one!
    *************************************************************************************************/
    void SequenceToMidi( const std::string              & outmidi, 
                         const MusicSequence            & seq, 
                         const SMDLPresetConversionInfo & remapdata,
                         int                              nbloop      = 0,
                         utils::eMIDIMode                 midmode     = utils::eMIDIMode::GS );

    /*************************************************************************************************
        SequenceToMidi
            Same as above, except without any preset conversion info!
    *************************************************************************************************/
    void SequenceToMidi( const std::string              & outmidi, 
                         const MusicSequence            & seq, 
                         int                              nbloop      = 0,
                         utils::eMIDIMode                 midmode     = utils::eMIDIMode::GS );

//===============================================================================
//  Import Utilities
//===============================================================================
    /*************************************************************************************************
        MidiToSequence
            Converts a MIDI file into a DSE Sequence.
            Optionally, pass a MusicSequence to base ourselves on for parsing.
            That sequence will be moved, modified, and outputed as return value if present.
    *************************************************************************************************/
    MusicSequence MidiToSequence(const std::string & inmidi, std::optional<MusicSequence> seq = std::nullopt);

};

#endif