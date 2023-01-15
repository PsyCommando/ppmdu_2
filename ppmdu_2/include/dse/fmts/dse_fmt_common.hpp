#ifndef DSE_FORMATS_COMMON_HPP
#define DSE_FORMATS_COMMON_HPP
#include <dse/dse_common.hpp>

#include <utils/gbyteutils.hpp>

#include <algorithm>

namespace DSE
{

//===============================================================================
// Sequence Info Chunk
//===============================================================================

    /// <summary>
    /// Sequence info chunk used in DSE v415.
    /// This chunk appears in sedl and smdl before a set of 
    /// trk chunks and provides details on that set of tracks.
    /// </summary>
    struct SeqInfoChunk_v415
    {
        static constexpr size_t size() { return 48; }
        static const size_t   PaddingLen = 16;
        static const uint16_t DefUnk30   = 0x1;
        static const uint16_t DefOffNext = 0x30;
        static const uint16_t DefUnk16   = 0xFF01;
        static const uint8_t  DefUnk22   = 0xF;
        static const uint32_t DefUnk23   = 0xFFFFFFFF;
        static const uint32_t DefUnk25   = 0x4000;
        static const uint32_t DefUnk26   = 0x4000;
        static const uint32_t DefUnk27   = 0x0040;
        static const uint16_t DefUnk28   = 0x400;
        static const uint16_t DefUnk31   = 0x08;
        static const uint32_t DefUnk12   = 0xFFFFFF00;

        uint16_t unk30   = DefUnk30;
        uint16_t offnext = DefOffNext;
        uint16_t unk16   = DefUnk16;
        uint8_t  nbtrks  = 0;
        uint8_t  nbchans = 0;
        uint8_t  unk19   = 0;
        uint8_t  unk20   = 0;
        uint8_t  unk21   = 0;
        uint8_t  unk22   = DefUnk22;
        uint32_t unk23   = DefUnk23;
        uint16_t unk24   = 0;
        uint16_t unk25   = DefUnk25;
        uint16_t unk26   = DefUnk26;
        uint16_t unk27   = DefUnk27;
        uint16_t unk28   = DefUnk28;
        uint8_t  unk29   = 0;
        uint8_t  unk31   = DefUnk31;
        uint32_t unk12   = DefUnk12;
        //16 bytes of 0xFF padding here

        template<class _outit> _outit WriteToContainer(_outit itwriteto)const
        {
            itwriteto = utils::WriteIntToBytes(unk30,   itwriteto);
            itwriteto = utils::WriteIntToBytes(DefOffNext, itwriteto);
            itwriteto = utils::WriteIntToBytes(unk16,   itwriteto);
            itwriteto = utils::WriteIntToBytes(nbtrks,  itwriteto);
            itwriteto = utils::WriteIntToBytes(nbchans, itwriteto);
            itwriteto = utils::WriteIntToBytes(unk19,   itwriteto);
            itwriteto = utils::WriteIntToBytes(unk20,   itwriteto);
            itwriteto = utils::WriteIntToBytes(unk21,   itwriteto);
            itwriteto = utils::WriteIntToBytes(unk22,   itwriteto);
            itwriteto = utils::WriteIntToBytes(unk23,   itwriteto);
            itwriteto = utils::WriteIntToBytes(unk24,   itwriteto);
            itwriteto = utils::WriteIntToBytes(unk25,   itwriteto);
            itwriteto = utils::WriteIntToBytes(unk26,   itwriteto);
            itwriteto = utils::WriteIntToBytes(unk27,   itwriteto);
            itwriteto = utils::WriteIntToBytes(unk28,   itwriteto);
            itwriteto = utils::WriteIntToBytes(unk29,   itwriteto);
            itwriteto = utils::WriteIntToBytes(unk31,   itwriteto);
            itwriteto = utils::WriteIntToBytes(unk12,   itwriteto);
            itwriteto = std::fill_n(itwriteto, PaddingLen, 0xFF);
            return itwriteto;
        }

