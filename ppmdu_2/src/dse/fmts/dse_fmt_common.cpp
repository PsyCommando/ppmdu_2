#include <dse/fmts/dse_fmt_common.hpp>

#include <iomanip>
#include <iostream>

using namespace std;

std::ostream& operator<<(std::ostream& os, const DSE::SeqInfoChunk_v415& obj)
{
    os << "\t-- Sequence Info Chunk v0x415 --\n"
        << showbase
        << hex << uppercase
        << "\tUnk30        : " << obj.unk30 << "\n"
        << "\toffnext      : " << obj.offnext << "\n"
        << "\tUnk16        : " << obj.unk16 << "\n"
        << dec << nouppercase
        << "\tNbTracks     : " << static_cast<short>(obj.nbtrks) << "\n"
        << "\tNbChans      : " << static_cast<short>(obj.nbchans) << "\n"
        << hex << uppercase
        << "\tUnk19        : " << obj.unk19 << "\n"
        << "\tUnk20        : " << obj.unk20 << "\n"
        << "\tUnk21        : " << obj.unk21 << "\n"
        << "\tUnk22        : " << obj.unk22 << "\n"
        << "\tUnk23        : " << obj.unk23 << "\n"
        << "\tUnk24        : " << obj.unk24 << "\n"
        << "\tUnk25        : " << obj.unk25 << "\n"
        << "\tUnk26        : " << obj.unk26 << "\n"
        << "\tUnk27        : " << obj.unk27 << "\n"
        << "\tUnk28        : " << obj.unk28 << "\n"
        << "\tUnk29        : " << obj.unk29 << "\n"
        << "\tUnk31        : " << obj.unk31 << "\n"
        << "\tUnk12        : " << obj.unk12 << "\n"
        << dec << nouppercase
        << noshowbase
        << "\n"
        ;
    return os;
}

std::ostream& operator<<(std::ostream& os, const DSE::SeqInfoChunk_v402& obj)
{
    os << "\t-- Sequence Info Chunk v0x415 --\n"
        << showbase
        << hex << uppercase
        << "\tUnk30        : " << obj.unk30 << "\n"
        << "\toffnext      : " << obj.offnext << "\n"
        << dec << nouppercase
        << "\tNbTracks     : " << static_cast<short>(obj.nbtrks) << "\n"
        << "\tNbChans      : " << static_cast<short>(obj.nbchans) << "\n"
        << hex << uppercase
        << "\tUnk5         : " << obj.unk5 << "\n"
        << "\tUnk6         : " << obj.unk6 << "\n"
        << "\tUnk7         : " << obj.unk7 << "\n"
        << "\tUnk8         : " << obj.unk8 << "\n"
        << dec << nouppercase
        << "\tMainVol      : " << static_cast<short>(obj.mainvol) << "\n"
        << "\tMainPan      : " << static_cast<short>(obj.mainpan) << "\n"
        << noshowbase
        << "\n"
        ;
    return os;
}