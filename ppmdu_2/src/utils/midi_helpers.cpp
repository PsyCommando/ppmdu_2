#include <utils/midi_helpers.hpp>

#include <jdksmidi/midi.h>
#include <jdksmidi/msg.h>
#include <jdksmidi/track.h>

#include <array>
#include <cstdint>

namespace utils
{
    //Constants
    const std::array<uint8_t, 9> RolandGSReset
    { {
            0x41, //Roland's ID
            0x10, //Device ID, 0x10 is default
            0x42, //Model ID, 0x42 is universal for Roland
            0x12, //0x12 means we're sending data
            0x40, //highest byte of address
            0x00, //mid byte of address
            0x7F, //lowest byte of address
            0x00, //data
            0x41, //checksum
    } };
    const std::array<uint8_t, 9> RolandGSDrumChanOff
    { {
            0x41, //Roland's ID
            0x10, //Device ID, 0x10 is default
            0x42, //Model ID, 0x42 is universal for Roland
            0x12, //0x12 means we're sending data
            0x40, //highest byte of address
            0x10, //mid byte of address
            0x15, //lowest byte of address
            0x00, //data
            0x1B, //checksum
    } };

    const std::array<uint8_t, 9> YamahaXGReset
    { {
            0x43, //Yamaha's ID
            0x10, //Device ID, 0x10 is default 
            0x4C,
            0x00,
            0x00,
            0x7E,
            0x00
    } };

    void WriteGSSysex(jdksmidi::MIDITrack* mastertrk)
    {
        using namespace jdksmidi;
        //Reset synth to GS mode
        MIDISystemExclusive mysysex(const_cast<uint8_t*>(RolandGSReset.data()), RolandGSReset.size(), RolandGSReset.size(), false); //** Must const_cast here, because for some reasons there's no constructor for a const array
        MIDITimedBigMessage gsreset;
        gsreset.SetDataLength(RolandGSReset.size());
        gsreset.CopySysEx(&mysysex);
        gsreset.SetSysEx(jdksmidi::SYSEX_START_N);
        mastertrk->PutEvent(gsreset);

        MIDITimedBigMessage sysendmsg;
        sysendmsg.SetSysEx(jdksmidi::SYSEX_END);
        mastertrk->PutEvent(sysendmsg);
    }

    //Functions
    void WriteGSDrumsOffSysex(jdksmidi::MIDITrack* mastertrk)
    {
        using namespace jdksmidi;
        MIDISystemExclusive mysysex(const_cast<uint8_t*>(RolandGSDrumChanOff.data()), RolandGSDrumChanOff.size(), RolandGSDrumChanOff.size(), false);
        MIDITimedBigMessage gsoffdrums;
        gsoffdrums.SetDataLength(RolandGSDrumChanOff.size());
        gsoffdrums.CopySysEx(&mysysex);
        gsoffdrums.SetSysEx(jdksmidi::SYSEX_START_N);
        mastertrk->PutEvent(gsoffdrums);

        MIDITimedBigMessage sysendmsg;
        sysendmsg.SetSysEx(jdksmidi::SYSEX_END);
        mastertrk->PutEvent(sysendmsg);
    }

    void WriteXGSysex(jdksmidi::MIDITrack* mastertrk)
    {
        using namespace jdksmidi;
        MIDISystemExclusive mysysex(const_cast<uint8_t*>(YamahaXGReset.data()), YamahaXGReset.size(), YamahaXGReset.size(), false);
        MIDITimedBigMessage xgreset;
        xgreset.SetDataLength(YamahaXGReset.size());
        xgreset.CopySysEx(&mysysex);
        xgreset.SetSysEx(jdksmidi::SYSEX_START_N);
        //Not sure if this all that I should do.. Documentation is sparse for XG
        mastertrk->PutEvent(xgreset);

        MIDITimedBigMessage sysendmsg;
        sysendmsg.SetSysEx(jdksmidi::SYSEX_END);
        mastertrk->PutEvent(sysendmsg);
    }

    void PutRPNValue(unsigned long time, uint8_t chanid, jdksmidi::MIDITrack& trkout, uint16_t rpn_id, uint16_t rpn_value)
    {
        using namespace jdksmidi;
        MIDITimedBigMessage msg;
        msg.SetTime(time);

        //Set active function to what we picked
        msg.SetControlChange(chanid, jdksmidi::C_RPN_LSB, (uint8_t)((rpn_id & MIDICCHighByteMask) >> 8));
        trkout.PutEvent(msg);
        msg.SetControlChange(chanid, jdksmidi::C_RPN_MSB, (uint8_t)rpn_id);
        trkout.PutEvent(msg);
        
        //Insert the parameters for the function in the data entry CC
        msg.SetControlChange(chanid, eMidiCC::C_DataEntryLSB, (uint8_t)((rpn_value & MIDICCHighByteMask) >> 8));
        trkout.PutEvent(msg);
        msg.SetControlChange(chanid, jdksmidi::C_DATA_ENTRY, (uint8_t)rpn_value);
        trkout.PutEvent(msg);

        //Send null function to clear the active function
        msg.SetControlChange(chanid, jdksmidi::C_RPN_LSB, 0x7F);
        trkout.PutEvent(msg);
        msg.SetControlChange(chanid, jdksmidi::C_RPN_MSB, 0x7F);
        trkout.PutEvent(msg);
    }

    void PutGenericSysExData(unsigned long time, uint8_t chanid, jdksmidi::MIDITrack& trkout, const std::vector<uint8_t>& msgdata)
    {
        using namespace jdksmidi;
        //Fill up the message
        MIDISystemExclusive sysex;
        sysex.PutEXC();
        sysex.PutByte(NonCommercialSysexID);
        sysex.PutByte(chanid);
        for (uint8_t b : msgdata)
            sysex.PutByte(b);
        sysex.PutEOX();

        MIDITimedBigMessage msg;
        msg.SetTime(time);
        msg.CopySysEx(&sysex);
        trkout.PutEvent(msg);
    }

};