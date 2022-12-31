#include <dse/dse_containers.hpp>
#include <iostream>
#include <sstream>
using namespace std;

namespace DSE
{
    std::string MusicSequence::printinfo()const
    {
        stringstream sstr;
        sstr << " ==== " <<m_meta.fname <<" ==== \n"
             << "CREATE ITME : " <<m_meta.createtime <<"\n"
             << "NB TRACKS   : " <<m_tracks.size()   <<"\n"
             << "TPQN        : " <<m_meta.tpqn       <<"\n\n";

        size_t cnttrk = 0;
        for( const auto & trk : m_tracks )
        {
            if( !trk.empty() )
                sstr << "\t- Track " <<cnttrk << ": Chan " << static_cast<uint16_t>(trk.GetMidiChannel()) << ", constains " << trk.size() <<" event(s).\n";
            ++cnttrk;
        }

        return sstr.str();
    }

///////////////////////////////////////////////////////////////////////////////
//PresetDatabase
///////////////////////////////////////////////////////////////////////////////
    PresetDatabase::PresetDatabase()
        :m_nbSampleBlocks(0)
    {}

    DSE::PresetBank& PresetDatabase::AddBank(DSE::PresetBank&& bnk)
    {
        const bankid_t myid = bnk.metadata().get_bank_id();
        //#TODO: Handle merging banks if neccessary 
        if (m_banks.contains(myid))
            bnk = HandleBankConflict(myid, bnk, m_banks.at(myid));

        //Keep some references to banks that contain samples, and banks that contain programs
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
                            m_sample2banks.try_emplace(cntsmpl, std::set<bankid_t>());
                        m_sample2banks.at(cntsmpl).insert(myid);

                        //Add our info to the sample map
                        if (!m_sampleMap.contains(cntsmpl))
                            m_sampleMap.try_emplace(cntsmpl, std::map<bankid_t, SampleBank::SampleBlock>());
                        m_sampleMap.at(cntsmpl).try_emplace(myid, SampleBank::SampleBlock(*smplbnk->sampleBlock(cntsmpl)));
                    }
                    if (smplbnk->IsInfoPresent(cntsmpl))
                    {
                        if (!m_preset2banks.contains(cntsmpl))
                            m_preset2banks.try_emplace(cntsmpl, std::set<bankid_t>());
                        m_preset2banks.at(cntsmpl).insert(myid);
                    }
                }
            }
        }

        auto result = m_banks.emplace(myid, std::forward<DSE::PresetBank>(bnk));
        return (result.first)->second;
    }

    PresetDatabase::sampledata_t* PresetDatabase::getSampleForBank(sampleid_t smplid, bankid_t bnkid)
    {
        SampleBank::SampleBlock* smplblk = getSampleBlockFor(smplid, bnkid);
        if (smplblk)
            return smplblk->pdata_.get();
        return nullptr;
    }

    DSE::WavInfo* PresetDatabase::getSampleInfoForBank(sampleid_t smplid, bankid_t bnkid)
    {
        SampleBank::SampleBlock* smplblk = getSampleBlockFor(smplid, bnkid);
        if (smplblk)
            return smplblk->pinfo_.get();
        return nullptr;
    }

    SampleBank::SampleBlock* PresetDatabase::getSampleBlockFor(sampleid_t smplid, bankid_t bnkid)
    {
        auto itfound = m_sampleMap.find(smplid);
        auto itend = m_sampleMap.end();

        if (itfound != itend)
        {
            std::map<bankid_t, SampleBank::SampleBlock>& bnkmap = itfound->second;

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

    PresetBank PresetDatabase::HandleBankConflict(bankid_t conflictid, const PresetBank& incoming, const PresetBank& existing)
    {
        clog << "<*>- Newly loaded bank \"" << incoming.metadata().origfname <<"\" conflicts with an already loaded bank \"" << existing.metadata().origfname <<"\", with bank id:" <<conflictid <<".\n";
        auto pPrgmbnk = incoming.prgmbank().lock();
        auto pSmplbnk = incoming.smplbank().lock();

        auto pExPrgmbnk = existing.prgmbank().lock();
        if ((pPrgmbnk != nullptr) && (pExPrgmbnk != nullptr))
        {
            clog << "\tAttempting Program Bank merge..\n";
            const size_t maxIncoming = pPrgmbnk->PrgmInfo().size();
            const size_t maxExisting = pExPrgmbnk->PrgmInfo().size();
            const presetid_t maxpres = std::max(maxIncoming, maxExisting);

            for (presetid_t cntpres = 0; cntpres < maxpres; ++cntpres)
            {
                if ((cntpres < maxExisting) && (cntpres < maxIncoming))
                {
                    const ProgramInfo* pExPrg = (*pExPrgmbnk)[cntpres].get();
                    const ProgramInfo* pIncPrg = (*pPrgmbnk)[cntpres].get();
                    if(pExPrg != nullptr && pIncPrg != nullptr)
                    {
                        clog << "\t- Conflict Preset#" << (int)cntpres << ": \n\tExisting:\n{\n" << (*pExPrg) << "}\n\tIncoming:\n{" << (*pIncPrg) << "}\n";
                    }
                }
            }
        }

        auto pExSmplbnk = existing.smplbank().lock();
        if ((pSmplbnk != nullptr) && (pExSmplbnk != nullptr))
        {
            clog << "\tAttempting Sample Bank merge..\n";
            const size_t maxIncoming = pSmplbnk->NbSlots();
            const size_t maxExisting = pExSmplbnk->NbSlots();
            const sampleid_t maxsmpl = std::max(maxIncoming, maxExisting);

            for (sampleid_t cntsmpl = 0; cntsmpl < maxsmpl; ++cntsmpl)
            {
                if ((cntsmpl < maxIncoming) && (cntsmpl < maxExisting))
                {
                    const SampleBank::SampleBlock* pExSmpl  = pExSmplbnk->sampleBlock(cntsmpl);
                    const SampleBank::SampleBlock* pIncSmpl = pSmplbnk->sampleBlock(cntsmpl);
                    if ((pExSmpl != nullptr) && (pIncSmpl != nullptr) && !pExSmpl->isnull() && !pIncSmpl->isnull())
                    {
                        if ((pExSmpl->pinfo_ != nullptr) && (pIncSmpl->pinfo_ != nullptr))
                        {
                            clog << "\t- Conflict Sample Info#" << (int)cntsmpl << ": \n\tExisting:\n{\n" << (*pExSmpl->pinfo_) << "}\n\tIncoming:\n{" << (*pIncSmpl->pinfo_) << "}\n";
                        }
                        
                        if ((pExSmpl->pdata_ != nullptr) && (pIncSmpl->pdata_ != nullptr))
                        {
                            clog << "\t- Conflict Sample Data#" << (int)cntsmpl << "\n";
                        }
                    }
                }
            }
        }
        return existing.merge(incoming);
    }
};