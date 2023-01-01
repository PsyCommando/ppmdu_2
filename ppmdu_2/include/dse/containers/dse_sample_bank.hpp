#ifndef DSE_SAMPLE_BANK_HPP
#define DSE_SAMPLE_BANK_HPP
#include <dse/dse_common.hpp>

namespace DSE
{
    /*****************************************************************************************
        SampleBank
            This class is used to maintain references to sample data.
            The samples in this are refered to by entries in a SampleMap instance.
    *****************************************************************************************/
    class SampleBank
    {
    public:
        typedef std::unique_ptr<std::vector<uint8_t>> smpl_t;            //Pointer to a vector of raw sample data
        typedef std::unique_ptr<DSE::WavInfo>         wavinfoptr_t;      //Pointer to wavinfo

        struct SampleBlock
        {
            wavinfoptr_t pinfo_;
            smpl_t       pdata_;

            inline bool isnull()const { return (pinfo_ == nullptr) && (pdata_ == nullptr); }

            SampleBlock()noexcept {}
            SampleBlock(smpl_t&& smpl, wavinfoptr_t&& wavi)noexcept
                :pinfo_(std::forward<wavinfoptr_t>(wavi)), pdata_(std::forward<smpl_t>(smpl))
            {}

            SampleBlock(const SampleBlock& other)noexcept
            {
                operator=(other);
            }

            SampleBlock& operator=(const SampleBlock& other)noexcept
            {
                if (other.pdata_)
                    pdata_ = smpl_t(new std::vector<uint8_t>(*other.pdata_));
                if (other.pinfo_)
                    pinfo_ = wavinfoptr_t(new DSE::WavInfo(*other.pinfo_));
                return *this;
            }

            SampleBlock(SampleBlock&& other)noexcept
            {
                pinfo_.reset(other.pinfo_.release());
                pdata_.reset(other.pdata_.release());
            }

            SampleBlock& operator=(SampleBlock&& other)noexcept
            {
                pinfo_.reset(other.pinfo_.release());
                pdata_.reset(other.pdata_.release());
                return *this;
            }

            SampleBlock merge(const SampleBlock& other)const
            {
                smpl_t newsmpl;
                wavinfoptr_t newwavi;
                if (!other.isnull())
                {
                    if (other.pdata_)
                        newsmpl = smpl_t(new std::vector<uint8_t>(*(other.pdata_)));
                    if (other.pinfo_)
                        newwavi = wavinfoptr_t(new DSE::WavInfo(*(other.pinfo_)));
                }
                if (!isnull())
                {
                    if (!newsmpl && pdata_)
                        newsmpl = smpl_t(new std::vector<uint8_t>(*pdata_));
                    if (!newwavi && pinfo_)
                        newwavi = wavinfoptr_t(new DSE::WavInfo(*pinfo_));
                }
                return SampleBlock(std::move(newsmpl), std::move(newwavi));
            }
        };
        typedef std::vector<SampleBlock>::iterator       iterator;
        typedef std::vector<SampleBlock>::const_iterator const_iterator;

        SampleBank()
        {}

        SampleBank(std::vector<SampleBlock>&& smpls)noexcept
            :m_SampleData(std::forward<std::vector<SampleBlock>>(smpls))
        {}

        SampleBank(SampleBank&& mv)noexcept
        {
            m_SampleData = std::move(mv.m_SampleData);
        }

        SampleBank& operator=(SampleBank&& mv)noexcept
        {
            m_SampleData = std::move(mv.m_SampleData);
            return *this;
        }

        SampleBank(const SampleBank& other)
        {
            DoCopyFrom(other);
        }

        const SampleBank& operator=(const SampleBank& other)
        {
            DoCopyFrom(other);
            return *this;
        }

        inline iterator       begin() { return m_SampleData.begin(); }
        inline const_iterator begin()const { return m_SampleData.begin(); }
        inline iterator       end() { return m_SampleData.end(); }
        inline const_iterator end()const { return m_SampleData.end(); }
        inline sampleid_t     size()const { return NbSlots(); }

