#include <dse/dse_common.hpp>

#include <dse/fmts/smdl.hpp> //#TODO #MOVEME
#include <dse/fmts/sedl.hpp>
#include <ext_fmts/adpcm.hpp>

#include <utils/pugixml_utils.hpp>

#include <pugixml.hpp>

#include <sstream>
#include <iomanip>
#include <utility>
using namespace std;
using namespace pugi;
using namespace pugixmlutils;

namespace DSE
{
//=================================================================================================
//  Constants
//=================================================================================================
    const std::array<eDSEChunks, NB_DSEChunks> DSEChunksList 
    {{
        eDSEChunks::wavi, 
        eDSEChunks::prgi, 
        eDSEChunks::kgrp, 
        eDSEChunks::pcmd,
        eDSEChunks::trk,
        eDSEChunks::seq,
        eDSEChunks::bnkl,
        eDSEChunks::mcrl,
        eDSEChunks::eoc,
        eDSEChunks::eod,
    }};

    const std::array<eDSEContainers, NB_DSEContainers> DSEContainerList
    {{
        eDSEContainers::sadl,
        eDSEContainers::sedl,
        eDSEContainers::smdl,
        eDSEContainers::swdl,
    }};


    //Duration lookup tables for DSE volume envelopes extracted from the game:
    const std::array<int16_t,128> Duration_Lookup_Table =
    {
        0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 
        0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F, 
        0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 
        0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F, 
        0x0020, 0x0023, 0x0028, 0x002D, 0x0033, 0x0039, 0x0040, 0x0048, 
        0x0050, 0x0058, 0x0062, 0x006D, 0x0078, 0x0083, 0x0090, 0x009E, 
        0x00AC, 0x00BC, 0x00CC, 0x00DE, 0x00F0, 0x0104, 0x0119, 0x012F, 
        0x0147, 0x0160, 0x017A, 0x0196, 0x01B3, 0x01D2, 0x01F2, 0x0214, 
        0x0238, 0x025E, 0x0285, 0x02AE, 0x02D9, 0x0307, 0x0336, 0x0367, 
        0x039B, 0x03D1, 0x0406, 0x0442, 0x047E, 0x04C4, 0x0500, 0x0546, 
        0x058C, 0x0622, 0x0672, 0x06CC, 0x071C, 0x0776, 0x07DA, 0x0834, 
        0x0898, 0x0906, 0x096A, 0x09D8, 0x0A50, 0x0ABE, 0x0B40, 0x0BB8, 
        0x0C3A, 0x0CBC, 0x0D48, 0x0DDE, 0x0E6A, 0x0F00, 0x0FA0, 0x1040, 
        0x10EA, 0x1194, 0x123E, 0x12F2, 0x13B0, 0x146E, 0x1536, 0x15FE, 
        0x16D0, 0x17A2, 0x187E, 0x195A, 0x1A40, 0x1B30, 0x1C20, 0x1D1A, 
        0x1E1E, 0x1F22, 0x2030, 0x2148, 0x2260, 0x2382, 0x2710, 0x7FFF
    };
    
