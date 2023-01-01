#ifndef DSE_MCTRL_HPP
#define DSE_MCTRL_HPP
#include <dse/dse_common.hpp>

namespace DSE
{
    /*****************************************************************************************
    *****************************************************************************************/
    struct McrlChunkContents
    {
        struct mcrlentry
        {
            uint16_t    unk12 = 0;
            std::string unkstr;

            template<class _init> _init Read(_init itbeg, _init itend)
            {
                _init itentrybeg = itbeg;
                unk12 = utils::ReadIntFromBytes<uint16_t>(itbeg, itend);
                if (unk12 == 0xFFFF)
                {
                    //If the entry is 0xFFFF there's nothing to parse
                    return itbeg;
                }

                uint16_t entrylen = utils::ReadIntFromBytes<uint16_t>(itbeg, itend);
                _init itendstr = std::next(itentrybeg, entrylen);
                unkstr = std::string(itbeg, itendstr);
                return itbeg;
            }

            template<class _outit> _outit Write(_outit itout)
            {
                itout = utils::WriteIntToBytes(unk12, itout);
                if (unk12 == 0xFFFF)
                    return itout; //Nothing else to do in this case

                uint16_t totallen = (sizeof(uint16_t) * 2) + unkstr.length();
                totallen += (totallen % 2); //Add to the count a padding byte if neccessary to align us on 2 bytes
                itout = utils::WriteIntToBytes(totallen, itout); //Put the computed entry length

                //Next put the string
                itout = utils::WriteStrToByteContainer(itout, unkstr);

                //And the padding
                utils::AppendPaddingBytes(itout, unkstr.length(), 2); //Since the string is the only thing that changes in size, use it to calc padding
                return itout;
            }
        };

        void AddEntry(mcrlentry&& entry)
        {
            mcrldata.push_back(entry);
        }

        void AddEntry(mcrlentry entry)
        {
            mcrldata.push_back(std::move(entry));
        }

        inline bool empty()const { return mcrldata.empty(); }

        std::vector<mcrlentry> mcrldata;
    };
};

#endif //!DSE_MCTRL_HPP