        template<class _init> _init ReadFromContainer(_init itReadfrom, _init itPastEnd)
        {
            itReadfrom = utils::ReadIntFromBytes(unk30,   itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(offnext, itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(unk16,   itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(nbtrks,  itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(nbchans, itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(unk19,   itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(unk20,   itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(unk21,   itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(unk22,   itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(unk23,   itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(unk24,   itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(unk25,   itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(unk26,   itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(unk27,   itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(unk28,   itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(unk29,   itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(unk31,   itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(unk12,   itReadfrom, itPastEnd);
            itReadfrom = std::next(itReadfrom, PaddingLen);
            return itReadfrom;
        }

        operator seqinfo_table()
        {
            seqinfo_table dest;
            dest.unk30   = unk30;
            dest.nextoff = offnext;
            dest.unk16   = unk16;
            dest.nbtrks  = nbtrks;
            dest.nbchans = nbchans;
            dest.unk19   = unk19;
            dest.unk20   = unk20;
            dest.unk21   = unk21;
            dest.unk22   = unk22;
            dest.unk23   = unk23;
            dest.unk24   = unk24;
            dest.unk25   = unk25;
            dest.unk26   = unk26;
            dest.unk27   = unk27;
            dest.unk28   = unk28;
            dest.unk29   = unk29;
            dest.unk31   = unk31;
            dest.unk12   = unk12;
            return dest;
        }

        SeqInfoChunk_v415& operator=(const seqinfo_table& tbl)
        {
            unk30   = tbl.unk30;
            offnext = tbl.nextoff;
            unk16   = tbl.unk16;
            nbtrks  = tbl.nbtrks;
            nbchans = tbl.nbchans;
            unk19   = tbl.unk19;
            unk20   = tbl.unk20;
            unk21   = tbl.unk21;
            unk22   = tbl.unk22;
            unk23   = tbl.unk23;
            unk24   = tbl.unk24;
            unk25   = tbl.unk25;
            unk26   = tbl.unk26;
            unk27   = tbl.unk27;
            unk28   = tbl.unk28;
            unk29   = tbl.unk29;
            unk31   = tbl.unk31;
            unk12   = tbl.unk12;
            return *this;
        }
    };

    /// <summary>
    /// Sequence info chunk used in DSE v402
    /// This chunk appears in sedl and smdl before a set of 
    /// trk chunks and provides details on that set of tracks.
    /// </summary>
    struct SeqInfoChunk_v402
    {
        static constexpr size_t size() { return 16; }
        static const uint16_t DefUnk30 = 0x1;
        static const uint16_t DefOffNext = 0x10;
        static const uint8_t  DefUnk5 = 0x1;
        static const uint8_t  DefUnk6 = 0x2;
        static const uint16_t DefUnk7 = 0x8;
        static const uint32_t DefUnk8 = 0x0F000000;

        uint16_t unk30   = DefUnk30;
        uint16_t offnext = DefOffNext;
        uint8_t  nbtrks  = 0;
        uint8_t  nbchans = 0;
        uint8_t  unk5    = DefUnk5;
        uint8_t  unk6    = DefUnk6;
        uint16_t unk7    = DefUnk7;
        int8_t   mainvol = DSE_DefaultVol;
        int8_t   mainpan = DSE_DefaultPan;
        uint32_t unk8    = DefUnk8;
        //No padding after

        template<class _outit> _outit WriteToContainer(_outit itwriteto)const
        {
            itwriteto = utils::WriteIntToBytes(unk30, itwriteto);
            itwriteto = utils::WriteIntToBytes(DefOffNext, itwriteto);
            itwriteto = utils::WriteIntToBytes(nbtrks, itwriteto);
            itwriteto = utils::WriteIntToBytes(nbchans, itwriteto);
            itwriteto = utils::WriteIntToBytes(unk5, itwriteto);
            itwriteto = utils::WriteIntToBytes(unk6, itwriteto);
            itwriteto = utils::WriteIntToBytes(unk7, itwriteto);
            itwriteto = utils::WriteIntToBytes(mainvol, itwriteto);
            itwriteto = utils::WriteIntToBytes(mainpan, itwriteto);
            itwriteto = utils::WriteIntToBytes(unk8, itwriteto);
            return itwriteto;
        }

        template<class _init> _init ReadFromContainer(_init itReadfrom, _init itPastEnd)
        {
            itReadfrom = utils::ReadIntFromBytes(unk30, itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(offnext, itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(nbtrks, itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(nbchans, itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(unk5, itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(unk6, itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(unk7, itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(mainvol, itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(mainpan, itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes(unk8, itReadfrom, itPastEnd);
            return itReadfrom;
        }

        operator seqinfo_table()
        {
            seqinfo_table dest{};
            dest.unk30   = unk30;
            dest.nextoff = offnext;
            dest.nbtrks  = nbtrks;
            dest.nbchans = nbchans;
            dest.unk5    = unk5;
            dest.unk6    = unk6;
            dest.unk7    = unk7;
            dest.unk8    = unk8;
            return dest;
        }

        SeqInfoChunk_v402& operator=(const seqinfo_table& tbl)
        {
            unk30   = tbl.unk30;
            offnext = tbl.nextoff;
            nbtrks  = tbl.nbtrks;
            nbchans = tbl.nbchans;
            unk5    = tbl.unk5;
            unk6    = tbl.unk6;
            unk7    = tbl.unk7;
            unk8    = tbl.unk8;
            return *this;
        }
    };

//
//
//
    template<typename _fwdit> void StringToDseFilename(const std::string& src, _fwdit itbeg, _fwdit itend, char padbyte = 0)
    {
        size_t tgtlen    = (size_t)std::distance(itbeg, itend);
        size_t stringend = std::min(src.size(), tgtlen - 1);
        std::fill(itbeg, itend, padbyte);
        
        std::copy(src.c_str(), src.c_str() + stringend, itbeg);

        //Make sure there's a ending 0
        if (stringend < tgtlen)
            *(std::next(itbeg, stringend)) = 0;
        else
            *(std::next(itbeg, tgtlen - 1)) = 0;
    }

};

std::ostream& operator<<(std::ostream& os, const DSE::SeqInfoChunk_v415& obj);
std::ostream& operator<<(std::ostream& os, const DSE::SeqInfoChunk_v402& obj);

#endif