    //Duration lookup tables for DSE volume envelopes extracted from the game:
    const std::array<int32_t,128> Duration_Lookup_Table_NullMulti =
    {
        0x00000000, 0x00000004, 0x00000007, 0x0000000A, 
        0x0000000F, 0x00000015, 0x0000001C, 0x00000024, 
        0x0000002E, 0x0000003A, 0x00000048, 0x00000057, 
        0x00000068, 0x0000007B, 0x00000091, 0x000000A8, 
        0x00000185, 0x000001BE, 0x000001FC, 0x0000023F, 
        0x00000288, 0x000002D6, 0x0000032A, 0x00000385, 
        0x000003E5, 0x0000044C, 0x000004BA, 0x0000052E, 
        0x000005A9, 0x0000062C, 0x000006B5, 0x00000746, 
        0x00000BCF, 0x00000CC0, 0x00000DBD, 0x00000EC6, 
        0x00000FDC, 0x000010FF, 0x0000122F, 0x0000136C, 
        0x000014B6, 0x0000160F, 0x00001775, 0x000018EA, 
        0x00001A6D, 0x00001BFF, 0x00001DA0, 0x00001F51, 
        0x00002C16, 0x00002E80, 0x00003100, 0x00003395, 
        0x00003641, 0x00003902, 0x00003BDB, 0x00003ECA, 
        0x000041D0, 0x000044EE, 0x00004824, 0x00004B73, 
        0x00004ED9, 0x00005259, 0x000055F2, 0x000059A4, 
        0x000074CC, 0x000079AB, 0x00007EAC, 0x000083CE, 
        0x00008911, 0x00008E77, 0x000093FF, 0x000099AA, 
        0x00009F78, 0x0000A56A, 0x0000AB80, 0x0000B1BB, 
        0x0000B81A, 0x0000BE9E, 0x0000C547, 0x0000CC17, 
        0x0000FD42, 0x000105CB, 0x00010E82, 0x00011768, 
        0x0001207E, 0x000129C4, 0x0001333B, 0x00013CE2, 
        0x000146BB, 0x000150C5, 0x00015B02, 0x00016572, 
        0x00017015, 0x00017AEB, 0x000185F5, 0x00019133, 
        0x0001E16D, 0x0001EF07, 0x0001FCE0, 0x00020AF7, 
        0x0002194F, 0x000227E6, 0x000236BE, 0x000245D7, 
        0x00025532, 0x000264CF, 0x000274AE, 0x000284D0, 
        0x00029536, 0x0002A5E0, 0x0002B6CE, 0x0002C802, 
        0x000341B0, 0x000355F8, 0x00036A90, 0x00037F79, 
        0x000394B4, 0x0003AA41, 0x0003C021, 0x0003D654, 
        0x0003ECDA, 0x000403B5, 0x00041AE5, 0x0004326A, 
        0x00044A45, 0x00046277, 0x00047B00, 0x7FFFFFFF
    };

    const int SizeADPCMPreambleWords = ::audio::IMA_ADPCM_PreambleLen / sizeof(int32_t);

    const std::string DSE_SmplFmt_PCM16   = "PCM16"s;
    const std::string DSE_SmplFmt_PCM8    = "PCM8"s;
    const std::string DSE_SmplFmt_ADPCM4  = "ADPCM4"s;
    const std::string DSE_SmplFmt_ADPCM3  = "ADPCM3"s;
    const std::string DSE_SmplFmt_PSG     = "PSG"s;
    const std::string DSE_SmplFmt_INVALID = "Invalid"s;

    const std::unordered_map<std::string, eDSESmplFmt> StrToDseSmplFmtHashTbl
    {
        std::make_pair(DSE_SmplFmt_PCM8,   eDSESmplFmt::pcm8),
        std::make_pair(DSE_SmplFmt_PCM16,  eDSESmplFmt::pcm16     ),
        std::make_pair(DSE_SmplFmt_ADPCM4, eDSESmplFmt::ima_adpcm4),
        std::make_pair(DSE_SmplFmt_ADPCM3, eDSESmplFmt::ima_adpcm3),
        std::make_pair(DSE_SmplFmt_INVALID, eDSESmplFmt::invalid),
    };

    eDSESmplFmt IntToDSESmplFmt(std::underlying_type_t<eDSESmplFmt> val)
    {
        using underlying_t = std::underlying_type_t<eDSESmplFmt>;
        if (val == static_cast<underlying_t>(eDSESmplFmt::pcm8))
            return eDSESmplFmt::pcm8;
        else if (val == static_cast<underlying_t>(eDSESmplFmt::pcm16))
            return eDSESmplFmt::pcm16;
        else if (val == static_cast<underlying_t>(eDSESmplFmt::ima_adpcm4))
            return eDSESmplFmt::ima_adpcm4;
        else if (val == static_cast<underlying_t>(eDSESmplFmt::ima_adpcm3))
            return eDSESmplFmt::ima_adpcm3;
        else
            return eDSESmplFmt::invalid;
    }

    std::string DseSmplFmtToString(eDSESmplFmt fmt)
    {
        switch (fmt)
        {
        case eDSESmplFmt::pcm8:
            return DSE_SmplFmt_PCM8;
        case eDSESmplFmt::pcm16:
            return DSE_SmplFmt_PCM16;
        case eDSESmplFmt::ima_adpcm4:
            return DSE_SmplFmt_ADPCM4;
        case eDSESmplFmt::ima_adpcm3:
            return DSE_SmplFmt_ADPCM3;
        };
        std::array<char, 48> tmpchr{ 0 };
        snprintf(tmpchr.data(), tmpchr.size(), "Invalid (0x%x)", static_cast<std::underlying_type_t<eDSESmplFmt>>(fmt));
        return { tmpchr.data() };
    }

