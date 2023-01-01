#ifndef DSE_KEY_GROUP_LIST_HPP
#define DSE_KEY_GROUP_LIST_HPP
#include <dse/dse_common.hpp>

namespace DSE
{
    /*****************************************************************************************
        KeygroupList
            Encapsulate and validate keygroups.
    *****************************************************************************************/
    class KeyGroupList
    {
    public:
        typedef std::vector<DSE::KeyGroup>::iterator            iterator;
        typedef std::vector<DSE::KeyGroup>::const_iterator      const_iterator;
        typedef std::vector<DSE::KeyGroup>::reference           reference;
        typedef std::vector<DSE::KeyGroup>::const_reference     const_reference;

        KeyGroupList()
        {}

        KeyGroupList(const std::vector<DSE::KeyGroup>& cpy)
            :m_groups(cpy)
        {}

        inline size_t             size()const { return m_groups.size(); }
        inline bool               empty()const { return m_groups.empty(); }

        inline iterator           begin() { return m_groups.begin(); }
        inline const_iterator     begin()const { return m_groups.begin(); }
        inline iterator           end() { return m_groups.end(); }
        inline const_iterator     end()const { return m_groups.end(); }

        inline reference& front() { return m_groups.front(); }
        inline const_reference& front()const { return m_groups.front(); }
        inline reference& back() { return m_groups.back(); }
        inline const_reference& back()const { return m_groups.back(); }

        //Validated Kgrp access!
        inline DSE::KeyGroup& operator[](size_t kgrp)
        {
            if (kgrp < size())
                return m_groups[kgrp];
            else
                return m_groups.front(); //Return the gloabal keygroup if the kgrp number is invalid (This might differ in the way the DSE engine handles out of range kgrp values)
        }

        inline const DSE::KeyGroup& operator[](size_t kgrp)const
        {
            if (kgrp < size())
                return m_groups[kgrp];
            else
                return m_groups.front(); //Return the gloabal keygroup if the kgrp number is invalid (This might differ in the way the DSE engine handles out of range kgrp values)
        }

        //Access to raw vector
        inline std::vector<DSE::KeyGroup>& GetVector() { return m_groups; }
        inline const std::vector<DSE::KeyGroup>& GetVector()const { return m_groups; }

    private:
        std::vector<DSE::KeyGroup>  m_groups;
    };
};

#endif // !DSE_KEY_GROUP_LIST_HPP
