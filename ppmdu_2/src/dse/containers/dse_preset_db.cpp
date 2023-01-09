#include <dse/containers/dse_preset_db.hpp>

namespace DSE
{
    ///////////////////////////////////////////////////////////////////////////////
    //PresetDatabase
    ///////////////////////////////////////////////////////////////////////////////
    PresetDatabase::PresetDatabase()
        :m_nbSampleBlocks(0)
    {}

    DSE::PresetBank& PresetDatabase::AddBank(DSE::PresetBank&& bnk)
    {
        const bankname_t& myid = bnk.metadata().get_original_file_name_no_ext();
        //#TODO: Handle merging banks if neccessary 
        if (m_banks.contains(myid))
        {
            assert(false);
        }

        //Keep some references to banks that contain samples, and banks that contain programs
        try
        {
            auto smplbnk = bnk.smplbank().lock();
            if (smplbnk)
            {
                m_nbSampleBlocks = std::max(m_nbSampleBlocks, smplbnk->NbSlots());
                for (sampleid_t cntsmpl = 0; cntsmpl < smplbnk->NbSlots(); ++cntsmpl)
                {
                    if (smplbnk->IsDataPresent(cntsmpl))
                    {

                        //Note that this bank had sample data for this sample
                        if (!m_sample2banks.contains(cntsmpl))
                            m_sample2banks.try_emplace(cntsmpl, std::set<bankname_t>());
                        m_sample2banks.at(cntsmpl).insert(myid);

                        //Add our info to the sample map
                        if (!m_sampleMap.contains(cntsmpl))
                            m_sampleMap.try_emplace(cntsmpl, std::map<bankname_t, SampleBank::SampleBlock>());
                        m_sampleMap.at(cntsmpl).try_emplace(myid, SampleBank::SampleBlock(*smplbnk->sampleBlock(cntsmpl)));
                    }
                    if (smplbnk->IsInfoPresent(cntsmpl))
                    {
                        if (!m_preset2banks.contains(cntsmpl))
                            m_preset2banks.try_emplace(cntsmpl, std::set<bankname_t>());
                        m_preset2banks.at(cntsmpl).insert(myid);
                    }
                }
            }
        }
        catch (const std::exception&)
        {
            std::throw_with_nested(std::runtime_error( "Error adding bank \"" + bnk.metadata().fname + "\"."));
        }

        auto result = m_banks.emplace(myid, std::forward<DSE::PresetBank>(bnk));
        return (result.first)->second;
    }

    PresetDatabase::sampledata_t* PresetDatabase::getSampleForBank(sampleid_t smplid, const bankname_t& bnkid)
    {
        SampleBank::SampleBlock* smplblk = getSampleBlockFor(smplid, bnkid);
        if (smplblk)
            return smplblk->pdata_.get();
        return nullptr;
    }

    DSE::WavInfo* PresetDatabase::getSampleInfoForBank(sampleid_t smplid, const bankname_t& bnkid)
    {
        SampleBank::SampleBlock* smplblk = getSampleBlockFor(smplid, bnkid);
        if (smplblk)
            return smplblk->pinfo_.get();
        return nullptr;
    }

    SampleBank::SampleBlock* PresetDatabase::getSampleBlockFor(sampleid_t smplid, const bankname_t& bnkid)
    {
        auto itfound = m_sampleMap.find(smplid);
        auto itend = m_sampleMap.end();

        if (itfound != itend)
        {
            std::map<bankname_t, SampleBank::SampleBlock>& bnkmap = itfound->second;

            //If we got the bank exactly, return now, without searching
            if (bnkmap.contains(bnkid))
                return &(bnkmap.at(bnkid));

            //Otherwise, we'll have to look for any lower banks numbers
            for (auto& bnkmap : bnkmap)
            {
                if (bnkmap.first <= bnkid)
                    return &(bnkmap.second);
            }
        }
        return nullptr;
    }
};