        SampleBank merge(const SampleBank& other)const
        {
            SampleBank sbnk;
            const sampleid_t maxsmpls = std::max(m_SampleData.size(), other.m_SampleData.size());
            sbnk.m_SampleData.resize(maxsmpls);

            for (sampleid_t cntsmpls = 0; cntsmpls < maxsmpls; ++cntsmpls)
            {
                SampleBlock newblk;

                if (cntsmpls < other.m_SampleData.size() && cntsmpls < m_SampleData.size())
                    newblk = m_SampleData[cntsmpls].merge(other.m_SampleData[cntsmpls]);
                else if (cntsmpls < other.m_SampleData.size() && !(other.m_SampleData[cntsmpls].isnull()))
                {
                    newblk = other.m_SampleData[cntsmpls];
                }
                else if (cntsmpls < m_SampleData.size() && !(m_SampleData[cntsmpls].isnull()))
                {
                    newblk = m_SampleData[cntsmpls];
                }
                sbnk.m_SampleData[cntsmpls] = std::move(newblk);
            }
            return sbnk;
        }

    public:
        //Info

        //Nb of sample slots with or without data
        inline sampleid_t                   NbSlots()const { return m_SampleData.size(); }

        //Access
        bool                                IsInfoPresent(sampleid_t index)const { return sampleInfo(index) != nullptr; }
        bool                                IsDataPresent(sampleid_t index)const { return sample(index) != nullptr; }

        inline DSE::SampleBank::SampleBlock* sampleBlock(sampleid_t index)
        {
            if (m_SampleData.size() > index)
                return &(m_SampleData[index]);
            else
                return nullptr;
        }

        inline const DSE::SampleBank::SampleBlock* sampleBlock(sampleid_t index)const
        {
            if (m_SampleData.size() > index)
                return &(m_SampleData[index]);
            else
                return nullptr;
        }


        inline DSE::WavInfo* sampleInfo(sampleid_t index)
        {
            if (m_SampleData.size() > index)
                return m_SampleData[index].pinfo_.get();
            else
                return nullptr;
        }

        inline const DSE::WavInfo* sampleInfo(sampleid_t index)const
        {
            if (m_SampleData.size() > index)
                return m_SampleData[index].pinfo_.get();
            else
                return nullptr;
        }


        inline std::vector<uint8_t>* sample(sampleid_t index)
        {
            if (m_SampleData.size() > index)
                return m_SampleData[index].pdata_.get();
            else
                return nullptr;
        }

        inline const std::vector<uint8_t>* sample(sampleid_t index)const
        {
            if (m_SampleData.size() > index)
                return m_SampleData[index].pdata_.get();
            else
                return nullptr;
        }

        void setSampleData(sampleid_t index, std::vector<uint8_t>&& data)
        {
            if (m_SampleData.size() <= index)
                m_SampleData.resize(index + 1); //Make sure we're big enough

            if (m_SampleData.size() > index)
                m_SampleData[index].pdata_.reset(new std::vector<uint8_t>(data));
        }

        inline std::vector<uint8_t>* operator[](sampleid_t index) { return sample(index); }
        inline const std::vector<uint8_t>* operator[](sampleid_t index)const { return sample(index); }

    private:

        //Copy the content pointed by the pointers, and not just the pointers themselves !
        void DoCopyFrom(const SampleBank& other)
        {
            m_SampleData.resize(other.m_SampleData.size());

            for (sampleid_t i = 0; i < other.m_SampleData.size(); ++i)
            {
                if (other.m_SampleData[i].pdata_ != nullptr)
                    m_SampleData[i].pdata_.reset(new std::vector<uint8_t>(*(other.m_SampleData[i].pdata_))); //Copy each objects and make a pointer

                if (other.m_SampleData[i].pinfo_ != nullptr)
                    m_SampleData[i].pinfo_.reset(new DSE::WavInfo(*(other.m_SampleData[i].pinfo_))); //Copy each objects and make a pointer
            }
        }

    private:
        std::vector<SampleBlock> m_SampleData;
    };
};

#endif // !DSE_SAMPLE_BANK_HPP