    eDSESmplFmt StringToDseSmplFmt(const char* str)
    {
        auto itfound = StrToDseSmplFmtHashTbl.find(str);
        if (itfound != StrToDseSmplFmtHashTbl.end())
            return itfound->second;
        return eDSESmplFmt::invalid;
    }

//=================================================================================================
//  DurationLookupTable stuff
//=================================================================================================

    int32_t DSEEnveloppeDurationToMSec( int8_t param, int8_t multiplier )
    { 
        param = utils::Clamp( abs(param), 0, 127 ); //Table indices go from 0 to 127
        if( multiplier == 0 )
            return (Duration_Lookup_Table_NullMulti[labs(param)]);
        else
            return (Duration_Lookup_Table[labs(param)] * multiplier);
    }

//
// DSE_MetaDataSEDL
//
    void DSE_MetaDataSEDL::setFromHeader(const DSE::SEDL_Header& hdr)
    {
        bankid_coarse = hdr.bankid_low;
        bankid_fine = hdr.bankid_high;
        fname = std::string(fname.begin(), fname.end());

        createtime.year = hdr.year;
        createtime.month = hdr.month;
        createtime.day = hdr.day;
        createtime.hour = hdr.hour;
        createtime.minute = hdr.minute;
        createtime.second = hdr.second;
        createtime.centsec = hdr.centisec;

        origversion = static_cast<eDSEVersion>(hdr.version);

        unk5 = hdr.unk5;
        unk6 = hdr.unk6;
        unk7 = hdr.unk7;
        unk8 = hdr.unk8;
    }

//
// SeqInfoXml
//

    namespace SeqInfoXml
    {
        const std::string PROP_;
    };
    void seqinfo_table::WriteXml(pugi::xml_node seqnode)const
    {
        using namespace SeqInfoXml;
        WriteNumberVarToXml(unk30, seqnode);
        WriteNumberVarToXml(unk16, seqnode);
        WriteNumberVarToXml(nbtrks, seqnode);
        WriteNumberVarToXml(nbchans, seqnode);
        WriteNumberVarToXml(unk19, seqnode);
        WriteNumberVarToXml(unk20, seqnode);
        WriteNumberVarToXml(unk21, seqnode);
        WriteNumberVarToXml(unk22, seqnode);
        WriteNumberVarToXml(unk23, seqnode);
        WriteNumberVarToXml(unk24, seqnode);
        WriteNumberVarToXml(unk25, seqnode);
        WriteNumberVarToXml(unk26, seqnode);
        WriteNumberVarToXml(unk27, seqnode);
        WriteNumberVarToXml(unk28, seqnode);
        WriteNumberVarToXml(unk29, seqnode);
        WriteNumberVarToXml(unk31, seqnode);
        WriteNumberVarToXml(unk12, seqnode);
    }
    void seqinfo_table::ParseXml(pugi::xml_node seqnode)
    {
        using namespace SeqInfoXml;
        ParseNumberVarFromXml(unk30,   seqnode);
        ParseNumberVarFromXml(unk16,   seqnode);
        ParseNumberVarFromXml(nbtrks,  seqnode);
        ParseNumberVarFromXml(nbchans, seqnode);
        ParseNumberVarFromXml(unk19,   seqnode);
        ParseNumberVarFromXml(unk20,   seqnode);
        ParseNumberVarFromXml(unk21,   seqnode);
        ParseNumberVarFromXml(unk22,   seqnode);
        ParseNumberVarFromXml(unk23,   seqnode);
        ParseNumberVarFromXml(unk24,   seqnode);
        ParseNumberVarFromXml(unk25,   seqnode);
        ParseNumberVarFromXml(unk26,   seqnode);
        ParseNumberVarFromXml(unk27,   seqnode);
        ParseNumberVarFromXml(unk28,   seqnode);
        ParseNumberVarFromXml(unk29,   seqnode);
        ParseNumberVarFromXml(unk31,   seqnode);
        ParseNumberVarFromXml(unk12,   seqnode);
    }

};

//==========================================================================================
//  StreamOperators
//==========================================================================================
    //Global stream operator
std::ostream& operator<<(std::ostream& os, const DSE::DateTime& obj)
{
    os  << static_cast<unsigned long>(obj.year) << "/"
        << static_cast<unsigned long>(obj.month) + 1 << "/"
        << static_cast<unsigned long>(obj.day) + 1 << "-"
        << static_cast<unsigned long>(obj.hour) << "h"
        << static_cast<unsigned long>(obj.minute) << "m"
        << static_cast<unsigned long>(obj.second) << "s";
    return os;
}

