#ifndef DSE_PRESET_DB_HPP
#define DSE_PRESET_DB_HPP
#include <dse/dse_common.hpp>
#include <dse/containers/dse_preset_bank.hpp>

#include <map>
#include <unordered_map>
#include <set>
#include <string>
#include <vector>

namespace DSE
{
    /// <summary>
    /// Wrapper around a large set of PresetBanks to allow much more easily indexing and accessing its contents.
    /// </summary>
    class PresetDatabase
    {
    public:
        using sampledata_t = std::vector<uint8_t>;
        using bankname_t = std::string;

        //Wrapper for adding new banks to the loader so we can set them up properly
        DSE::PresetBank& AddBank(DSE::PresetBank&& bnk);

        PresetDatabase();

    public:
        //Helpers
        //Retrieve the last overriden sample data for the given sample and bank
        sampledata_t* getSampleForBank(sampleid_t smplid, const bankname_t& bnkid);

        //Retrieve the last overriden sample info for the given sample and bank
        DSE::WavInfo* getSampleInfoForBank(sampleid_t smplid, const bankname_t& bnkid);

        //Retrieve the sample block for a given sample id
        DSE::SampleBank::SampleBlock* getSampleBlockFor(sampleid_t smplid, const bankname_t& bnkid);

        inline const DSE::SampleBank::SampleBlock* getSampleBlockFor(sampleid_t smplid, const bankname_t& bnkid)const
        {
            return const_cast<PresetDatabase*>(this)->getSampleBlockFor(smplid, bnkid);
        }

        inline const DSE::WavInfo* getSampleInfoForBank(sampleid_t smplid, const bankname_t& bnkid)const
        {
            return const_cast<PresetDatabase*>(this)->getSampleInfoForBank(smplid, bnkid);
        }

        inline const sampledata_t* getSampleForBank(sampleid_t smplid, const bankname_t& bnkid)const
        {
            return const_cast<PresetDatabase*>(this)->getSampleForBank(smplid, bnkid);
        }

        inline sampleid_t getNbSampleBlocks()const
        {
            return m_nbSampleBlocks;
        }

    public:
        //Std access stuff
        typedef std::unordered_map<bankname_t, PresetBank>::iterator       iterator;
        typedef std::unordered_map<bankname_t, PresetBank>::const_iterator const_iterator;

        inline size_t size()const { return m_banks.size(); }
        inline bool empty()const { return m_banks.empty(); }
        inline bool contains(const bankname_t bankid)const { return m_banks.contains(bankid); }

        inline iterator begin() { return m_banks.begin(); }
        inline const_iterator begin()const { return m_banks.begin(); }

        inline iterator end() { return m_banks.end(); }
        inline const_iterator end()const { return m_banks.end(); }

        inline PresetBank& operator[](const bankname_t& index) { return m_banks.at(index); }
        inline const PresetBank& operator[](const bankname_t& index)const { return m_banks.at(index); }

        inline iterator find(const bankname_t& bankid) { return m_banks.find(bankid); }
        inline const_iterator find(const bankname_t& bankid)const { return m_banks.find(bankid); }

        inline PresetBank& at(const bankname_t& bankid) { return m_banks.at(bankid); }
        inline const PresetBank& at(const bankname_t& bankid)const { return m_banks.at(bankid); }

    private:
        std::unordered_map<std::string, DSE::PresetBank>                    m_banks;          //Map of all our banks by a string id, either the original filename, or something else in the case where there's no filename
        std::map<sampleid_t, std::map<bankname_t, SampleBank::SampleBlock>> m_sampleMap;      //Allow us to grab the latest overriden sample entry by id and bank id.
        std::map<presetid_t, std::set<bankname_t>>                          m_preset2banks;   //List what loaded banks actually define a given preset
        std::map<sampleid_t, std::set<bankname_t>>                          m_sample2banks;   //List in what banks a given sample id is defined
        sampleid_t                                                          m_nbSampleBlocks; //The Maximum number of sample blocks contained throughout all banks
    };
};

#endif // !DSE_PRESET_DB_HPP
