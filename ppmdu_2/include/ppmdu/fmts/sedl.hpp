#ifndef SEDL_HPP
#define SEDL_HPP
/*
sedl.hpp
2015/05/20
psycommando@gmail.com
Description: Utilities for handling Pokemon Mystery Dungeon: Explorers of Sky/Time/Darkness's .sed files.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <dse/dse_common.hpp>
#include <dse/dse_containers.hpp>

#ifdef USE_PPMDU_CONTENT_TYPE_ANALYSER
    #include <types/content_type_analyser.hpp>
    namespace filetypes
    {
        extern const ContentTy CnTy_SEDL; //Content ID db handle
    };
#endif

namespace DSE
{

    static const uint32_t SEDL_MagicNumber = static_cast<uint32_t>(eDSEContainers::sedl);//0x7365646C; //"sedl"


    /****************************************************************************************
        SEDL_Header
            The header of the SEDL file.
    ****************************************************************************************/
    struct SEDL_Header
    {
        static const uint32_t Size     = 56; //without padding
        static const uint32_t FNameLen = 16;

        static unsigned int size() { return Size; }

        uint32_t magicn          = 0;
        uint32_t spacer          = 0; //Always 0
        uint32_t flen            = 0;
        uint16_t version         = 0;
        uint8_t  bankid_low      = 0; //Low byte of the bank id used in this sedl.
        uint8_t  bankid_high     = 0; //High byte of the bank id used in this sedl.
        uint32_t unk3            = 0;
        uint32_t unk4            = 0;
        
        //Creation time
        uint16_t year            = 0;
        uint8_t  month           = 0;
        uint8_t  day             = 0;
        uint8_t  hour            = 0;
        uint8_t  minute          = 0;
        uint8_t  second          = 0;
        uint8_t  centisec        = 0;

        //Filename
        std::array<char,FNameLen> fname;

        uint16_t unk5            = 0;
        uint16_t unk6            = 0;
        uint16_t unk7            = 0;
        uint16_t unk8            = 0;


        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes   ( SEDL_MagicNumber, itwriteto, false ); //Write constant magic number, to avoid bad surprises
            itwriteto = utils::WriteIntToBytes   ( spacer,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( flen,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( version,          itwriteto );
            itwriteto = utils::WriteIntToBytes   ( bankid_low,       itwriteto );
            itwriteto = utils::WriteIntToBytes   ( bankid_high,      itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk3,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk4,             itwriteto );

            itwriteto = utils::WriteIntToBytes   ( year,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( month,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( day,              itwriteto );
            itwriteto = utils::WriteIntToBytes   ( hour,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( minute,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( second,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( centisec,         itwriteto );

            itwriteto = utils::WriteStrToByteContainer( itwriteto,        fname, fname.size() );

            itwriteto = utils::WriteIntToBytes   ( unk5,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk6,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk7,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk8,             itwriteto );

            return itwriteto;
        }


        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itPastEnd )
        {
            magicn      = utils::ReadIntFromBytes<decltype(magicn)>     ( itReadfrom, itPastEnd, false ); //iterator is incremented
            spacer      = utils::ReadIntFromBytes<decltype(spacer)>     ( itReadfrom, itPastEnd );
            flen        = utils::ReadIntFromBytes<decltype(flen)>       ( itReadfrom, itPastEnd );
            version     = utils::ReadIntFromBytes<decltype(version)>    ( itReadfrom, itPastEnd );
            bankid_low  = utils::ReadIntFromBytes<decltype(bankid_low)> ( itReadfrom, itPastEnd );
            bankid_high = utils::ReadIntFromBytes<decltype(bankid_high)>( itReadfrom, itPastEnd );
            unk3        = utils::ReadIntFromBytes<decltype(unk3)>       ( itReadfrom, itPastEnd );
            unk4        = utils::ReadIntFromBytes<decltype(unk4)>       ( itReadfrom, itPastEnd );

            year        = utils::ReadIntFromBytes<decltype(year)>       ( itReadfrom, itPastEnd );
            month       = utils::ReadIntFromBytes<decltype(month)>      ( itReadfrom, itPastEnd );
            day         = utils::ReadIntFromBytes<decltype(day)>        ( itReadfrom, itPastEnd );
            hour        = utils::ReadIntFromBytes<decltype(hour)>       ( itReadfrom, itPastEnd );
            minute      = utils::ReadIntFromBytes<decltype(minute)>     ( itReadfrom, itPastEnd );
            second      = utils::ReadIntFromBytes<decltype(second)>     ( itReadfrom, itPastEnd );
            centisec    = utils::ReadIntFromBytes<decltype(centisec)>   ( itReadfrom, itPastEnd );

            itReadfrom  = utils::ReadStrFromByteContainer( itReadfrom, fname.data(), FNameLen );

            unk5        = utils::ReadIntFromBytes<decltype(unk5)>       ( itReadfrom, itPastEnd );
            unk6        = utils::ReadIntFromBytes<decltype(unk6)>       ( itReadfrom, itPastEnd );
            unk7        = utils::ReadIntFromBytes<decltype(unk7)>       ( itReadfrom, itPastEnd );
            unk8        = utils::ReadIntFromBytes<decltype(unk8)>       ( itReadfrom, itPastEnd );

            return itReadfrom;
        }
    };

    /// <summary>
    /// Seq chunk header and offset table. 
    /// Some games, like shiren completely omit this header. 
    /// The offset list is instead tacked at the end of the sedl header, and also are made relative to the start of the sedl header.
    /// </summary>
    struct seq_chunk_header
    {
        static const uint32_t OffsetTableBeg = 16; //The offset table begins 16 bytes after the start of the seq_chunk_header header

        uint32_t magicn    = 0; //For parsing only
        uint16_t spacer    = 0; //2bytes of 0
        uint16_t version   = 0;
        uint16_t seqtbloff = 0; //Offset of the seqtable/length of the header.
        uint32_t chunklen  = 0;

        //seq table
        std::vector<uint16_t> seqoffsets; //List of sequence table offsets. Always begins with 0x0000

        template<class _outit>
        _outit WriteToContainer(_outit itwriteto)const
        {
            itwriteto = utils::WriteIntToBytes(static_cast<uint32_t>(eDSEChunks::seq), itwriteto, false); //Write constant magic number, to avoid bad surprises
            itwriteto = utils::WriteIntToBytes(spacer,    itwriteto);
            itwriteto = utils::WriteIntToBytes(version,   itwriteto);
            itwriteto = utils::WriteIntToBytes(seqtbloff, itwriteto);
            itwriteto = utils::WriteIntToBytes(chunklen,  itwriteto);

            utils::WriteIntToBytes(static_cast<uint16_t>(0), itwriteto); //Starting zero
            for (uint16_t offs : seqoffsets) //We assume we got valid offsets at this point
                itwriteto = utils::WriteIntToBytes(offs, itwriteto);

            //Apply padding to our offset table
            utils::AppendPaddingBytes(itwriteto, 16 + (seqoffsets.size() * 2), 16); //Padding for the table is zeros
            return itwriteto;
        }

        template<class _init>
        _init ReadFromContainer(_init itReadfrom, _init itPastEnd)
        {
            magicn    = utils::ReadIntFromBytes<decltype(magicn)>   (itReadfrom, itPastEnd, false); //iterator is incremented
            spacer    = utils::ReadIntFromBytes<decltype(spacer)>   (itReadfrom, itPastEnd);
            version   = utils::ReadIntFromBytes<decltype(version)>  (itReadfrom, itPastEnd);
            seqtbloff = utils::ReadIntFromBytes<decltype(seqtbloff)>(itReadfrom, itPastEnd);
            chunklen  = utils::ReadIntFromBytes<decltype(chunklen)> (itReadfrom, itPastEnd);

            _init ittblbeg = itReadfrom;
            seqoffsets.resize(0);

            //First entry is always 0
            uint16_t curptr = utils::ReadIntFromBytes<uint16_t>(itReadfrom, itPastEnd);
            if (curptr != 0)
                throw std::runtime_error("First entry of the sequence offsets table is not 0!");
            //Ignore the first 0

            //Get the first non-zero entry to tell the end of the table
            uint16_t tblendoffs = utils::ReadIntFromBytes<uint16_t>(itReadfrom, itPastEnd);

            //Just ignore the table if we hit padding
            if (tblendoffs != 0)
            {
                _init ittblend = std::next(ittblbeg, tblendoffs);
                seqoffsets.push_back(tblendoffs); //Add the ptr we just read

                while (itReadfrom != ittblend) //Kepp reading until we hit the end of the table
                {
                    curptr = utils::ReadIntFromBytes<uint16_t>(itReadfrom, ittblend);
                    if (curptr == 0)
                        break; //Break out of the loop if we hit a 0, since that's the padding used for the table
                    seqoffsets.push_back(curptr);
                }
            }
            return itReadfrom;
        }
    };

    /// <summary>
    /// 
    /// </summary>
    struct mcrl_chunk_header
    {
        uint32_t label    = 0;
        uint16_t spacer   = 0;
        uint16_t version  = 0;
        uint32_t hdrlen   = 0;
        uint32_t chunklen = 0;

        std::vector<uint16_t> ptrtbl; //Offsets to entries

        template<class _outit>
        _outit WriteToContainer(_outit itwriteto)const
        {
            itwriteto = utils::WriteIntToBytes(static_cast<uint32_t>(eDSEChunks::mcrl), itwriteto, false);
            itwriteto = utils::WriteIntToBytes(spacer,   itwriteto);
            itwriteto = utils::WriteIntToBytes(version,  itwriteto);
            itwriteto = utils::WriteIntToBytes(hdrlen,   itwriteto);
            itwriteto = utils::WriteIntToBytes(chunklen, itwriteto);

            for (uint16_t offs : ptrtbl)
            {
                itwriteto = utils::WriteIntToBytes(offs, itwriteto);
            }
            utils::AppendPaddingBytes(itwriteto, 16 + (ptrtbl.size() * 2), 16);
            return itwriteto;
        }

        template<class _init>
        _init ReadFromContainer(_init itReadfrom, _init itPastEnd)
        {
            label    = utils::ReadIntFromBytes<decltype(label)>   (itReadfrom, itPastEnd, false); //iterator is incremented
            spacer   = utils::ReadIntFromBytes<decltype(spacer)>  (itReadfrom, itPastEnd);
            version  = utils::ReadIntFromBytes<decltype(version)> (itReadfrom, itPastEnd);
            hdrlen   = utils::ReadIntFromBytes<decltype(hdrlen)>  (itReadfrom, itPastEnd);
            chunklen = utils::ReadIntFromBytes<decltype(chunklen)>(itReadfrom, itPastEnd);

            ptrtbl.resize(0);
            _init itbeftable = itReadfrom;
            uint16_t curptr = utils::ReadIntFromBytes<uint16_t>(itReadfrom, itPastEnd);
            _init itendtable = std::next(itbeftable, curptr);
            ptrtbl.push_back(curptr);

            while (itReadfrom != itendtable)
            {
                curptr = utils::ReadIntFromBytes<uint16_t>(itReadfrom, itendtable);
                if (curptr == 0)
                    break; //Break on padding zeros
                ptrtbl.push_back(curptr);
            }
            return itReadfrom;
        }
    };

    /// <summary>
    /// 
    /// </summary>
    struct bnkl_chunk_header
    {
        uint32_t label    = 0;
        uint16_t spacer   = 0;
        uint16_t version  = 0;
        uint32_t hdrlen   = 0;
        uint32_t chunklen = 0;

        std::vector<uint16_t> ptrtbl; //Offsets to entries

        template<class _outit>
        _outit WriteToContainer(_outit itwriteto)const
        {
            itwriteto = utils::WriteIntToBytes(static_cast<uint32_t>(eDSEChunks::bnkl), itwriteto, false);
            itwriteto = utils::WriteIntToBytes(spacer,   itwriteto);
            itwriteto = utils::WriteIntToBytes(version,  itwriteto);
            itwriteto = utils::WriteIntToBytes(hdrlen,   itwriteto);
            itwriteto = utils::WriteIntToBytes(chunklen, itwriteto);

            for (uint16_t offs : ptrtbl)
            {
                itwriteto = utils::WriteIntToBytes(offs, itwriteto);
            }
            utils::AppendPaddingBytes(itwriteto, 16 + (ptrtbl.size() * 2), 16);
            return itwriteto;
        }

        template<class _init>
        _init ReadFromContainer(_init itReadfrom, _init itPastEnd)
        {
            label    = utils::ReadIntFromBytes<decltype(label)>(itReadfrom, itPastEnd, false); //iterator is incremented
            spacer   = utils::ReadIntFromBytes<decltype(spacer)>(itReadfrom, itPastEnd);
            version  = utils::ReadIntFromBytes<decltype(version)>(itReadfrom, itPastEnd);
            hdrlen   = utils::ReadIntFromBytes<decltype(hdrlen)>(itReadfrom, itPastEnd);
            chunklen = utils::ReadIntFromBytes<decltype(chunklen)>(itReadfrom, itPastEnd);

            ptrtbl.resize(0);
            _init itbeftable = itReadfrom;
            uint16_t curptr = utils::ReadIntFromBytes<uint16_t>(itReadfrom, itPastEnd);
            _init itendtable = std::next(itbeftable, curptr);
            ptrtbl.push_back(curptr);

            while (itReadfrom != itendtable)
            {
                curptr = utils::ReadIntFromBytes<uint16_t>(itReadfrom, itendtable);
                if (curptr == 0)
                    break; //Break on padding zeros
                ptrtbl.push_back(curptr);
            }
            return itReadfrom;
        }
    };


    //
    DSE::SoundEffectSequences ParseSEDL(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend);
    void WriteSEDL(const std::string& path, const DSE::SoundEffectSequences & seq);
};

#endif