#ifndef DSE_PROGRAM_BANK_HPP
#define DSE_PROGRAM_BANK_HPP
#include <dse/dse_common.hpp>
#include <dse/containers/dse_keygroup_list.hpp>

#include <memory>

namespace DSE
{
    /*****************************************************************************************
        ProgramBank
            Contains the entries for each program contained in a SWD file, along with the
            file's keygroup table!
    *****************************************************************************************/
    class ProgramBank
    {
    public:
        typedef std::unique_ptr<DSE::ProgramInfo> ptrprg_t;

        ProgramBank()noexcept
        {}

        ~ProgramBank()noexcept
        {}

        ProgramBank(std::vector<ptrprg_t>&& prgminf, std::vector<DSE::KeyGroup>&& kgrp)noexcept
            :m_prgminfoslots(std::forward<std::vector<ptrprg_t>>(prgminf)),
            m_Groups(kgrp)
        {}

        ProgramBank(ProgramBank&& mv)noexcept
        {
            operator=(std::forward<ProgramBank>(mv));
        }

        ProgramBank& operator=(ProgramBank&& mv)noexcept
        {
            m_prgminfoslots = std::move(mv.m_prgminfoslots);
            m_Groups = std::move(mv.m_Groups);
            return *this;
        }

        ProgramBank(const ProgramBank& other)noexcept
        {
            operator=(other);
        }

        ProgramBank& operator=(const ProgramBank& other)noexcept
        {
            m_prgminfoslots.reserve(other.m_prgminfoslots.size());
            for (const ptrprg_t& ptr : other.m_prgminfoslots)
            {
                ptrprg_t newp;
                //The program ptrs may be null
                if (ptr)
                    newp = ptrprg_t(new DSE::ProgramInfo(*ptr));
                m_prgminfoslots.push_back(std::move(newp));
            }
            m_Groups = other.m_Groups;
            return *this;
        }

        ProgramBank merge(const ProgramBank& other)const
        {
            ProgramBank pbank;
            const size_t maxprgm = std::max(other.m_prgminfoslots.size(), m_prgminfoslots.size());
            pbank.m_prgminfoslots.resize(maxprgm);

            for (size_t i = 0; i < maxprgm; ++i)
            {
                ptrprg_t newp;
                if (i < other.m_prgminfoslots.size() && other.m_prgminfoslots[i])
                {
                    newp = ptrprg_t(new DSE::ProgramInfo(*(other.m_prgminfoslots[i])));
                }
                else if (i < m_prgminfoslots.size() && m_prgminfoslots[i])
                {
                    newp = ptrprg_t(new DSE::ProgramInfo(*(m_prgminfoslots[i])));
                }
                pbank.m_prgminfoslots[i] = ptrprg_t(newp.release());
            }

            const size_t maxkgrp = std::max(other.m_Groups.size(), m_Groups.size());
            pbank.m_Groups.GetVector().resize(maxkgrp);
            for (size_t cntkgrp = 0; cntkgrp < maxkgrp; ++cntkgrp)
            {
                KeyGroup grp;
                if (cntkgrp < other.m_Groups.size())
                    grp = other.m_Groups[cntkgrp];
                else if (cntkgrp < m_Groups.size())
                    grp = m_Groups[cntkgrp];
                else
                    continue;
                pbank.m_Groups[cntkgrp] = grp;
            }
            return pbank;
        }

        ptrprg_t& operator[](size_t index) { return m_prgminfoslots[index]; }
        const ptrprg_t& operator[](size_t index)const { return m_prgminfoslots[index]; }

        std::vector<ptrprg_t>& PrgmInfo() { return m_prgminfoslots; }
        const std::vector<ptrprg_t>& PrgmInfo()const { return m_prgminfoslots; }

        KeyGroupList& Keygrps() { return m_Groups; }
        const KeyGroupList& Keygrps()const { return m_Groups; }

    private:
        std::vector<ptrprg_t>       m_prgminfoslots;
        KeyGroupList                m_Groups;
    };
};

#endif // !DSE_PROGRAM_BANK_HPP