std::ostream& operator<<(std::ostream& strm, const DSE::KeyGroup& other)
{
    strm<< setfill('0') << setw(2)
        << other.id 
        << setfill(' ') << setw(8)
        << static_cast<short>(other.poly) 
        << setw(8)
        << static_cast<short>(other.priority) 
        << setw(8)
        << static_cast<short>(other.vclow) 
        << setw(8)
        << static_cast<short>(other.vchigh) 
        << setw(8)
        << static_cast<short>(other.unk50)    << " " << static_cast<short>(other.unk51)
        ;
    return strm;
}

std::ostream& operator<<(std::ostream& strm, const DSE::ProgramInfo& other)
{
    using namespace DSE;
    strm <<"\t" << std::setfill(' ') << "ID" << setw(8) << "Vol" << setw(8) << "Pan" << setw(8) << "Unk4" << setw(8) << "Poly?"
        << "\n";

    strm<<"\t" << std::setfill('0') << setw(2)
        << other.id
        << std::setfill(' ') << setw(8)
        << static_cast<short>(other.prgvol)
        << setw(8)
        << static_cast<short>(other.prgpan)
        << showbase << hex << setw(8)
        << static_cast<short>(other.unk4)
        << noshowbase << dec << setw(8)
        << static_cast<short>(other.unkpoly)
        << "\n"
        ;

    strm << "\t-- LFOs --\n"
        << "\t#" <<setfill(' ') << setw(8)
        << "Unk34"
        << setw(8)
        << "Unk52"
        << setw(8)
        << "Dest"
        << setw(8)
        << "Shape"
        << setw(8)
        << "Rate"
        << setw(8)
        << "Unk29"
        << setw(8)
        << "Depth"
        << setw(8)
        << "Delay"
        << setw(8)
        << "Unk32"
        << setw(8)
        << "Unk33"
        << "\n";

    //Write the LFOs
    int cntlfo = 0;
    for (const DSE::LFOTblEntry& lfoen : other.m_lfotbl)
    {
        strm<< "\t" 
            << cntlfo <<setfill(' ') << setw(8)
            << showbase <<hex
            << static_cast<short>(lfoen.unk34)
            << setw(8)
            << static_cast<short>(lfoen.unk52)
            << setw(8)
            << noshowbase << dec
            << static_cast<short>(lfoen.dest)
            << setw(8)
            << static_cast<short>(lfoen.wshape)
            << setw(8)
            << lfoen.rate
            << showbase << hex << setw(8)
            << lfoen.unk29
            << noshowbase << dec << setw(8)
            << setw(8)
            << lfoen.depth
            << setw(8)
            << lfoen.delay
            << showbase << hex << setw(8)
            << lfoen.unk32
            << setw(8)
            << lfoen.unk33 
            << noshowbase << dec
            << "\n"
            ;
        ++cntlfo;
    }

    strm << "\t-- Splits --\n"
        << "\t"
        << setfill(' ') << setw(3)
        << "ID"
        << setw(8)
        << "Unk11" 
        << setw(8)
        << "Unk25"
        << setw(8)
        << "lowkey"
        << setw(8)
        << "hikey"
        << setw(8)
        << "lowkey2"
        << setw(8)
        << "hikey2"
        << setw(8)
        << "lovel"
        << setw(8)
        << "hivel"
        << setw(8)
        << "lovel2"
        << setw(8)
        << "hivel2"
        << setw(8)
        << "smplID"
        << setw(8)
        << "ftune"
        << setw(8)
        << "ctune"
        << setw(8)
        << "key"
        << setw(8)
        << "ktps"
        << setw(8)
        << "vol"
        << setw(8)
        << "pan"
        << setw(8)
        << "kgrid"
        << setw(8)
        << "on"
        << setw(8)
        << "mult"
        << setw(44)
        << "atkv-atk-dec-sus-hold-dec2-rel"
        <<"\n"
        ;

    //Write the Splits
    int cntsplits = 0;
    for (const DSE::SplitEntry& split : other.m_splitstbl)
    {
        strm<< "\t"
            << std::setfill('0') << setw(3)
            << static_cast<short>(split.id)
            << std::setfill(' ') << setw(8)
            << showbase <<hex
            << static_cast<short>(split.bendrange)
            << setw(8)
            << static_cast<short>(split.unk25)
            << setw(8)
            << noshowbase << dec
            << static_cast<short>(split.lowkey)
            << setw(8) 
            << static_cast<short>(split.hikey)
            << setw(8)
            << static_cast<short>(split.lowkey2)
            << setw(8)
            << static_cast<short>(split.hikey2)
            << setw(8)
            << static_cast<short>(split.lovel)
            << setw(8)
            << static_cast<short>(split.hivel)
            << setw(8)
            << static_cast<short>(split.lovel2)
            << setw(8)
            << static_cast<short>(split.hivel2)
            << setw(8)
            << static_cast<short>(split.smplid)
            << setw(8)
            << static_cast<short>(split.ftune)
            << setw(8)
            << static_cast<short>(split.ctune)
            << setw(8)
            << static_cast<short>(split.rootkey)
            << setw(8)
            << static_cast<short>(split.ktps)
            << setw(8)
            << static_cast<short>(split.smplvol)
            << setw(8)
            << static_cast<short>(split.smplpan)
            << setw(8)
            << static_cast<short>(split.kgrpid)
            << setw(8)
            << static_cast<short>(split.envon)
            << setw(8)
            << static_cast<short>(split.env.envmulti)
            << setw(8)
            << static_cast<short>(split.env.atkvol)  << "-"
            << setw(3)
            << static_cast<short>(split.env.attack)  << "-"
            << setw(3)
            << static_cast<short>(split.env.decay)   << "-"
            << setw(3)
            << static_cast<short>(split.env.sustain) << "-"
            << setw(3)
            << static_cast<short>(split.env.hold)    << "-"
            << setw(4)
            << static_cast<short>(split.env.decay2)  << "-"
            << setw(4)
            << static_cast<short>(split.env.release)
            << "\n"
            ;
        ++cntsplits;
    }

    return strm;
}

