#ifndef DSE_PRESET_BANK_HPP
#define DSE_PRESET_BANK_HPP
#include <dse/dse_common.hpp>
#include <dse/containers/dse_program_bank.hpp>
#include <dse/containers/dse_sample_bank.hpp>

#include <memory>

namespace DSE
{
    /*****************************************************************************************
        PresetBank
            Is the combination of a SampleBank, and an ProgramBank.
            Or just an instrument bank if samples are not available
    *****************************************************************************************/
    class PresetBank
    {
    public:

        typedef std::shared_ptr<ProgramBank>   ptrprg_t;
        typedef std::weak_ptr<ProgramBank>     wptrprg_t;

        typedef std::shared_ptr<SampleBank>       ptrsmpl_t;
        typedef std::weak_ptr<SampleBank>         wptrsmpl_t;

        PresetBank()noexcept
        {}

        PresetBank(DSE::DSE_MetaDataSWDL&& meta, std::unique_ptr<ProgramBank>&& pInstrument, std::unique_ptr<SampleBank>&& pSmpl)noexcept
            :m_pPrgbnk(std::move(pInstrument)), m_pSamples(std::move(pSmpl)), m_meta(std::move(meta))
        {}

        PresetBank(DSE::DSE_MetaDataSWDL&& meta, std::unique_ptr<ProgramBank>&& pInstrument)noexcept
            :m_pPrgbnk(std::move(pInstrument)), m_pSamples(nullptr), m_meta(std::move(meta))
        {}

        PresetBank(DSE::DSE_MetaDataSWDL&& meta, std::unique_ptr<SampleBank>&& pSmpl)noexcept
            :m_pPrgbnk(nullptr), m_pSamples(std::move(pSmpl)), m_meta(std::move(meta))
        {}

        PresetBank(PresetBank&& mv)noexcept
        {
            operator=(std::forward<PresetBank>(mv));
        }

        PresetBank& operator=(PresetBank&& mv)noexcept
        {
            m_pPrgbnk = std::move(mv.m_pPrgbnk);
            m_pSamples = std::move(mv.m_pSamples);
            m_meta = std::move(mv.m_meta);
            return *this;
        }

        PresetBank merge(const PresetBank& other)const
        {
            PresetBank pbank;
            pbank.metadata(m_meta);

            //Merge, copy, or not the program bank
            if (m_pPrgbnk && other.m_pPrgbnk)
                pbank.prgmbank(std::unique_ptr<ProgramBank>(new ProgramBank(m_pPrgbnk->merge(*other.m_pPrgbnk))));
            else if (m_pPrgbnk)
                pbank.prgmbank(std::unique_ptr<ProgramBank>(new ProgramBank(*m_pPrgbnk)));
            else if (other.m_pPrgbnk)
                pbank.prgmbank(std::unique_ptr<ProgramBank>(new ProgramBank(*other.m_pPrgbnk)));

            //Merge, copy or not the samples
            if (m_pSamples && other.m_pSamples)
                pbank.smplbank(std::unique_ptr<SampleBank>(new SampleBank(m_pSamples->merge(*other.m_pSamples))));
            else if (m_pSamples)
                pbank.smplbank(std::unique_ptr<SampleBank>(new SampleBank(*m_pSamples)));
            else if (other.m_pSamples)
                pbank.smplbank(std::unique_ptr<SampleBank>(new SampleBank(*other.m_pSamples)));

            return pbank;
        }


        DSE::DSE_MetaDataSWDL& metadata() { return m_meta; }
        const DSE::DSE_MetaDataSWDL& metadata()const { return m_meta; }
        void                      metadata(const DSE::DSE_MetaDataSWDL& data) { m_meta = data; }
        void                      metadata(DSE::DSE_MetaDataSWDL&& data) { m_meta = data; }

        //Returns a weak_ptr to the samplebank
        wptrsmpl_t                smplbank() { return m_pSamples; }
        const wptrsmpl_t          smplbank()const { return m_pSamples; }
        void                      smplbank(ptrsmpl_t&& samplesbank) { m_pSamples = std::move(samplesbank); }
        //void                      smplbank( SampleBank  * samplesbank)                       { m_pSamples = samplesbank; }
        void                      smplbank(std::unique_ptr<SampleBank>&& samplesbank) { m_pSamples = std::move(samplesbank); }

        //Returns a weak_ptr to the program bank
        wptrprg_t                prgmbank() { return m_pPrgbnk; }
        const wptrprg_t          prgmbank()const { return m_pPrgbnk; }
        void                      prgmbank(ptrprg_t&& bank) { m_pPrgbnk = std::move(bank); }
        //void                      prgmbank( ProgramBank * bank)                           { m_pPrgbnk.reset(bank); }
        void                      prgmbank(std::unique_ptr<ProgramBank>&& bank) { m_pPrgbnk = std::move(bank); }

    private:
        //Can't copy
        PresetBank(const PresetBank&) = delete;
        PresetBank& operator=(const PresetBank&) = delete;

        DSE::DSE_MetaDataSWDL m_meta;
        ptrprg_t              m_pPrgbnk;      //A program bank may not be shared by many
        ptrsmpl_t             m_pSamples;     //A sample bank may be shared by many
        bool                  m_bWasModified; //Whether this file was modified after being loaded, or was imported by a user
    };
};

#endif // !DSE_PRESET_BANK_HPP
