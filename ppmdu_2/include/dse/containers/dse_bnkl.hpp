#ifndef DSE_BNKL_HPP
#define DSE_BNKL_HPP
#include <dse/dse_common.hpp>

namespace DSE
{
    /*****************************************************************************************
    *****************************************************************************************/
    struct BnklChunkContents
    {
        struct bnklentry
        {
            uint16_t unk17 = 0;
            std::vector<uint16_t> bankids;

            template<class _init> _init Read(_init itbeg, _init itend)
            {
                bankids.resize(0);
                _init itentrybeg = itbeg;
                uint16_t nbbankids = utils::ReadIntFromBytes<uint16_t>(itbeg, itend);
                unk17 = utils::ReadIntFromBytes<uint16_t>(itbeg, itend);

                for (int cntbank = 0; cntbank < nbbankids; ++cntbank)
                    bankids.push_back(utils::ReadIntFromBytes<uint16_t>(itbeg, itend));
                return itbeg;
            }

            template<class _outit> _outit Write(_outit itout)
            {
                itout = utils::WriteIntToBytes(static_cast<uint16_t>(bankids.size()), itout);
                itout = utils::WriteIntToBytes(unk17, itout);
                for (uint16_t bankid : bankids)
                    itout = utils::WriteIntToBytes(bankid, itout);
                return itout;
            }
        };

        void AddEntry(bnklentry&& entry)
        {
            bnkldata.push_back(entry);
        }

        void AddEntry(bnklentry entry)
        {
            bnkldata.push_back(std::move(entry));
        }

        inline bool empty()const { return bnkldata.empty(); }

        std::vector<bnklentry> bnkldata;
    };
};

#endif //!DSE_BNKL_HPP