std::ostream& operator<<(std::ostream& strm, const DSE::WavInfo& other)
{
    strm << "WavI: SmplID:" << other.id 
         << ", Tune:(f: " << (int)other.ftune 
         << ",c:" << (int)other.ctune
         << "), Root:" << (int)other.rootkey 
         << ", KTps.:" << (int)other.ktps 
         << ", Vol:" << (int)other.vol
         << ", Pan:" << (int)other.pan 
         << ", Fmt:\"" << DSE::DseSmplFmtToString(other.smplfmt)
         << "\", Loop:" << other.smplloop 
         << ", SmplRate:" << (double)(other.smplrate / 1000.0)
         << " kHz, Loop(beg:" << other.loopbeg 
         << ", len:" << other.looplen 
         << "), Env:" << (other.envon ? "On"s : "Off"s)
         << "(atklvl:" << other.env.atkvol 
         << ", atk:" << other.env.attack
         << ", dec:" << other.env.decay
         << ", sus:" << other.env.sustain
         << ",hold:" << other.env.hold
         << ",dec2:" << other.env.decay2
         << ",rel:" << other.env.release
         << ")\n"
        ;
    return strm;
}

//SMDL
std::ostream& operator<<(std::ostream& strm, const DSE::TrkEvent& ev)
{
    using namespace DSE;
    auto info = GetEventInfo(static_cast<eTrkEventCodes>(ev.evcode));

    if (info)
    {
        strm << "(0x" << setfill('0') << setw(2) << hex << uppercase << right << static_cast<unsigned short>(ev.evcode) << ") : "
            << nouppercase << dec << setfill(' ') << setw(16) << left << info->evlbl;

        if (!ev.params.empty())
        {
            strm << hex << uppercase << "( ";
            for (size_t i = 0; i < ev.params.size(); ++i)
            {
                strm << "0x" << setfill('0') << setw(2) << right << static_cast<unsigned short>(ev.params[i]);
                if (i != (ev.params.size() - 1))
                    strm << ", ";
            }
            strm << dec << nouppercase << " )";
        }
        else if (ev.evcode >= static_cast<uint8_t>(eTrkEventCodes::Delay_HN) && ev.evcode <= static_cast<uint8_t>(eTrkEventCodes::Delay_64N))
            strm << dec << static_cast<unsigned short>(TrkDelayCodeVals.at(ev.evcode)) << " ticks";
        else
            strm << "N/A";
    }
    else
        strm << "ERROR EVENT CODE " << uppercase << hex << static_cast<unsigned short>(ev.evcode) << dec << nouppercase;
    return strm;
}

std::ostream& operator<<(std::ostream& strm, const DSE::eDSEContainers cnty)
{
    uint32_t convertedty = static_cast<uint32_t>(cnty);
    strm << static_cast<int8_t>(convertedty >> 24) << static_cast<int8_t>(convertedty >> 16) << static_cast<int8_t>(convertedty >> 8) << static_cast<int8_t>(convertedty);
    return strm;
}